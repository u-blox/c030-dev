#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_charger_bq24295.h"

using namespace utest::v1;

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

// Minimum and maximum numbers
#define MAX_INPUT_VOLTAGE_LIMIT_MV  5080
#define MIN_INPUT_VOLTAGE_LIMIT_MV  3880
#define MAX_INPUT_CURRENT_LIMIT_MA  3000
#define MIN_INPUT_CURRENT_LIMIT_MA  100
#define MAX_SYSTEM_VOLTAGE_MV  3700
#define MIN_SYSTEM_VOLTAGE_MV  3000
#define MAX_FAST_CHARGING_CURRENT_LIMIT_MA 3008
#define MIN_FAST_CHARGING_CURRENT_LIMIT_MA 512
#define MAX_PRECHARGING_CURRENT_LIMIT_MA 2048
#define MIN_PRECHARGING_CURRENT_LIMIT_MA 128
#define MAX_CHARGING_TERMINATION_CURRENT_MA 2048
#define MIN_CHARGING_TERMINATION_CURRENT_MA 128
#define MAX_CHARGING_VOLTAGE_LIMIT_MV  4400
#define MIN_CHARGING_VOLTAGE_LIMIT_MV  3504
#define MIN_BOOST_VOLTAGE_MV 4550
#define MAX_BOOST_VOLTAGE_MV 5510

#ifndef NUM_RAND_ITERATIONS
// The number of iterations of random input values in various tests
#define NUM_RAND_ITERATIONS 50
#endif

#ifndef PIN_I2C_SDA
// Default for C030 board
#define PIN_I2C_SDA PB_7
#endif

#ifndef PIN_I2C_SDC
// Default for C030 board
#define PIN_I2C_SDC PB_6
#endif

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

#ifdef TARGET_UBLOX_C027
// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
#endif
// I2C interface
I2C * gpI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SDC);

