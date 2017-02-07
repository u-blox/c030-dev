#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "lipo_gauge_bq27441.h"

using namespace utest::v1;

// This required only for UTM board
static DigitalOut i2CPullUpBar(P1_1, 0);
// Objects
LipoGaugeBq27441 * pLipoGauge = new LipoGaugeBq27441();
I2C * pI2C = new I2C(P0_27, P0_28);

// Test that the BQ27441 LiPo gauge can be initialised
void test_init() {
    TEST_ASSERT(pLipoGauge->init(pI2C));
}

utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(10, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Testing initialisation", test_init),
};

Specification specification(test_setup, cases);

// Entry point into the tests
int main() {    
    bool success = false;
    
    if ((pI2C != NULL) && (pLipoGauge != NULL)) {        
        success = !Harness::run(specification);
    } else {
        printf ("Unable to instantiate objects.\n");
    }
    
    return success;
}