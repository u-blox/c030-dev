#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "battery_gauge_bq27441.h"

using namespace utest::v1;

// Pick some sensible minimum and maximum numbers
#define MAX_TEMPERATURE_READING_C  80
#define MIN_TEMPERATURE_READING_C -20
#define MIN_VOLTAGE_READING_MV     0
#define MAX_VOLTAGE_READING_MV     12000 // Bigger than a 3 cell LiPo
#define MAX_CURRENT_READING_MA     2000
#define MIN_CURRENT_READING_MA    -2000
#define MIN_CAPACITY_READING_MAH   0
#define MAX_CAPACITY_READING_MAH   30000 // A very big battery indeed

#ifndef PIN_I2C_SDA
// Default for UTM board
#define PIN_I2C_SDA P0_27
#endif

#ifndef PIN_I2C_SDC
// Default for UTM board
#define PIN_I2C_SDC P0_28
#endif

// The maximum size of configuration block
// that we can handle in one go
#define MAX_CONFIG_BLOCK_SIZE 32

// This required only for UTM board
static DigitalOut gI2CPullUpBar(P1_1, 0);
// I2C interface
I2C * gpI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SDC);

// An empty array, so that we can check for emptiness
static const char zeroArray[MAX_CONFIG_BLOCK_SIZE] = {0};

// Print a buffer as a nice hex string
static void printBytesAsHex(const char * pBuf, uint32_t size)
{
    uint32_t x;

    printf (" 0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");
    for (x = 1; x <= size; x++, pBuf++)
    {
        if (x % 16 == 8) {
            printf ("%02x  ", *pBuf);
        } else if (x % 16 == 0) {
            printf ("%02x\n", *pBuf);
        } else {
            printf ("%02x-", *pBuf);
        }
    }
    
    if (x % 16 !=  1) {
        printf("\n");
    }
}

// Test that the BQ27441 battery gauge can be initialised
void test_init() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    
    TEST_ASSERT_FALSE(pBatteryGauge->init(NULL));
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
}

// Test that battery detection can be performed
// TODO: find a way to check that a battery is not detected correctly
void test_battery_detection() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->isBatteryDetected());
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->isBatteryDetected());
}

// Test that a temperature reading can be performed
void test_temperature() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t temperatureC;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getTemperature(&temperatureC));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
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
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->getVoltage(&voltageMV));
    printf ("Voltage %.3f V.\n", ((float) voltageMV) / 1000);
    // Range check
    TEST_ASSERT((voltageMV >= MIN_VOLTAGE_READING_MV) && (voltageMV <= MAX_VOLTAGE_READING_MV));
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getVoltage(NULL));
}

// Test that a current reading can be performed
void test_current() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t currentMA;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getCurrent(&currentMA));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->getCurrent(&currentMA));
    printf ("Current %.3f A.\n", ((float) currentMA) / 1000);
    // Range check
    TEST_ASSERT((currentMA >= MIN_CURRENT_READING_MA) && (currentMA <= MAX_CURRENT_READING_MA));
    
    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getCurrent(NULL));
}

// Test that a remaining capacity reading can be performed
void test_remaining_capacity() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    int32_t capacityMAh;
    
    // Call should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->getRemainingCapacity(&capacityMAh));
    
    // Normal case
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
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
    TEST_ASSERT(pBatteryGauge->init(gpI2C));
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(&batteryPercent));
    printf ("Remaining percentage %d%%.\n", batteryPercent);
    // Range check
    TEST_ASSERT((batteryPercent >= 0) && (batteryPercent <= 100));

    // The parameter is allowed to be NULL
    TEST_ASSERT(pBatteryGauge->getRemainingPercentage(NULL));
}