// ----------------------------------------------------------------
// TESTS
// ----------------------------------------------------------------

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
// NOTE: if this test fails, make sure you haven't got an actual fault. Better
// to reset the chip/board entirely before running these tests so that one
// doesn't occur.
void test_charger_fault() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    char bitmap = (char) BatteryChargerBq24295::CHARGER_FAULT_NONE;
    
    // Call should return no faults if the battery charger has not been initialised
    bitmap = pBatteryCharger->getChargerFaults();
    TEST_ASSERT_EQUAL_INT8((char) BatteryChargerBq24295::CHARGER_FAULT_NONE, bitmap);
    
    // Normal case
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    bitmap = pBatteryCharger->getChargerFaults();
    printf ("Charger fault is 0x%02x.\n", bitmap);
    // Should be just the watchdog as we are in host mode and are not servicing the watchdog
    // however the chip seems to return battery over-voltage sometimes when it is fully charged
    // so allow this through also
    // TODO: find a way to test other faults
    TEST_ASSERT((bitmap & ~(BatteryChargerBq24295::CHARGER_FAULT_WATCHDOG_EXPIRED | BatteryChargerBq24295::CHARGER_FAULT_BATTERY_OVER_VOLTAGE)) == 0);
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
    TEST_ASSERT_FALSE(pBatteryCharger->getInputCurrentLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->getInputCurrentLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->enableInputLimits());
    TEST_ASSERT_FALSE(pBatteryCharger->disableInputLimits());
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&voltageOriginal));
    TEST_ASSERT(pBatteryCharger->getInputCurrentLimit(&currentOriginal));
    enabledOriginal = pBatteryCharger->areInputLimitsEnabled();

    // Voltage and current beyond the limits
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(MIN_INPUT_VOLTAGE_LIMIT_MV - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setInputVoltageLimit(MAX_INPUT_VOLTAGE_LIMIT_MV + 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setInputCurrentLimit(MIN_INPUT_CURRENT_LIMIT_MA - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setInputCurrentLimit(MAX_INPUT_CURRENT_LIMIT_MA + 1));
    
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

    // The voltage limit read back should not be more than 80 mV below the value requested
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_INPUT_VOLTAGE_LIMIT_MV + rand() % (MAX_INPUT_VOLTAGE_LIMIT_MV - MIN_INPUT_VOLTAGE_LIMIT_MV + 1);
        TEST_ASSERT(pBatteryCharger->setInputVoltageLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getInputVoltageLimit(&getValue));
        TEST_ASSERT((getValue > setValue - 80) && (getValue <= setValue));
        TEST_ASSERT((getValue >= MIN_INPUT_VOLTAGE_LIMIT_MV) && (getValue <= MAX_INPUT_VOLTAGE_LIMIT_MV));
    }

    // The current limit read back should always be less than or equal to the set value
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
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

// Test that we enable and disable OTG and normal charging
void test_charging_enable() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    bool otgEnabled;
    bool chargingEnabled;
    
    // Call should fail if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->enableOtg());
    TEST_ASSERT_FALSE(pBatteryCharger->disableOtg());
    TEST_ASSERT_FALSE(pBatteryCharger->enableCharging());
    TEST_ASSERT_FALSE(pBatteryCharger->disableCharging());
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    otgEnabled = pBatteryCharger->isOtgEnabled();
    chargingEnabled = pBatteryCharger->isChargingEnabled();

    // Enable and disable OTG
    TEST_ASSERT(pBatteryCharger->enableOtg());
    TEST_ASSERT(pBatteryCharger->isOtgEnabled());
    TEST_ASSERT(pBatteryCharger->disableOtg());
    TEST_ASSERT_FALSE(pBatteryCharger->isOtgEnabled());
    TEST_ASSERT(pBatteryCharger->enableOtg());
    TEST_ASSERT(pBatteryCharger->isOtgEnabled());
    
    // Enable and disable charging
    TEST_ASSERT(pBatteryCharger->enableCharging());
    TEST_ASSERT(pBatteryCharger->isChargingEnabled());
    TEST_ASSERT(pBatteryCharger->disableCharging());
    TEST_ASSERT_FALSE(pBatteryCharger->isChargingEnabled());
    TEST_ASSERT(pBatteryCharger->enableCharging());
    TEST_ASSERT(pBatteryCharger->isChargingEnabled());
    
    // Put the initial values back when we're done
    if (otgEnabled) {
        pBatteryCharger->enableOtg();
    } else {
        pBatteryCharger->disableOtg();
    }
    if (chargingEnabled) {
        pBatteryCharger->enableCharging();
    } else {
        pBatteryCharger->disableCharging();
    }
}

