#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "lipo_charger_bq24295.h"

using namespace utest::v1;

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(P0_27, P0_28);

// Test that the BQ24295 LiPo charger can be initialised
void test_init() {
    LipoChargerBq24295 * pLipoCharger = new LipoChargerBq24295();
    
    TEST_ASSERT_FALSE(pLipoCharger->init(NULL));
    TEST_ASSERT(pLipoCharger->init(gpI2C));
}

// Test that we can read the charger state from the BQ24295 LiPo charger
void test_charger_state() {
    LipoChargerBq24295 * pLipoCharger = new LipoChargerBq24295();
    LipoChargerBq24295::ChargerState chargerState = LipoChargerBq24295::CHARGER_STATE_UNKNOWN;
    
    // Call should fail if the LiPo charger has not been initialised
    chargerState = pLipoCharger->getChargerState();
    TEST_ASSERT(chargerState == LipoChargerBq24295::CHARGER_STATE_UNKNOWN);
    
    // Normal case
    TEST_ASSERT(pLipoCharger->init(gpI2C));
    chargerState = pLipoCharger->getChargerState();
    printf ("Charger state is %d.\n", chargerState);
    // Range check
    TEST_ASSERT(chargerState != LipoChargerBq24295::CHARGER_STATE_UNKNOWN);
    TEST_ASSERT(chargerState < LipoChargerBq24295::MAX_NUM_CHARGE_STATES);
}

// Test that we can read whether external power is present or not
// according to the BQ24295 LiPo charger
void test_external_power_present() {
    LipoChargerBq24295 * pLipoCharger = new LipoChargerBq24295();
    
    // Call should return false if the LiPo charger has not been initialised
    TEST_ASSERT_FALSE(pLipoCharger->isExternalPowerPresent());
    
    // Normal case: must return true as the USB cable is plugged
    // in when running these tests
    TEST_ASSERT(pLipoCharger->init(gpI2C));
    TEST_ASSERT(pLipoCharger->isExternalPowerPresent());
}

// Test that we can read the charger fault from the BQ24295 LiPo charger
void test_charger_fault() {
    LipoChargerBq24295 * pLipoCharger = new LipoChargerBq24295();
    LipoChargerBq24295::ChargerFault chargerFault = LipoChargerBq24295::CHARGER_FAULT_UNKNOWN;
    
    // Call should fail if the LiPo charger has not been initialised
    chargerFault = pLipoCharger->getChargerFault();
    TEST_ASSERT(chargerFault == LipoChargerBq24295::CHARGER_FAULT_UNKNOWN);
    
    // Normal case
    TEST_ASSERT(pLipoCharger->init(gpI2C));
    chargerFault = pLipoCharger->getChargerFault();
    printf ("Charger fault is %d.\n", chargerFault);
    // Range check
    TEST_ASSERT(chargerFault != LipoChargerBq24295::CHARGER_FAULT_UNKNOWN);
    TEST_ASSERT(chargerFault < LipoChargerBq24295::MAX_NUM_CHARGER_FAULTS);
}

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(10, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Testing initialisation", test_init),
    Case("Testing charger state read", test_charger_state),
    Case("Testing external power presence", test_external_power_present),
    Case("Testing charger fault read", test_charger_fault),
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
