#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "low_power.h"
#include <stm32f4xx_hal_rcc.h>
#include <stm32f4xx_hal_pwr.h>
 
using namespace utest::v1;

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

#ifndef NUM_RAND_ITERATIONS
// The number of iterations of random input values in various tests
#define NUM_RAND_ITERATIONS 1000
#endif

// The duration to sleep for during a test
#define SLEEP_DURATION_SECONDS 3

// Ticker period, should be set such that it will occur
// within SLEEP_DURATION_SECONDS
#define TICKER_PERIOD_US 1000000

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// An instance of low power
static LowPower *gpLowPower = new LowPower();

// A ticker
static Ticker gTicker;

// A count of ticks
static uint32_t gTickCount = 0;

// An item to take up all of Backup SRAM
BACKUP_SRAM
static char gBackupSram[4096];

// ----------------------------------------------------------------
// PRIVATE FUNCTIONS
// ----------------------------------------------------------------

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
    gTicker.detach();
    
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
    
    // Test a short stop
    startTime = time(NULL);
    startTicker();
    gpLowPower->enterStop(SLEEP_DURATION_SECONDS * 1000);
    TEST_ASSERT_FALSE(tickerExpired());
    TEST_ASSERT(time(NULL) - startTime >= SLEEP_DURATION_SECONDS - 1); // -1 for tolerance
    stopTicker();
    
    // Do it again
    startTime = time(NULL);
    startTicker();
    gpLowPower->enterStop(SLEEP_DURATION_SECONDS * 1000);
    TEST_ASSERT_FALSE(tickerExpired());
    TEST_ASSERT(time(NULL) - startTime >= SLEEP_DURATION_SECONDS - 1); // -1 for tolerance
    stopTicker();
}

// Test the number of user interrupts that have been enabled
void test_interrupts_enabled() {
    int32_t userInterruptsEnabled;
    uint8_t list[NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET];
    
    // Fill with a known value
    memset(&(list[0]), 0xff, sizeof (list));
    
    // Check that we can just get the number back without any parameters
    userInterruptsEnabled = gpLowPower->numUserInterruptsEnabled();
    
#ifdef TARGET_STM
    TEST_ASSERT_EQUAL_INT32(2, userInterruptsEnabled);
#endif

    // Now ask for a list, but only with one entry
    userInterruptsEnabled = gpLowPower->numUserInterruptsEnabled(&(list[0]), 1);
    // Check that the second entry is untouched
    TEST_ASSERT_EQUAL_UINT8(0xff, list[1]);
#ifdef TARGET_STM
    // Check that two interrupts are enabled
    TEST_ASSERT_EQUAL_INT32(2, userInterruptsEnabled);
    // Check that the first entry is the RTC Alarm interrupt (used by enableStop())
    TEST_ASSERT_EQUAL_UINT8(RTC_Alarm_IRQn, list[0]);
#endif

    // Now ask for the full list
    userInterruptsEnabled = gpLowPower->numUserInterruptsEnabled(&(list[0]), sizeof (list));
#ifdef TARGET_STM
    // Check that two interrupts are enabled
    TEST_ASSERT_EQUAL_INT32(2, userInterruptsEnabled);
    // Check that the third entry is untouched
    TEST_ASSERT_EQUAL_UINT8(0xff, list[2]);
    // Check that the first entry is the RTC Alarm interrupt
    TEST_ASSERT_EQUAL_UINT8(RTC_Alarm_IRQn, list[0]);
    // Check that the second entry is the TIM5 interrupt (used for RTOS tick)
    TEST_ASSERT_EQUAL_UINT8(TIM5_IRQn, list[1]);
#endif
}

// Test Standby mode
void test_standby_mode() {
#ifdef TARGET_STM
    // Check that the Backup SRAM array has been placed correctly
    TEST_ASSERT_EQUAL_UINT32 (0x40024000, &(gBackupSram[0]));
#endif
    
    // Fill backup SRAM with 0x42
    memset (&(gBackupSram[0]), 0x42, sizeof (gBackupSram));
    
    // Test that we succeeded in doing so
    for (uint32_t x = 0; x < sizeof (gBackupSram); x++) {
        TEST_ASSERT_EQUAL_INT8(0x42, gBackupSram[x]);
    }
    
    // Now store the current time in the first four bytes
    *((time_t *) &(gBackupSram[0])) = time(NULL);

    startTicker();
    gpLowPower->enterStandby(SLEEP_DURATION_SECONDS * 1000);

    // At the end of Standby mode the processor will reset and we will
    // come back again at main().  There we can check if (a) Backup SRAM
    // is still as we left it and (b) the right amount of time has expired

    // We should never get here
    TEST_ASSERT(false);
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
    Case("Stop mode", test_stop_mode),
    // This must be run second as test_stop_mode is expected to enable some interupts
    Case("Num user interrupts enabled", test_interrupts_enabled),
#ifdef TARGET_STM
 // Standby mode doesn't work while debugging, it's just the way the HW is
# ifdef DEBUG
# error If you want to run the test suite in debug mode, comment out this line.
# else
    // Standby mode is only implemented for ST micro cores
    // This must be the last test as it resets us back to main()
    Case("Standby mode", test_standby_mode)
# endif
#endif
};

Specification specification(test_setup, cases);

// ----------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------

int main() {
    bool success = true;
    
#ifndef DEBUG
    gpLowPower->exitDebugMode();
#endif

    // If the RTC is running then we must have been
    // in the Standby mode test and have just come back
    // from reset, so check that Backup SRAM has the
    // expected contents
    if (time(NULL) != (time_t) -1) {
        // Check that we are at least SLEEP_DURATION_SECONDS past the time
        // we entered Standby mode, which is stored at the start of gBackSream
        printf ("Time: %d, recorded time %d.\n", time(NULL), *((time_t *) &(gBackupSram[0])));
        if (time(NULL) - *((time_t *) &(gBackupSram[0])) < SLEEP_DURATION_SECONDS - 1) { // -1 for tolerance
            success = false;
        }

        // The rest should be the fill value
        for (uint32_t x = sizeof (time_t); (x < (sizeof (gBackupSram) - sizeof (time_t))) && success; x++) {
            if (gBackupSram[x] != 0x42) {
                success = false;
            }
        }

        TEST_ASSERT(success);

        // Now we implement the end of the suite of tests manually
        printf ("{{__testcase_finish;Standby mode;%01d;%01d}}\n", success, !success);
        printf ("{{__testcase_summary;3;%01d}}\n", !success);
        if (success) {
            printf("{{end;success}}\n");
        } else {
            printf("{{end;failure}}\n");
        }
        printf ("{{__exit;%01d}}\n", !success);
    } else {
        success = !Harness::run(specification);
    }
    
    return success;
}

// End Of File