// Test that we can read and change the system voltage
void test_system_voltage() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t voltageOriginal;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getSystemVoltage(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setSystemVoltage(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    TEST_ASSERT(pBatteryCharger->getSystemVoltage(&voltageOriginal));

    // Beyond the limits
    TEST_ASSERT_FALSE(pBatteryCharger->setSystemVoltage(MIN_SYSTEM_VOLTAGE_MV - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setSystemVoltage(MAX_SYSTEM_VOLTAGE_MV + 1));
    
    // At the limits
    TEST_ASSERT(pBatteryCharger->setSystemVoltage(MIN_SYSTEM_VOLTAGE_MV));
    TEST_ASSERT(pBatteryCharger->getSystemVoltage(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_SYSTEM_VOLTAGE_MV, getValue);
    TEST_ASSERT(pBatteryCharger->setSystemVoltage(MAX_SYSTEM_VOLTAGE_MV));
    TEST_ASSERT(pBatteryCharger->getSystemVoltage(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_SYSTEM_VOLTAGE_MV, getValue);

    // The voltage read back should be at least the value requested and
    // not more than 100 mV greater
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_SYSTEM_VOLTAGE_MV + rand() % (MAX_SYSTEM_VOLTAGE_MV - MIN_SYSTEM_VOLTAGE_MV + 1);
        TEST_ASSERT(pBatteryCharger->setSystemVoltage(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getSystemVoltage(&getValue));
        TEST_ASSERT((getValue < setValue + 100) && (getValue >= setValue));
        TEST_ASSERT((getValue >= MIN_SYSTEM_VOLTAGE_MV) && (getValue <= MAX_SYSTEM_VOLTAGE_MV));
    }

    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getSystemVoltage(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setSystemVoltage(voltageOriginal));
}

// Test that we can read and change the fast charging current limits
void test_fast_charging_current_limits() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t currentOriginal;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getFastChargingCurrentLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingCurrentLimit(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    TEST_ASSERT(pBatteryCharger->getFastChargingCurrentLimit(&currentOriginal));

    // Beyond the limits
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingCurrentLimit(MIN_FAST_CHARGING_CURRENT_LIMIT_MA - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingCurrentLimit(MAX_FAST_CHARGING_CURRENT_LIMIT_MA + 1));
    
    // At the limits
    TEST_ASSERT(pBatteryCharger->setFastChargingCurrentLimit(MIN_FAST_CHARGING_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getFastChargingCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_FAST_CHARGING_CURRENT_LIMIT_MA, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingCurrentLimit(MAX_FAST_CHARGING_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getFastChargingCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_FAST_CHARGING_CURRENT_LIMIT_MA, getValue);

    // The current limit read back should not be more than 64 mA below the value requested
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_FAST_CHARGING_CURRENT_LIMIT_MA + rand() % (MAX_FAST_CHARGING_CURRENT_LIMIT_MA - MIN_FAST_CHARGING_CURRENT_LIMIT_MA + 1);
        TEST_ASSERT(pBatteryCharger->setFastChargingCurrentLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getFastChargingCurrentLimit(&getValue));
        TEST_ASSERT(getValue <= setValue);
        TEST_ASSERT((getValue >= MIN_FAST_CHARGING_CURRENT_LIMIT_MA) && (getValue <= MAX_FAST_CHARGING_CURRENT_LIMIT_MA));
    }
    
    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getFastChargingCurrentLimit(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setFastChargingCurrentLimit(currentOriginal));
}

// Test that we can enable and disable the ICGH/IPRECH margin
void test_icgh_iprech_margin() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    bool marginEnabled;
    
    // Call should fail if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->isIcghIprechMarginEnabled());
    TEST_ASSERT_FALSE(pBatteryCharger->enableIcghIprechMargin());
    TEST_ASSERT_FALSE(pBatteryCharger->disableIcghIprechMargin());
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    marginEnabled = pBatteryCharger->isIcghIprechMarginEnabled();

    // Enable and disable the margin
    TEST_ASSERT(pBatteryCharger->enableIcghIprechMargin());
    TEST_ASSERT(pBatteryCharger->isIcghIprechMarginEnabled());
    TEST_ASSERT(pBatteryCharger->disableIcghIprechMargin());
    TEST_ASSERT_FALSE(pBatteryCharger->isIcghIprechMarginEnabled());
    TEST_ASSERT(pBatteryCharger->enableIcghIprechMargin());
    TEST_ASSERT(pBatteryCharger->isIcghIprechMarginEnabled());
    
    // Put the initial value back when we're done
    if (marginEnabled) {
        pBatteryCharger->enableIcghIprechMargin();
    } else {
        pBatteryCharger->disableIcghIprechMargin();
    }
}

// Test that we can read and change the pre-charging current limits
void test_precharging_current_limits() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t currentOriginal;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getPrechargingCurrentLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setPrechargingCurrentLimit(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(&currentOriginal));

    // Beyond the limits
    TEST_ASSERT_FALSE(pBatteryCharger->setPrechargingCurrentLimit(MIN_PRECHARGING_CURRENT_LIMIT_MA - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setPrechargingCurrentLimit(MAX_PRECHARGING_CURRENT_LIMIT_MA + 1));
    
    // At the limits
    TEST_ASSERT(pBatteryCharger->setPrechargingCurrentLimit(MIN_PRECHARGING_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_PRECHARGING_CURRENT_LIMIT_MA, getValue);
    TEST_ASSERT(pBatteryCharger->setPrechargingCurrentLimit(MAX_PRECHARGING_CURRENT_LIMIT_MA));
    TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_PRECHARGING_CURRENT_LIMIT_MA, getValue);

    // The current limit read back should not be more than 128 mA below the value requested
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_PRECHARGING_CURRENT_LIMIT_MA + rand() % (MAX_PRECHARGING_CURRENT_LIMIT_MA - MIN_PRECHARGING_CURRENT_LIMIT_MA + 1);
        TEST_ASSERT(pBatteryCharger->setPrechargingCurrentLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(&getValue));
        TEST_ASSERT(getValue <= setValue);
        TEST_ASSERT((getValue >= MIN_PRECHARGING_CURRENT_LIMIT_MA) && (getValue <= MAX_PRECHARGING_CURRENT_LIMIT_MA));
    }
    
    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setPrechargingCurrentLimit(currentOriginal));
}

// Test that we can read, change and enable/disable the charging termination current
void test_charging_termination_current() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t currentOriginal;
    bool terminationEnabled;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getChargingTerminationCurrent(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingTerminationCurrent(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->enableChargingTermination());
    TEST_ASSERT_FALSE(pBatteryCharger->disableChargingTermination());
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(&currentOriginal));
    terminationEnabled = pBatteryCharger->isChargingTerminationEnabled();

    // Beyond the limits
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingTerminationCurrent(MIN_CHARGING_TERMINATION_CURRENT_MA - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingTerminationCurrent(MAX_CHARGING_TERMINATION_CURRENT_MA + 1));
    
    // At the limits
    TEST_ASSERT(pBatteryCharger->setChargingTerminationCurrent(MIN_CHARGING_TERMINATION_CURRENT_MA));
    TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_CHARGING_TERMINATION_CURRENT_MA, getValue);
    TEST_ASSERT(pBatteryCharger->setChargingTerminationCurrent(MAX_CHARGING_TERMINATION_CURRENT_MA));
    TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_CHARGING_TERMINATION_CURRENT_MA, getValue);

    // The current limit read back should not be more than 128 mA below the value requested
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_CHARGING_TERMINATION_CURRENT_MA + rand() % (MAX_CHARGING_TERMINATION_CURRENT_MA - MIN_CHARGING_TERMINATION_CURRENT_MA + 1);
        TEST_ASSERT(pBatteryCharger->setChargingTerminationCurrent(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(&getValue));
        TEST_ASSERT(getValue <= setValue);
        TEST_ASSERT((getValue >= MIN_CHARGING_TERMINATION_CURRENT_MA) && (getValue <= MAX_CHARGING_TERMINATION_CURRENT_MA));
    }
    
    // Enable and disable the margin
    TEST_ASSERT(pBatteryCharger->enableChargingTermination());
    TEST_ASSERT(pBatteryCharger->isChargingTerminationEnabled());
    TEST_ASSERT(pBatteryCharger->disableChargingTermination());
    TEST_ASSERT_FALSE(pBatteryCharger->isChargingTerminationEnabled());
    TEST_ASSERT(pBatteryCharger->enableChargingTermination());
    TEST_ASSERT(pBatteryCharger->isChargingTerminationEnabled());
    
    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setChargingTerminationCurrent(currentOriginal));
    if (terminationEnabled) {
        pBatteryCharger->enableChargingTermination();
    } else {
        pBatteryCharger->disableChargingTermination();
    }
}

