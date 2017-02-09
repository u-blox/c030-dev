#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_gauge_ltc2943.h"

using namespace utest::v1;

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(P0_27, P0_28);

/// RSense (in mOhm) on UTM board
#define RSENSE_MOHM 68

// Pick some sensible minimum and maximum numbers
#define MAX_TEMPERATURE_READING_C  80
#define MIN_TEMPERATURE_READING_C -20
#define MIN_VOLTAGE_READING_MV     0
#define MAX_VOLTAGE_READING_MV     12000 // Bigger than a 3 cell LiPo
#define MAX_CURRENT_READING_MA     2000
#define MIN_CURRENT_READING_MA    -2000

// Test that the LTC2943 battery gauge can be initialised
void test_init() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    
    TEST_ASSERT_FALSE(pBatteryGauge->init(NULL, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
}

// Test that battery capacity monitoring can be performed
void test_monitor() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->setMonitor(true));
    
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    // Normal case
    TEST_ASSERT(pBatteryGauge->setMonitor(true));    
    // TODO do something to assess whether it's actually working
    TEST_ASSERT(pBatteryGauge->setMonitor(false));
    
    // Normal case, slow mode
    TEST_ASSERT(pBatteryGauge->setMonitor(true, true));    
    // TODO do something to assess whether it's actually working slowly
    TEST_ASSERT(pBatteryGauge->setMonitor(false));
}

// Test that a temperature reading can be performed
void test_temperature() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
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
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
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

// Test that a current reading can be performed
void test_current() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    int32_t currentMA;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getVoltage(&currentMA));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getCurrent(&currentMA));
    printf ("Current %.3f A.\n", ((float) currentMA) / 1000);
    // Range check
    TEST_ASSERT((currentMA >= MIN_CURRENT_READING_MA) && (currentMA <= MAX_CURRENT_READING_MA));
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getCurrent(NULL));
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
    Case("Monitor", test_monitor),
    Case("Temperature read", test_temperature),
    Case("Voltage read", test_voltage),
    Case("Current read", test_current),
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
