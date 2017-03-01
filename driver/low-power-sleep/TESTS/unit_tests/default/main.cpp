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

// ----------------------------------------------------------------
// PRIVATE DECLARATIONS
// ----------------------------------------------------------------

// Declare a sub-class of the low power driver so that we can get at the
// protected methods to test them
class SubLowPower: public LowPower {
public:
    typedef struct {
        uint32_t second;   //<! 0 to 59
        uint32_t minute;   //<! 0 to 59
        uint32_t hour;     //<! 0 to 23
        uint32_t day;      //<! 1 to 31, i.e. 1 based
        uint32_t month;    //<! 1 to 12, i.e. 1 based
        uint32_t year;     //<! 0 to YEAR_MAX where 0 = BASE_YEAR
    } PublicDateTime_t;
    
    // Public versions of the protected classes from LowPower
    void publicAddPeriod(SubLowPower::PublicDateTime_t * pDateTime, time_t periodSeconds);
    bool publicSetAlarmA(const SubLowPower::PublicDateTime_t * pDateTime);
    bool publicSetRtcAlarm(time_t periodSeconds);
    bool publicIsLeapYear(uint32_t year);
    
    // Convert a PublicDateTime_t struct into seconds since 1990
    time_t dateTimeToSecondsSince1990(SubLowPower::PublicDateTime_t * pDateTime);
    
protected:
};

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// Courtesy of Google...
uint32_t const gLeapYears[] = {1904, 1908, 1912, 1916, 1920, 1924, 1928, 1932, \
                               1936, 1940, 1944, 1948, 1952, 1956, 1960, 1964, \
                               1968, 1972, 1976, 1980, 1984, 1988, 1992, 1996, \
                               2000, 2004, 2008, 2012, 2016, 2020};

// Days in each month
uint8_t gDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// My sub-class
SubLowPower *gSubLowPower = new SubLowPower();

// ----------------------------------------------------------------
// PRIVATE FUNCTIONS
// ----------------------------------------------------------------

// Definition of the sub-class methods, just calling the protected ones
void SubLowPower::publicAddPeriod(SubLowPower::PublicDateTime_t * pDateTime, time_t periodSeconds)
{
    LowPower::DateTime_t dateTime;
    
    if (pDateTime != NULL) {
        memcpy (&dateTime, pDateTime, sizeof (dateTime));
        addPeriod(&dateTime, periodSeconds);
        memcpy (pDateTime, &dateTime, sizeof (*pDateTime));
    } else {
        addPeriod(NULL, periodSeconds);
    }    
}
bool SubLowPower::publicSetAlarmA(const SubLowPower::PublicDateTime_t * pDateTime)
{
    bool success = false;
    LowPower::DateTime_t dateTime;
    
    if (pDateTime != NULL) {
        memcpy (&dateTime, pDateTime, sizeof (dateTime));
        success = setAlarmA(&dateTime);
    } else {
        success = setAlarmA(NULL);
    }
    
    return success;
}
bool SubLowPower::publicSetRtcAlarm(time_t periodSeconds)
{
    return setRtcAlarm(periodSeconds);
}
bool SubLowPower::publicIsLeapYear(uint32_t year)
{
    return isLeapYear(year);
}

