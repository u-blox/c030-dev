#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_charger_bq24295.h"

using namespace utest::v1;

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(P0_27, P0_28);

// Test that the BQ24295 battery charger can be initialised
void test_init() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    
    TEST_ASSERT_FALSE(pBatteryCharger->init(NULL));
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
}

// Test that we can read the charger state from the BQ24295 battery charger
void test_charger_state() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    BatteryChargerBq24295::ChargerState chargerState = BatteryChargerBq24295::CHARGER_STATE_UNKNOWN;
    
    // Call should fail if the battery charger has not been initialised
    chargerState = pBatteryCharger->getChargerState();
    TEST_ASSERT(chargerState == BatteryChargerBq24295::CHARGER_STATE_UNKNOWN);
    
    // Normal case
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    chargerState = pBatteryCharger->getChargerState();
    printf ("Charger state is %d.\n", chargerState);
    // Range check
    TEST_ASSERT(chargerState != BatteryChargerBq24295::CHARGER_STATE_UNKNOWN);
    TEST_ASSERT(chargerState < BatteryChargerBq24295::MAX_NUM_CHARGE_STATES);
}

// Test that we can read whether external power is present or not
// according to the BQ24295 battery charger
void test_external_power_present() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    
    // Call should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->isExternalPowerPresent());
    
    // Normal case: must return true as the USB cable is plugged
    // in when running these tests
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    TEST_ASSERT(pBatteryCharger->isExternalPowerPresent());
}

// Test that we can read the charger fault from the BQ24295 battery charger
void test_charger_fault() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    BatteryChargerBq24295::ChargerFault chargerFault = BatteryChargerBq24295::CHARGER_FAULT_UNKNOWN;
    
    // Call should fail if the battery charger has not been initialised
    chargerFault = pBatteryCharger->getChargerFault();
    TEST_ASSERT(chargerFault == BatteryChargerBq24295::CHARGER_FAULT_UNKNOWN);
    
    // Normal case
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    chargerFault = pBatteryCharger->getChargerFault();
    printf ("Charger fault is %d.\n", chargerFault);
    // Range check
    TEST_ASSERT(chargerFault != BatteryChargerBq24295::CHARGER_FAULT_UNKNOWN);
    TEST_ASSERT(chargerFault < BatteryChargerBq24295::MAX_NUM_CHARGER_FAULTS);
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
    Case("Charger state read", test_charger_state),
    Case("External power presence", test_external_power_present),
    Case("Charger fault read", test_charger_fault),
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
