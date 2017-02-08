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

// Pick some sensible minimum and maximum numbers
#define MAX_VOLTAGE_READING_MV 12000 // Bigger than a 3 cell LiPo
#define MAX_TEMPERATURE_READING_C  80
#define MIN_TEMPERATURE_READING_C -20

// Test that the LTC2943 battery gauge can be initialised
void test_init() {
    BatteryGaugeLtc2943 * pGauge = new BatteryGaugeLtc2943();
    
    TEST_ASSERT_FALSE(pGauge->init(NULL));
    TEST_ASSERT(pGauge->init(gpI2C));
}

// Test that a temperature reading can be performed
void test_temperature() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    int8_t temperatureC;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getTemperature(&temperatureC));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->getTemperature(&temperatureC));
    printf ("Temperature %d C.\n", temperatureC);
    // Range check
    TEST_ASSERT_INT8_WITHIN(MIN_TEMPERATURE_READING_C, MAX_TEMPERATURE_READING_C, temperatureC);
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getTemperature(NULL));
}

// Test that a voltage reading can be performed
void test_voltage() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    uint16_t voltageMV;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getVoltage(&voltageMV));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->getVoltage(&voltageMV));
    printf ("Voltage %.3f V.\n", ((float) voltageMV) / 1000);
    // Range check
    TEST_ASSERT_UINT16_WITHIN(MAX_VOLTAGE_READING_MV, 0, voltageMV);
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getVoltage(NULL));
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