// Test that we can read and change the various charging voltage limits
void test_charging_voltage_limits() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t chargingOriginal;
    int32_t fastChargingOriginal;
    int32_t rechargingOriginal;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getChargingVoltageLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingVoltageLimit(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingVoltageThreshold(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setRechargingVoltageThreshold(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    TEST_ASSERT(pBatteryCharger->getChargingVoltageLimit(&chargingOriginal));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&fastChargingOriginal));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&rechargingOriginal));

    // Beyond the limits for the charging voltage
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingVoltageLimit(MIN_CHARGING_VOLTAGE_LIMIT_MV - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setChargingVoltageLimit(MAX_CHARGING_VOLTAGE_LIMIT_MV + 1));
    
    // At the limits for the charging voltage
    TEST_ASSERT(pBatteryCharger->setChargingVoltageLimit(MIN_CHARGING_VOLTAGE_LIMIT_MV));
    TEST_ASSERT(pBatteryCharger->getChargingVoltageLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_CHARGING_VOLTAGE_LIMIT_MV, getValue);
    TEST_ASSERT(pBatteryCharger->setChargingVoltageLimit(MAX_CHARGING_VOLTAGE_LIMIT_MV));
    TEST_ASSERT(pBatteryCharger->getChargingVoltageLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_CHARGING_VOLTAGE_LIMIT_MV, getValue);
    
    // The charging voltage limit read back should not be more than 16 mV below the value requested
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_CHARGING_VOLTAGE_LIMIT_MV + rand() % (MAX_CHARGING_VOLTAGE_LIMIT_MV - MIN_CHARGING_VOLTAGE_LIMIT_MV + 1);
        TEST_ASSERT(pBatteryCharger->setChargingVoltageLimit(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getChargingVoltageLimit(&getValue));
        TEST_ASSERT((getValue > setValue - 16) && (getValue <= setValue));
        TEST_ASSERT((getValue >= MIN_CHARGING_VOLTAGE_LIMIT_MV) && (getValue <= MAX_CHARGING_VOLTAGE_LIMIT_MV));
    }

    // Fast charging threshold is 2.8 V or 3.0 V
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(2799));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(2800, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(2800));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(2800, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(2801));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(3000, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(3000));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(3000, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(3001));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(3000, getValue);
    
    // Recharging threshold is 100 mV or 300 mV
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(99));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(100, getValue);
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(100));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(100, getValue);
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(101));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(300, getValue);
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(300));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(300, getValue);
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(301));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(300, getValue);
    
    // Parameters can be NULL
    TEST_ASSERT(pBatteryCharger->getChargingVoltageLimit(NULL));
    TEST_ASSERT(pBatteryCharger->getFastChargingVoltageThreshold(NULL));
    TEST_ASSERT(pBatteryCharger->getRechargingVoltageThreshold(NULL));

    // Put the initial values back when we're done
    TEST_ASSERT(pBatteryCharger->setChargingVoltageLimit(chargingOriginal));
    TEST_ASSERT(pBatteryCharger->setFastChargingVoltageThreshold(fastChargingOriginal));
    TEST_ASSERT(pBatteryCharger->setRechargingVoltageThreshold(rechargingOriginal));
}

