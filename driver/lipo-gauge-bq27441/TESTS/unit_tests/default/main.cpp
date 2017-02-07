#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "lipo_gauge_bq27441.h"

using namespace utest::v1;

// Pick some sensible minimum and maximum numbers
#define MAX_VOLTAGE_READING 12 // Bigger than a 3 cell LiPo
#define MAX_TEMPERATURE_READING_C  50
#define MIN_TEMPERATURE_READING_C -10

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(P0_27, P0_28);

// Test that the BQ27441 LiPo gauge can be initialised
void test_init() {
    LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
    
    TEST_ASSERT_FALSE(pLipoGauge->init(NULL));
    TEST_ASSERT(pLipoGauge->init(gpI2C));
}

// Test that a temperature reading can be performed
void test_temperature() {
    LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
    int8_t temperature;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pLipoGauge->getTemperature(&temperature));
    
    TEST_ASSERT(pLipoGauge->init(gpI2C));
    TEST_ASSERT(pLipoGauge->getTemperature(&temperature));
    TEST_ASSERT(temperature >= MIN_TEMPERATURE_READING_C);
    TEST_ASSERT(temperature <= MAX_TEMPERATURE_READING_C);
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pLipoGauge->getTemperature(NULL));
}

// Test that a voltage reading can be performed
void test_voltage() {
    LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
    uint16_t voltage;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pLipoGauge->getVoltage(&voltage));
    
    TEST_ASSERT(pLipoGauge->init(gpI2C));
    TEST_ASSERT(pLipoGauge->getVoltage(&voltage));
    TEST_ASSERT(voltage >= MAX_VOLTAGE_READING);
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pLipoGauge->getVoltage(NULL));
}

// Test that a remaining capacity reading can be performed
void test_remaining_capacity() {
    LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
    uint32_t capacityMAh;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pLipoGauge->getRemainingCapacity(&capacityMAh));
    
    TEST_ASSERT(pLipoGauge->init(gpI2C));
    TEST_ASSERT(pLipoGauge->getRemainingCapacity(&capacityMAh));

    // The parameter is allowed to be NULL
    TEST_ASSERT(pLipoGauge->getRemainingCapacity(NULL));
}

// Test that a remaining percentage reading can be performed
void test_remaining_percentage() {
    LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
    uint16_t batteryPercent;
    
    // Call should fail if the LiPo gauge has not been initialised
    TEST_ASSERT_FALSE(pLipoGauge->getRemainingPercentage(&batteryPercent));
    
    TEST_ASSERT(pLipoGauge->init(gpI2C));
    TEST_ASSERT(pLipoGauge->getRemainingPercentage(&batteryPercent));
    TEST_ASSERT(batteryPercent <= 100);

    // The parameter is allowed to be NULL
    TEST_ASSERT(pLipoGauge->getRemainingPercentage(NULL));
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
    Case("Testing temperature reading", test_temperature),
    Case("Testing voltage reading", test_voltage),
    Case("Testing remaining capacity reading", test_remaining_capacity),
    Case("Testing remaining percentage reading", test_remaining_percentage),
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
