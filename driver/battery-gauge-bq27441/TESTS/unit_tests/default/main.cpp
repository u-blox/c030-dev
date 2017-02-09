#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_gauge_bq27441.h"

using namespace utest::v1;

// Pick some sensible minimum and maximum numbers
#define MIN_VOLTAGE_READING_MV     0
#define MAX_VOLTAGE_READING_MV     12000 // Bigger than a 3 cell LiPo
#define MIN_CAPACITY_READING_MAH   0
#define MAX_CAPACITY_READING_MAH   30000 // A very big battery indeed
#define MAX_TEMPERATURE_READING_C  80
#define MIN_TEMPERATURE_READING_C -20

/// Default RSense (in mOhm)
#define RSENSE_MOHM 10

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(P0_27, P0_28);

// Test that the BQ27441 battery gauge can be initialised
void test_init() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    
    TEST_ASSERT_FALSE(pBatteryGauge->init(NULL, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
}

// Test that a temperature reading can be performed
void test_temperature() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t temperatureC;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getTemperature(&temperatureC));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getTemperature(&temperatureC));
    printf ("Temperature %d C.\n", temperatureC);
    // Range check
    TEST_ASSERT((temperatureC >= MIN_TEMPERATURE_READING_C) && (temperatureC <= MAX_TEMPERATURE_READING_C));
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getTemperature(NULL));
}

// Test that a voltage reading can be performed
void test_voltage() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t voltageMV;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getVoltage(&voltageMV));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getVoltage(&voltageMV));
    printf ("Voltage %.3f V.\n", ((float) voltageMV) / 1000);
    // Range check
    TEST_ASSERT((voltageMV >= MIN_VOLTAGE_READING_MV) && (voltageMV <= MAX_VOLTAGE_READING_MV));
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getVoltage(NULL));
}

// Test that a remaining capacity reading can be performed
void test_remaining_capacity() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t capacityMAh;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getRemainingCapacity(&capacityMAh));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getRemainingCapacity(&capacityMAh));
    printf ("Remaining capacity %.3f Ah.\n", ((float) capacityMAh) / 1000);
    // Range check
    TEST_ASSERT((capacityMAh >= MIN_CAPACITY_READING_MAH) && (capacityMAh <= MAX_CAPACITY_READING_MAH));

    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getRemainingCapacity(NULL));
}

// Test that a remaining percentage reading can be performed
void test_remaining_percentage() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t batteryPercent;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getRemainingPercentage(&batteryPercent));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(&batteryPercent));
    printf ("Remaining percentage %d%%.\n", batteryPercent);
    // Range check
    TEST_ASSERT((batteryPercent >= 0) && (batteryPercent <= 100));

    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(NULL));
}

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(10, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Initialisation", test_init),
    Case("Temperature read", test_temperature),
    Case("Voltage read", test_voltage),
    Case("Remaining capacity read", test_remaining_capacity),
    Case("Remaining percentage read", test_remaining_percentage),
};

Specification specification(test_setup, cases);

// Entry point into the tests
int main() {    
    bool success = false;
    
    if (gpI2C != NULL) {        
        success = !Harness::run(specification);
    } else {
        printf ("Unable to instantiate I2C interface.\n");
    }
    
    return success;
}

// End Of File
