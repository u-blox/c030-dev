#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "low_power.h"

using namespace utest::v1;

// Setup the test environment
utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea with a timeout
    GREENTEA_SETUP(30, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Test cases
Case cases[] = {
    Case("Blah", test_blah)
};

Specification specification(test_setup, cases);

// Entry point into the tests
int main() {    
    bool success = false;
    
    success = !Harness::run(specification);
    
    return success;
}

// End Of File
