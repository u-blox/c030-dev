/* mbed Microcontroller Library
 * Copyright (c) 2017 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file ltc2943.cpp
 * This file defines the API to the Linear Technology LTC2943 battery gauge chip.
 */

// Define this to print debug information
#define DEBUG_LTC2943

#include <mbed.h>
#include <battery_gauge_ltc2943.h>

#ifdef DEBUG_LTC2943
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

// How long to wait for one ADC read of temperature, voltage and current
// to be performed
#define ADC_READ_WAIT_MS 100

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

// Read two bytes from an address
bool BatteryGaugeLtc2943::getTwoBytes (uint8_t registerAddress, uint16_t *pBytes)
{
    bool success = false;
    char data[3];

    if (gpI2c != NULL) {
        // Send a command to read from registerAddress
        data[0] = registerAddress;
        data[1] = 0;
        data[2] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 2) == 0)) {
            success = true;
            if (pBytes) {
                *pBytes = (((uint16_t) data[1]) << 8) + data[2];
            }
        }
    }

    return success;
}

// Ensure that the ADC of the chip has taken a reading recently.
bool BatteryGaugeLtc2943::makeAdcReading (void)
{
    bool success = false;
    char data[2];
    
    if (gReady && (gpI2c != NULL)){
        // Read the control register
        data[0] = 0x01;  // Address of the control register
        data[1] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            // If the ADC mode, in bits 6 and 7, is zero, set
            // it to 01, which will take a manual reading of voltage,
            // current and temperature and then return them to zero
            if ((data[1] & 0xC0) == 0) {
                data[1] |= 0x80;
                if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                    wait_ms(ADC_READ_WAIT_MS);
                    success = true;
                }
            } else {
                // Readings are already being made, nothing to do here
                success = true;
            }
        }
    }
    
    return success;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
BatteryGaugeLtc2943::BatteryGaugeLtc2943(void)
{
    gpI2c = NULL;
    gReady = false;
}

/// Destructor.
BatteryGaugeLtc2943::~BatteryGaugeLtc2943(void)
{
}

/// Initialise ourselves.
bool BatteryGaugeLtc2943::init (I2C * pI2c, uint32_t rSenseMOhm, uint8_t address)
{
    bool success = false;
    char data[2];

    gpI2c = pI2c;
    gAddress = address << 1;

    if (gpI2c != NULL) {
        // Do a dummy read of the control register
        data[0] = 0x01;  // Address of the control register
        data[1] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            // Any response is good
            success = true;
            gReady = true;
        }
   }

    if (success)
    {
        // TODO: set up Prescaler
    }
    
#ifdef DEBUG_LTC2943
    if (success) {
        printf("BatteryGaugeLtc2943 (I2C 0x%02x): handler initialised (control register contents 0x%02x).\r\n", gAddress >> 1, data[1]);
    } else {
        printf("BatteryGaugeLtc2943 (I2C 0x%02x): device NOT found.\r\n", gAddress >> 1);
    }
#endif

    return success;
}

/// Get the temperature of the chip.
bool BatteryGaugeLtc2943::getTemperature (int8_t *pTemperatureC)
{
    bool success = false;
    int32_t temperature = 0;
    uint16_t data;

    if (gReady && (gpI2c != NULL)){
        if (makeAdcReading()) {
            // Read from the temperature register address
            if (getTwoBytes (0x14, &data)) {
                success = true;

                // From the data sheet the temperature (in Kelvin) is
                // T = 510 * data / 0xFFFF
                temperature = ((uint32_t) data * 510) >> 16;
                temperature -= 273;

                if (pTemperatureC) {
                    *pTemperatureC = (int8_t) temperature;
                }

#ifdef DEBUG_LTC2943
                printf("BatteryGaugeLtc2943 (I2C 0x%02x): temperature registers report 0x%04x, so %d C.\r\n", gAddress >> 1, data, (int) temperature);
#endif
            }
        }
    }

    return success;
}

/// Get the Voltage of the battery.
bool BatteryGaugeLtc2943::getVoltage (uint16_t *pVoltageMV)
{
    bool success = false;
    uint16_t data;
    uint16_t voltage = 0;

    if (gReady && (gpI2c != NULL)){
        if (makeAdcReading()) {
            // Read from the voltage register address
            if (getTwoBytes (0x08, &data)) {
                success = true;

                // From the data sheet the voltage (in mV) is
                // V = 23.6 * data / 0xFFFF
                voltage = ((uint32_t) data * 236 / 10) >> 16;
                
                if (pVoltageMV) {
                    *pVoltageMV = data;
                }

#ifdef DEBUG_LTC2943
                printf("BatteryGaugeLtc2943 (I2C 0x%02x): voltage registers report 0x%04x, giving a voltage of %.3f V.\r\n", gAddress >> 1, data, ((float) voltage) / 1000);
#endif
            }
        }
    }
        
    return success;
}

/// Get the remaining battery capacity.
bool BatteryGaugeLtc2943::getRemainingCapacity (uint32_t *pCapacityMAh)
{
    bool success = false;
    uint16_t data = 0;

    return success;
}

/// Get the battery percentage remaining
bool BatteryGaugeLtc2943::getRemainingPercentage (uint16_t *pBatteryPercent)
{
    bool success = false;
    uint16_t data = 0;

    return success;
}

// End Of File