// Test that we can read and change the fast charging safety timer
void test_fast_charging_safety_timer() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t timerOriginal;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingSafetyTimer(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&timerOriginal));

    // Beyond the limit
    TEST_ASSERT_FALSE(pBatteryCharger->setFastChargingSafetyTimer(-1));
    
    // There are permissible values are 0, 5, 8, 12 and 20 so test them and the
    // boundaries around them
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(0));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(0, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(1));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(5, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(4));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(5, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(6));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(5, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(7));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(5, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(8));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(8, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(11));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(8, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(12));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(12, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(19));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(12, getValue);
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(20));
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(&getValue));
    TEST_ASSERT_EQUAL_INT32(20, getValue);

    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getFastChargingSafetyTimer(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setFastChargingSafetyTimer(timerOriginal));
}

// Test setting the boost mode voltage and temperature limits
void test_boost_limits() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t originalVoltage;
    int32_t originalUpperTemperature;
    bool upperTemperatureEnabled;
    int32_t originalLowerTemperature;
    int32_t setValue;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getBoostVoltage(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setBoostVoltage(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setBoostUpperTemperatureLimit(getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->disableBoostUpperTemperatureLimit());
    TEST_ASSERT_FALSE(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setBoostLowerTemperatureLimit(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    TEST_ASSERT(pBatteryCharger->getBoostVoltage(&originalVoltage));
    upperTemperatureEnabled = pBatteryCharger->isBoostUpperTemperatureLimitEnabled();
    if (upperTemperatureEnabled) {
        TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&originalUpperTemperature));
    }
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&originalLowerTemperature));

    // Beyond the limits for the voltage
    TEST_ASSERT_FALSE(pBatteryCharger->setBoostVoltage(MIN_BOOST_VOLTAGE_MV - 1));
    TEST_ASSERT_FALSE(pBatteryCharger->setBoostVoltage(MAX_BOOST_VOLTAGE_MV + 1));
    
    // At the limits for the voltage
    TEST_ASSERT(pBatteryCharger->setBoostVoltage(MIN_BOOST_VOLTAGE_MV));
    TEST_ASSERT(pBatteryCharger->getBoostVoltage(&getValue));
    TEST_ASSERT_EQUAL_INT32(MIN_BOOST_VOLTAGE_MV, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostVoltage(MAX_BOOST_VOLTAGE_MV));
    TEST_ASSERT(pBatteryCharger->getBoostVoltage(&getValue));
    TEST_ASSERT_EQUAL_INT32(MAX_BOOST_VOLTAGE_MV, getValue);
    
    // The voltage read back should be at least the value requested and
    // not more than 64 mV greater
    for (uint32_t x = 0; x < NUM_RAND_ITERATIONS; x++) {
        setValue = MIN_BOOST_VOLTAGE_MV + rand() % (MAX_BOOST_VOLTAGE_MV - MIN_BOOST_VOLTAGE_MV + 1);
        TEST_ASSERT(pBatteryCharger->setBoostVoltage(setValue));
        getValue = -1;
        TEST_ASSERT(pBatteryCharger->getBoostVoltage(&getValue));
        TEST_ASSERT((getValue < setValue + 64) && (getValue >= setValue));
        TEST_ASSERT((getValue >= MIN_BOOST_VOLTAGE_MV) && (getValue <= MAX_BOOST_VOLTAGE_MV));
    }

    // Disable the upper temperature limit and check that the get function returns false
    TEST_ASSERT(pBatteryCharger->disableBoostUpperTemperatureLimit());
    TEST_ASSERT_FALSE(pBatteryCharger->isBoostUpperTemperatureLimitEnabled());
    TEST_ASSERT_FALSE(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    
    // The boost upper temperature limit is either 55, 60 or 65
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(-1));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(55, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(0));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(55, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(59));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(55, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(60));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(60, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(64));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(60, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(65));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(65, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(100));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(65, getValue);
    
    // The boost lower temperature is either -10 or -20
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(1));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-10, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(0));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-10, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(-0));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-10, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(-10));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-10, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(-11));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-20, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(-20));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-20, getValue);
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(-100));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(&getValue));
    TEST_ASSERT_EQUAL_INT32(-20, getValue);
    
    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getBoostVoltage(NULL));
    TEST_ASSERT(pBatteryCharger->getBoostUpperTemperatureLimit(NULL));
    TEST_ASSERT(pBatteryCharger->getBoostLowerTemperatureLimit(NULL));

    // Put the initial values back when we're done
    TEST_ASSERT(pBatteryCharger->setBoostVoltage(originalVoltage));    
    if (upperTemperatureEnabled) {
        TEST_ASSERT(pBatteryCharger->setBoostUpperTemperatureLimit(originalUpperTemperature));
    } else {
        TEST_ASSERT(pBatteryCharger->disableBoostUpperTemperatureLimit());
    }
    TEST_ASSERT(pBatteryCharger->setBoostLowerTemperatureLimit(originalLowerTemperature));    
}

