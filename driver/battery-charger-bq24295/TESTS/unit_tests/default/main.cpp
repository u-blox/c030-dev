#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_charger_bq24295.h"

using namespace utest::v1;

// Minimum and maximum numbers
#define MAX_INPUT_VOLTAGE_LIMIT_MV  5080
#define MIN_INPUT_VOLTAGE_LIMIT_MV  3880
#define MAX_INPUT_CURRENT_LIMIT_MA  3000
#define MIN_INPUT_CURRENT_LIMIT_MA  100

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
    TEST_ASSERT(chargerState < BatteryChargerBq24295::MAX_NUM_CHARGER_STATES);
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

// Test that we can read and change the input voltage and current limits
void test_input_limits() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t voltageOriginal;
    int32_t currentOriginal;
    bool enabledOriginal;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getInputVoltageLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->getInputCurrentLimit(&getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&voltageOriginal));
    TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(&currentOriginal));
    enabledOriginal = pBatteryCharger->areInputLimitsEnabled();

    // Voltage and current beyond the limits
    setValue = MIN_INPUT_VOLTAGE_LIMIT_MV - 1;
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(setValue));
    setValue = MAX_INPUT_VOLTAGE_LIMIT_MV + 1;
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(setValue));
    setValue = MIN_INPUT_CURRENT_LIMIT_MA - 1;
    TEST_ASSERT_FALSE(pBatteryCharger->setInputCurrentLimit(setValue));
    setValue = MAX_INPUT_CURRENT_LIMIT_MA + 1;
    TEST_ASSERT_FALSE(pBatteryCharger->setInputCurrentLimit(setValue));
    
    // Voltage and current at the limits
    TEST_ASSERT(pBatteryCharger->setInputVoltageLimit(MIN_INPUT_VOLTAGE_LIMIT_MV));
    TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_INPUT_VOLTAGE_LIMIT_MV, getValue);
    TEST_ASSERT(pBatteryCharger->setInputVoltageLimit(MAX_INPUT_VOLTAGE_LIMIT_MV));
    TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_INPUT_VOLTAGE_LIMIT_MV, getValue);
    TEST_ASSERT(pBatteryCharger->setInputCurrentLimit(MIN_INPUT_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_INPUT_CURRENT_LIMIT_MA, getValue);
    TEST_ASSERT(pBatteryCharger->setInputCurrentLimit(MAX_INPUT_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_INPUT_CURRENT_LIMIT_MA, getValue);

    // The voltage should be less than 80 mV lower than the set value when it is read back.
    // 50 iterations should be sufficient
    for (uint32_t x = 0; x < 50; x++) {
        setValue = MIN_INPUT_VOLTAGE_LIMIT_MV + rand() % (MAX_INPUT_VOLTAGE_LIMIT_MV - MIN_INPUT_VOLTAGE_LIMIT_MV + 1);
        TEST_ASSERT(pBatteryCharger->setInputVoltageLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&getValue));
        TEST_ASSERT((getValue > setValue - 80) && (getValue <= setValue));
        TEST_ASSERT((getValue >= MIN_INPUT_VOLTAGE_LIMIT_MV) && (getValue <= MAX_INPUT_VOLTAGE_LIMIT_MV));
    }

    // The current value read back should always be less than or equal to the set value when it is read back.
    // 50 iterations should be sufficient
    for (uint32_t x = 0; x < 50; x++) {
        setValue = MIN_INPUT_CURRENT_LIMIT_MA + rand() % (MAX_INPUT_CURRENT_LIMIT_MA - MIN_INPUT_CURRENT_LIMIT_MA + 1);
        TEST_ASSERT(pBatteryCharger->setInputCurrentLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(&getValue));
        TEST_ASSERT(getValue <= setValue);
        TEST_ASSERT((getValue >= MIN_INPUT_CURRENT_LIMIT_MA) && (getValue <= MAX_INPUT_CURRENT_LIMIT_MA));
    }
    
    // Enable and disable the limits
    TEST_ASSERT(pBatteryCharger->enableInputLimits());
    TEST_ASSERT(pBatteryCharger->areInputLimitsEnabled());
    TEST_ASSERT(pBatteryCharger->disableInputLimits());
    TEST_ASSERT_FALSE(pBatteryCharger->areInputLimitsEnabled());
    TEST_ASSERT(pBatteryCharger->enableInputLimits());
    TEST_ASSERT(pBatteryCharger->areInputLimitsEnabled());
    
    // Parameters can be NULL
    TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(NULL));
    TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(NULL));

    // Put the initial values back when we're done
    TEST_ASSERT(pBatteryCharger->setInputVoltageLimit(voltageOriginal));
    TEST_ASSERT(pBatteryCharger->setInputCurrentLimit(currentOriginal));
    if (enabledOriginal) {
        pBatteryCharger->enableInputLimits();
    } else {
        pBatteryCharger->disableInputLimits();
    }
}

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(120, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Initialisation", test_init),
    Case("Charger state read", test_charger_state),
    Case("External power presence", test_external_power_present),
    Case("Charger fault read", test_charger_fault),
    Case("Input limits", test_input_limits),
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