// Test advanced functions to read/write the configuration of the chip
void test_advanced() {
    BatteryGaugeBq27441 * pBatteryGauge = new BatteryGaugeBq27441();
    uint8_t subClassId = 80; // IT Cfg
    int32_t offset = 0;
    int32_t length = MAX_CONFIG_BLOCK_SIZE - offset;
    char data1[MAX_CONFIG_BLOCK_SIZE];
    uint32_t deadArea1 = 0xdeadbeef;
    char data2[MAX_CONFIG_BLOCK_SIZE];
    uint32_t deadArea2 = 0xdeadbeef;
    uint32_t sealCode = 0;
    int16_t tmp;
    
    // All calls should fail if the battery gauge has not been initialised
    TEST_ASSERT_FALSE(pBatteryGauge->advancedSetConfig(subClassId, offset, length, &(data1[0])));
    TEST_ASSERT_FALSE(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data1[0])));
        
    // Initialise the battery gauge
    TEST_ASSERT(pBatteryGauge->init(gpI2C));

    // Read IT Cfg (total length 79 bytes), starting from 0, into data1
    subClassId = 80;
    memset(&(data1[0]), 0, sizeof (data1));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data1[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data1[0]), length);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea1);
    
    // Read it again, with an offset of 16 bytes, into data2
    offset = 16;
    length = MAX_CONFIG_BLOCK_SIZE - 16;
    memset(&(data2[0]), 0, sizeof (data2));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data2[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data2[0]), length);
    // The second 16 bytes of data1 and the first 16 bytes of data2 should match
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&(data1[16]), &(data2[0]), 16);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea2);

    // Read the next block of IT Cfg into data1
    offset = MAX_CONFIG_BLOCK_SIZE;
    length = MAX_CONFIG_BLOCK_SIZE;
    memset(&(data1[0]), 0, sizeof (data1));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data1[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data1[0]), length);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea1);

    // Read the only the first 16 bytes, from the same offset into IT Cfg, into data2
    length = 16;
    memset(&(data2[0]), 0, sizeof (data2));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data2[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data2[0]), length);
    // The first 16 bytes of data1 and data2 should match
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&(data1[0]), &(data2[0]), length);
    // The remainder of data2 should be zero
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&(zeroArray[0]), &(data2[length]), sizeof(data2) - length);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea2);

    // Read Delta Voltage, two bytes at offset 39 in sub-class State, into data1
    subClassId = 82;
    offset = 39;
    length = 2;
    memset(&(data1[0]), 0, sizeof (data1));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data1[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data1[0]), length);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea1);
    
    // Copy Delta Voltage, change the lower byte and then write it back
    (data1[1])++;
    printf ("Modified data block:\n");
    printBytesAsHex(&(data1[0]), length);
    TEST_ASSERT(pBatteryGauge->advancedSetConfig(subClassId, offset, length, &(data1[0])));
    printf("%d bytes written to subClassID %d, offset %d:\n", length, subClassId, offset);

    // Now read sub-class State back again, but use an offset/length combination that crosses
    // a 32 byte boundary (which should fail)
    subClassId = 82;
    offset = 30;
    length = 11;
    memset(&(data2[0]), 0, sizeof (data2));
    TEST_ASSERT_FALSE(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data2[0])));
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea1);
    
    // Now read it properly and check that the Delta Voltage really is the new value
    subClassId = 82;
    offset = 32;
    length = 9;
    memset(&(data2[0]), 0, sizeof (data2));
    TEST_ASSERT(pBatteryGauge->advancedGetConfig(subClassId, offset, length, &(data2[0])));
    printf("%d bytes received from subClassID %d, offset %d:\n", length, subClassId, offset);
    printBytesAsHex(&(data2[0]), length);
    TEST_ASSERT_EQUAL_UINT32 (0xdeadbeef, deadArea1);
    TEST_ASSERT_EQUAL_UINT32 (data1[0], data2[7]);
    TEST_ASSERT_EQUAL_UINT32 (data1[1], data2[8]);
    
    // Now put Delta Voltage back as it was
    (data2[8])--;
    TEST_ASSERT(pBatteryGauge->advancedSetConfig(subClassId, offset, length, &(data2[0])));
    printf("%d bytes written to subClassID %d, offset %d:\n", length, subClassId, offset);
}

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(20, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Initialisation", test_init),
    Case("Battery detection", test_battery_detection),
    Case("Temperature read", test_temperature),
    Case("Voltage read", test_voltage),
    Case("Current read", test_current),
    Case("Remaining capacity read", test_remaining_capacity),
    Case("Remaining percentage read", test_remaining_percentage),
    Case("Advanced", test_advanced)
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