// Test setting the chip's thermal regulation threshold
void test_chip_thermal_regulation_threshold() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    int32_t originalTemperature;
    int32_t getValue;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->setChipThermalRegulationThreshold(getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&originalTemperature));

    // The thermal regulation threshold is one of 60C, 80C, 100C or 120C
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(-1));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(60, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(0));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(60, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(79));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(60, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(80));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(80, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(99));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(80, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(100));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(100, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(101));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(100, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(119));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(100, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(120));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(120, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(121));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(120, getValue);
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(200));
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(&getValue));
    TEST_ASSERT_EQUAL_INT32(120, getValue);

    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->getChipThermalRegulationThreshold(NULL));

    // Put the initial value back when we're done
    TEST_ASSERT(pBatteryCharger->setChipThermalRegulationThreshold(originalTemperature));    
}

// Test that we can enable and disable shipping mode
void test_shipping_mode() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    bool shippingModeEnabled;
    
    // Call should fail if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->enableShippingMode());
    TEST_ASSERT_FALSE(pBatteryCharger->disableShippingMode());
    TEST_ASSERT_FALSE(pBatteryCharger->isShippingModeEnabled());
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial values
    shippingModeEnabled = pBatteryCharger->isShippingModeEnabled();

    // Enable and disable shipping mode
    TEST_ASSERT(pBatteryCharger->enableShippingMode());
    TEST_ASSERT(pBatteryCharger->isShippingModeEnabled());
    TEST_ASSERT(pBatteryCharger->disableShippingMode());
    TEST_ASSERT_FALSE(pBatteryCharger->isShippingModeEnabled());
    TEST_ASSERT(pBatteryCharger->enableShippingMode());
    TEST_ASSERT(pBatteryCharger->isShippingModeEnabled());
    
    // Put the initial value back when we're done
    if (shippingModeEnabled) {
        pBatteryCharger->enableShippingMode();
    } else {
        pBatteryCharger->disableShippingMode();
    }
}

