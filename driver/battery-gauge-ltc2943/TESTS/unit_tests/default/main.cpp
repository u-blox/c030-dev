#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_gauge_ltc2943.h"

using namespace utest::v1;

// Pick some sensible minimum and maximum numbers
#define MAX_TEMPERATURE_READING_C  80
#define MIN_TEMPERATURE_READING_C -20
#define MIN_VOLTAGE_READING_MV     0
#define MAX_VOLTAGE_READING_MV     12000 // Bigger than a 3 cell LiPo
#define MAX_CURRENT_READING_MA     2000
#define MIN_CURRENT_READING_MA    -2000

#ifndef RSENSE_MOHM
// RSense (in mOhm) on UTM board
#define RSENSE_MOHM 68
#endif

#ifndef BATTERY_CAPACITY_MAH
#define BATTERY_CAPACITY_MAH 2300
#endif

#ifndef PIN_I2C_SDA
// Default for UTM board
#define PIN_I2C_SDA P0_27
#endif

#ifndef PIN_I2C_SDC
// Default for UTM board
#define PIN_I2C_SDC P0_28
#endif

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SDC);

// Test that the LTC2943 battery gauge can be initialised
void test_init() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    
    TEST_ASSERT_FALSE(pBatteryGauge->init(NULL, RSENSE_MOHM));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    
    // Different prescaler values
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 1));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 4));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 16));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 64));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 256));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 1024));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 4096));

    // Different ALCC values    
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 4096, BatteryGaugeLtc2943::ALCC_OFF));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 1024, BatteryGaugeLtc2943::ALCC_CHARGE_COMPLETE_INPUT));
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 1, BatteryGaugeLtc2943::ALCC_ALERT_OUTPUT));
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

// Test that we can set charging complete and read back the capacity
// TODO: find a way to test non-100% numbers
void test_charging() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    int32_t remainingChargeMAH;
    int32_t remainingPercentage;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->setChargingComplete(BATTERY_CAPACITY_MAH));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->setChargingComplete(BATTERY_CAPACITY_MAH));
    // Read the remaining capacity value back    
    TEST_ASSERT(pBatteryGauge->getRemainingCharge(&remainingChargeMAH));
    // Check that it is the same as what we said
    TEST_ASSERT(remainingChargeMAH == BATTERY_CAPACITY_MAH);
    // Check that we are now at 100%
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(&remainingPercentage));
    TEST_ASSERT(remainingPercentage == 100);

    // Repeat with some different values
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM / 2));
    TEST_ASSERT(pBatteryGauge->setChargingComplete(BATTERY_CAPACITY_MAH * 2));
    // Read the remaining capacity value back    
    TEST_ASSERT(pBatteryGauge->getRemainingCharge(&remainingChargeMAH));
    // Check that it is the same as what we said
    TEST_ASSERT(remainingChargeMAH == BATTERY_CAPACITY_MAH * 2);
    // Check that we are still at 100%
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(&remainingPercentage));
    TEST_ASSERT(remainingPercentage == 100);

    // Repeat with non-default prescaler
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM, BATTERY_GAUGE_LTC2943_ADDRESS, 1024));
    TEST_ASSERT(pBatteryGauge->setChargingComplete(BATTERY_CAPACITY_MAH));
    // Read the remaining capacity value back    
    TEST_ASSERT(pBatteryGauge->getRemainingCharge(&remainingChargeMAH));
    // Check that it is the same as what we said
    TEST_ASSERT(remainingChargeMAH == BATTERY_CAPACITY_MAH);
    // Check that we are still at 100%
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(&remainingPercentage));
    TEST_ASSERT(remainingPercentage == 100);
}

// Test that the alert reason can be retrieved
// TODO: find a way to test actual useful return values
void test_alert() {
    BatteryGaugeLtc2943 * pBatteryGauge = new BatteryGaugeLtc2943();
    
    // Call should return none if the battery gauge has not been initialised
    TEST_ASSERT(pBatteryGauge->getAlertReason() == BatteryGaugeLtc2943::ALERT_NONE);
    
    // Call also returns none if the battery gauge has been initialised and nothing has happened
    TEST_ASSERT(pBatteryGauge->init(gpI2C, RSENSE_MOHM));
    TEST_ASSERT(pBatteryGauge->getAlertReason() == BatteryGaugeLtc2943::ALERT_NONE);
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
    Case("Charging", test_charging),
    Case("Alert", test_alert),
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