// Convert a PublicDateTime_t struct into seconds since BASE_YEAR
time_t SubLowPower::dateTimeToSecondsSince1990(SubLowPower::PublicDateTime_t * pDateTime) {
    time_t periodSeconds = 0;
    
    if (pDateTime != NULL) {
        MBED_ASSERT (pDateTime->second < 60);
        MBED_ASSERT (pDateTime->minute < 60);
        MBED_ASSERT (pDateTime->hour < 24);
        MBED_ASSERT ((pDateTime->day >= 1) && (pDateTime->day <= 31));
        MBED_ASSERT ((pDateTime->month >= 1) && (pDateTime->month <= 12));
        MBED_ASSERT (pDateTime->year <= YEAR_MAX);

        // Do the easy ones
        periodSeconds += pDateTime->second;
        periodSeconds += pDateTime->minute * 60;
        periodSeconds += pDateTime->hour * 60 * 60;
        periodSeconds += (pDateTime->day - 1) * 60 * 60 * 24; // -1 'cos it is 1 based
        periodSeconds += pDateTime->year * 60 * 60 * 24 * 365;
        // Sort out the extra days due to previous leap years
        for (uint32_t x = 0; x < pDateTime->year; x++) {
            if (gSubLowPower->publicIsLeapYear(x + BASE_YEAR)) {
                periodSeconds += 60 * 60 * 24;
            }
        }
        // Sort out the month
        for (uint8_t x = 0; x < pDateTime->month - 1; x++) { // -1 'cos it is 1 based
            periodSeconds += gDaysInMonth[x] * 60 * 60 * 24;
            if ((x == 1) && gSubLowPower->publicIsLeapYear(pDateTime->year + BASE_YEAR)) {
                periodSeconds += 60 * 60 * 24;
            }
        }
    }
    
    return periodSeconds;
}

// ----------------------------------------------------------------
// TESTS
// ----------------------------------------------------------------

// Test leap year calculation
void test_leap_year() {
    bool found;
    
    for (uint32_t x = 1904; x <= 2020; x++) {
        found = false;
        for (uint32_t y = 0; (y < sizeof(gLeapYears)) && !found; y++) {
            if (x == gLeapYears[y]) {
                found = true;
            }
        }
        
        if (found) {
            TEST_ASSERT(gSubLowPower->publicIsLeapYear(x));
        } else {
            TEST_ASSERT_FALSE(gSubLowPower->publicIsLeapYear(x));
        }
    }        
}

// Test adding small numbers of seconds to a time/date struct for a year and a day
void test_add_small_increments() {
    SubLowPower::PublicDateTime_t dateTimeOriginal;
    SubLowPower::PublicDateTime_t dateTime;
    time_t secondsSince1990;
    uint32_t startYear = 2016 - BASE_YEAR;// 2016 was a leap year
    time_t startSeconds;
    time_t incrementSeconds;
    time_t addedSeconds = 0;
    
    // Set the original to zero (taking into account the 1-based items)
    memset (&dateTimeOriginal, 0, sizeof (dateTimeOriginal));
    dateTimeOriginal.day = 1;
    dateTimeOriginal.month = 1;
    
    memcpy (&dateTime, &dateTimeOriginal, sizeof (dateTime));
    
    // Call should not explode if the date/time is NULL
    gSubLowPower->publicAddPeriod(NULL, 5);
    
    // Check that our local conversion function does basically the right thing
    TEST_ASSERT_EQUAL_INT32 (0, gSubLowPower->dateTimeToSecondsSince1990(&dateTimeOriginal));
    
    // Adding zero should do nothing
    gSubLowPower->publicAddPeriod(&dateTime, 0);
    TEST_ASSERT (memcmp (&dateTimeOriginal, &dateTime, sizeof (dateTime)) == 0);
    
    // Now run through a leap year and 1 day, in small numbers of seconds increments
    memset (&dateTime, 0, sizeof (dateTime));
    dateTime.day = 1;
    dateTime.month = 1;
    dateTime.year = startYear;
    
    // First, work out the number of seconds passed due to previous years, taking into account
    // leap years
    startSeconds = dateTime.year * 60 * 60 * 24 * 365;
    for (uint32_t x = 0; x < dateTime.year; x++) {
        if (gSubLowPower->publicIsLeapYear(x + BASE_YEAR)) {
            startSeconds += 60 * 60 * 24;
        }
    }
    
    // Now actually do the incrementing/checking
    while (addedSeconds < 60 * 60 * 24 * 366) {
        // Check
        secondsSince1990 = gSubLowPower->dateTimeToSecondsSince1990(&dateTime);
        if (secondsSince1990 != startSeconds + addedSeconds) {
            printf ("%d seconds since 1990 produces %04d-%02d-%02d %02d:%02d:%02d, which is actually %d seconds since 1990.\n",
                    startSeconds + addedSeconds, dateTime.year + BASE_YEAR, dateTime.month, dateTime.day, dateTime.hour,
                    dateTime.minute, dateTime.second, secondsSince1990);
            TEST_ASSERT_EQUAL_INT32 (startSeconds + addedSeconds, secondsSince1990);
        }
        // Increment in the range 0 to 9 seconds
        incrementSeconds = (time_t) (rand() % 10);
        addedSeconds += incrementSeconds;
        gSubLowPower->publicAddPeriod(&dateTime, incrementSeconds);
    }
}