// Test the advanced functions
void test_advanced() {
    BatteryChargerBq24295 * pBatteryCharger = new BatteryChargerBq24295();
    char originalValue;
    uint8_t address = 0x03; // REG03, the pre-charge/termination current control register
    char getValue;
    int32_t precharge;
    int32_t termination;
    
    // Calls should return false if the battery charger has not been initialised
    TEST_ASSERT_FALSE(pBatteryCharger->advancedGet(address, &getValue));
    TEST_ASSERT_FALSE(pBatteryCharger->advancedSet(address, getValue));
    
    // Initialise the battery charger
    TEST_ASSERT(pBatteryCharger->init(gpI2C));
    
    // Save the initial value from address
    TEST_ASSERT(pBatteryCharger->advancedGet(address, &originalValue));

    // REG03 contains the pre-charge current limit in the upper four bits
    // and the termination current in the lower four bits, so we can read
    // those and work out what the raw register value is.
    TEST_ASSERT(pBatteryCharger->getPrechargingCurrentLimit(&precharge));
    TEST_ASSERT(pBatteryCharger->getChargingTerminationCurrent(&termination));
    
    precharge = (precharge - MIN_PRECHARGING_CURRENT_LIMIT_MA) / 128;
    termination = (termination - MIN_CHARGING_TERMINATION_CURRENT_MA) / 128;
    TEST_ASSERT_EQUAL_INT8(((precharge << 4) | (termination & 0x0f)), originalValue);
    
    // Now write something else to the register and check that it changes
    TEST_ASSERT(pBatteryCharger->advancedSet(address, 0x01));
    TEST_ASSERT(pBatteryCharger->advancedGet(address, &getValue));
    TEST_ASSERT_EQUAL_INT8(0x01, getValue);

    // Parameter can be NULL
    TEST_ASSERT(pBatteryCharger->advancedGet(address, NULL));

    // Put the initial value back now that we're done
    TEST_ASSERT(pBatteryCharger->advancedSet(address, originalValue));    
}

// ----------------------------------------------------------------
// TEST ENVIRONMENT
// ----------------------------------------------------------------

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea, timeout is long enough to run these tests with
    // DEBUG_BQ24295 defined
    GREENTEA_SETUP(240, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Initialisation", test_init),
    Case("Charger state read", test_charger_state),
    Case("External power presence", test_external_power_present),
    Case("Charger fault", test_charger_fault),
    Case("Input limits", test_input_limits),
    Case("Charging enable", test_charging_enable),
    Case("System voltage", test_system_voltage),
    Case("Fast charging current limits", test_fast_charging_current_limits),
    Case("Icgh/Iprech margin", test_icgh_iprech_margin),
    Case("Pre-charging current limits", test_precharging_current_limits),
    Case("Charging termination current", test_charging_termination_current),
    Case("Charging voltage limits", test_charging_voltage_limits),
    Case("Fast charging safety timer", test_fast_charging_safety_timer),
    Case("Boost limits", test_boost_limits),
    Case("Chip thermal regulation threshold", test_chip_thermal_regulation_threshold),
    Case("Shipping mode", test_shipping_mode),
    Case("Advanced", test_advanced)
};

Specification specification(test_setup, cases);

// ----------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------

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
