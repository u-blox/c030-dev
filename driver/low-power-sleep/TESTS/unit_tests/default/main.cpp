#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "low_power.h"

using namespace utest::v1;

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

#ifndef NUM_RAND_ITERATIONS
// The number of iterations of random input values in various tests
#define NUM_RAND_ITERATIONS 1000
#endif

// The duration to sleep for during a test
#define SLEEP_DURATION_SECONDS 2

// Ticker period, should be set such that it will occur
// within SLEEP_DURATION_SECONDS
#define TICKER_PERIOD_US 1000000

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// An instance of low power 
LowPower *gpLowPower = new LowPower();

// A ticker
Ticker gTicker;

// A count of ticks
uint32_t gTickCount = 0;

// ----------------------------------------------------------------
// PRIVATE FUNCTIONS
// ----------------------------------------------------------------

// Set the pTimeStruct to time zero
static void earliestTimeStruct(struct tm * pTimeStruct)
{
    memset (pTimeStruct, 0, sizeof (*pTimeStruct));
    pTimeStruct->tm_mday = 1;
    pTimeStruct->tm_year = 70;
}

// A ticker function that increments gTickCount
static void incTickCount(void)
{
    gTickCount++;
}

// Check if the ticker has gone off
static bool tickerExpired(void)
{
    return gTickCount > 0;
}

// Set off the ticker and timer
static void startTicker (void)
{
    // Reset the counter
    gTickCount = 0;
    
    // Start the ticker
    gTicker.attach_us (&incTickCount, TICKER_PERIOD_US);
    
    // Make sure that it is running
    wait_ms ((TICKER_PERIOD_US / 1000) + 1);
    TEST_ASSERT_EQUAL_UINT32(1, gTickCount);
    
    // Reset the counter again
    gTickCount = 0;
}

// Stop the ticker
static void stopTicker (void)
{
    // Stop the ticker
    gTicker.attach_us (NULL, 0);
    
    // Reset counters
    gTickCount = 0;
    
    // Make sure that it has really stopped
    wait_ms ((TICKER_PERIOD_US / 1000) + 1);
    TEST_ASSERT_EQUAL_UINT32(0, gTickCount);
}

// ----------------------------------------------------------------
// TESTS
// ----------------------------------------------------------------

// Test Stop mode
void test_stop_mode() {
    time_t startTime;
    struct tm alarmStruct;
    
    // Set the time to get the RTC running
    set_time(0);
    
    // First test a short stop
    startTime = time(NULL);
    startTicker();
    TEST_ASSERT(gpLowPower ->enterStop(SLEEP_DURATION_SECONDS));
    TEST_ASSERT_FALSE(tickerExpired());
    TEST_ASSERT(time(NULL) - startTime >= SLEEP_DURATION_SECONDS);
    stopTicker();
    
    // Do it again with unnecessary interrupts disabled
    startTime = time(NULL);
    startTicker();
    TEST_ASSERT(gpLowPower ->enterStop(SLEEP_DURATION_SECONDS, true));
    TEST_ASSERT_FALSE(tickerExpired());
    TEST_ASSERT(time(NULL) - startTime >= SLEEP_DURATION_SECONDS);
    stopTicker();
    
    // Now run through all the corner cases of sleep
    // boundaries I can think of
    earliestTimeStruct(&alarmStruct);
    alarmStruct.tm_sec = 58;
    set_time(mktime(&alarmStruct)); 
    startTime = time(NULL);
    startTicker();    
    TEST_ASSERT(gpLowPower ->enterStop(SLEEP_DURATION_SECONDS));
    TEST_ASSERT_FALSE(tickerExpired());
    TEST_ASSERT(time(NULL) - startTime >= SLEEP_DURATION_SECONDS);
    stopTicker();

    // Let the UART recover after sleep or mbedgt can
    // be confused by partial prints
    wait_ms(100);
    printf("Printing something to flush UART of rubbish.");
}

// ----------------------------------------------------------------
// TEST ENVIRONMENT
// ----------------------------------------------------------------

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea with a timeout
    GREENTEA_SETUP(120, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Stop mode", test_stop_mode)
};

Specification specification(test_setup, cases);

// ----------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------

int main() {    
    bool success = false;
    
    srand(time(NULL));
    success = !Harness::run(specification);
    
    return success;
}

// End Of File