// Test adding wide ranging random increments to a time/date struct
void test_add_wide_increments() {
    SubLowPower::PublicDateTime_t dateTime;
    time_t secondsSince1990;
    time_t secondsCheck;
    
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        // Set the original to zero (taking into account the 1-based items)
        memset (&dateTime, 0, sizeof (dateTime));
        dateTime.day = 1;
        dateTime.month = 1;
        // Chose a random number of seconds to add
        secondsSince1990 = (time_t) (rand() % 0x7FFFFFFF);
        // Add them
        gSubLowPower->publicAddPeriod(&dateTime, secondsSince1990);
        // Check them
        secondsCheck = gSubLowPower->dateTimeToSecondsSince1990(&dateTime);
        if (secondsCheck != secondsSince1990) {
            printf ("%d seconds since 1990 produces %04d-%02d-%02d %02d:%02d:%02d, which is actually %d seconds since 1990.\n",
                    secondsSince1990, dateTime.year + BASE_YEAR, dateTime.month, dateTime.day, dateTime.hour,
                    dateTime.minute, dateTime.second, secondsCheck);
            TEST_ASSERT_EQUAL_INT32 (secondsSince1990, secondsCheck);
        }
    }
}

// Test using random start dates and adding random increments to a time/date struct
void test_add_random_starts() {
    SubLowPower::PublicDateTime_t startDateTime;
    SubLowPower::PublicDateTime_t newDateTime;
    time_t secondsSince1990;
    time_t secondsCheck;
    time_t startSecondsSince1990;
    time_t newSecondsSince1990;
    
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        // Set the original to zero (taking into account the 1-based items)
        memset (&startDateTime, 0, sizeof (startDateTime));
        startDateTime.day = 1;
        startDateTime.month = 1;
        // Add a random number of seconds (the range half of that
        // in test_add_wide_increments() to avoid overflow)
        secondsSince1990 = (time_t) (rand() % (0x7FFFFFFF / 2));
        // Add them to create a new start date
        gSubLowPower->publicAddPeriod(&startDateTime, secondsSince1990);
        startSecondsSince1990 = gSubLowPower->dateTimeToSecondsSince1990(&startDateTime);
        memcpy (&newDateTime, &startDateTime, sizeof (newDateTime));
        
        // Now add a new random number to that start date
        secondsSince1990 = (time_t) (rand() % (0x7FFFFFFF / 2));
        gSubLowPower->publicAddPeriod(&newDateTime, secondsSince1990);
        newSecondsSince1990 = gSubLowPower->dateTimeToSecondsSince1990(&newDateTime);
        
        // Check the result
        secondsCheck = newSecondsSince1990 - startSecondsSince1990;
        if (secondsCheck != secondsSince1990) {
            printf ("Adding %d seconds since 1990 to %04d-%02d-%02d %02d:%02d:%02d (%d seconds) produces %04d-%02d-%02d %02d:%02d:%02d (%d seconds), which is actually %d seconds difference.\n",
                    secondsSince1990, startDateTime.year + BASE_YEAR, startDateTime.month, startDateTime.day, startDateTime.hour,
                    startDateTime.minute, startDateTime.second, startSecondsSince1990,
                    newDateTime.year + BASE_YEAR, newDateTime.month, newDateTime.day, newDateTime.hour,
                    newDateTime.minute, newDateTime.second, newSecondsSince1990, secondsCheck);
            TEST_ASSERT_EQUAL_INT32 (secondsSince1990, secondsCheck);
        }
    }
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
    Case("Leap year", test_leap_year),
    Case("Add period (small increments, takes 50 seconds)", test_add_small_increments),
    Case("Add period (wide ranging increments)", test_add_wide_increments),
    Case("Add period (random start dates)", test_add_random_starts)
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
