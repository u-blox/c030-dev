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

/// Read two bytes from an address.
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

/// Ensure that the ADC of the chip has taken a reading recently.
bool BatteryGaugeLtc2943::makeAdcReading (void)
{
    bool success = false;
    char storedControlRegister;
    char data[2];
    
    if (gReady && (gpI2c != NULL)){
        // Read the control register
        data[0] = 0x01;  // Address of the control register
        data[1] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            storedControlRegister = data[1];
            // If the ADC mode, in bits 6 and 7, is zero, set
            // it to 01, which will take a manual reading of voltage,
            // current and temperature and then return them to zero.
            // Also make sure that the power-down bit is not set.
            if ((data[1] & 0xC0) == 0) {
                data[1] |= 0x40;
                data[1] &= ~0x01;
                if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                    wait_ms(ADC_READ_WAIT_MS);
                    // Now set the shutdown bit to ensure lowest power state
                    data[1] = storedControlRegister | 0x01;
                    if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                        success = true;
                    }
                }
            } else {
                // Readings are already being made, nothing to do here
                success = true;
            }
        }
    }
    
    return success;
}

/// Convert a 16 bit register reading into a temperature reading in C.
int32_t BatteryGaugeLtc2943::registerToTemperatureC (uint16_t data)
{
    // From the data sheet the temperature (in Kelvin) is
    // T = 510 * data / 0xFFFF    
    return (((uint32_t) data * 510) >> 16) - 273;;
}

/// Convert a 16 bit register reading into a voltage reading in mV.
int32_t BatteryGaugeLtc2943::registerToVoltageMV (uint16_t data)
{
    // From the data sheet the voltage (in mV) is
    // V = 23600 * data / 0xFFFF
    return (((int32_t) data) * 23600) >> 16;
}

/// Convert a 16 bit register reading into a current reading in mA.
int32_t BatteryGaugeLtc2943::registerToCurrentMA (uint16_t data, int32_t rSenseMOhm)
{
    // From the data sheet, max current corresponds to 0xFFFF
    // (which is 60 mV across rSense) while min (most negative)
    // current corresponds to 0 (which is -60 mV across rSense).    
    return (((int32_t) (data - 0x7FFF)) * 60 >> 15) / rSenseMOhm;;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS: initialisation and monitoring
//----------------------------------------------------------------

/// Constructor.
BatteryGaugeLtc2943::BatteryGaugeLtc2943(void)
{
    gpI2c = NULL;
    gReady = false;
    gRemainingChargeKnown = false;
}

/// Destructor.
BatteryGaugeLtc2943::~BatteryGaugeLtc2943(void)
{
}

/// Initialise ourselves.
bool BatteryGaugeLtc2943::init (I2C * pI2c, int32_t rSenseMOhm, uint8_t address, int32_t prescaler, Alcc alcc)
{
    char data[2];

    gpI2c = pI2c;
    gAddress = address << 1;
    gRSenseMOhm = rSenseMOhm;
    
    MBED_ASSERT(alcc < MAX_NUM_ALCCS);

    if (gpI2c != NULL) {
        // Set up the prescaler, ALCC and set everything to lowest power state for now
        data[0] = 0x01;  // Address of the control register
        
        // Bits 7:6 top two bits zero == ADC asleep
        // Bits 5:3 coded prescaler value
        // Bits 2:1 ALCC mode (10 for Alert Output, 01 for Charge Complete Input, 00 for Off
        // Bit  0   1 to power down analogue components
        
        data[1] = 0x01;
        data[1] |= alcc << 1;
        switch (prescaler) {
            case 1:
                // Coded prescaler value is zero, nothing to do
            break;
            case 4:
                data[1] |= 1 << 3;
            break;
            case 16:
                data[1] |= 2 << 3;
            break;
            case 64:
                data[1] |= 3 << 3;
            break;
            case 256:
                data[1] |= 4 << 3;
            break;
            case 1024:
                data[1] |= 5 << 3;
            break;
            case 4096:
                data[1] |= 6 << 3;
            break;
            default:
                MBED_ASSERT(false);
            break;
        }
        
        // Now write to the control register
        if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
            gReady = true;
        }
   }

#ifdef DEBUG_LTC2943
    if (gReady) {
        printf("BatteryGaugeLtc2943 (I2C 0x%02x): handler initialised (control register set to 0x%02x).\n", gAddress >> 1, data[1]);
    } else {
        printf("BatteryGaugeLtc2943 (I2C 0x%02x): device NOT found.\r\n", gAddress >> 1);
    }
#endif

    return gReady;
}

// Set whether battery capacity monitoring is on or off
bool BatteryGaugeLtc2943::setMonitor (bool onNotOff, bool isSlow)
{
    bool success = false;
    char data[2];
    
    if (gReady && (gpI2c != NULL)){
        // Read the control register
        data[0] = 0x01;  // Address of the control register
        data[1] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {

            if (onNotOff) {
                // Set the ADC mode in bits 6 and 7 to 11 or, if
                // isSlow is true, to 10 (in which case a measurement
                // is only performed every ten seconds).
                data[1] |= 0xC0;
                if (isSlow) {
                    data[1] &= ~0x40;
                }
                // Also make sure that the power-down bit is not set.
                data[1] &= ~0x01;
            } else {
                // Set the ADC mode to 00 and the power down bit to 1
                data[1] &= ~0xC0;
                data[1] |= 0x01;
            }
            
            // Write the new value to the control register
            if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                success = true;
            }
        }
    }

    return success;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS: getting readings
//----------------------------------------------------------------

/// Get the temperature of the chip.
bool BatteryGaugeLtc2943::getTemperature (int32_t *pTemperatureC)
{
    bool success = false;
    uint16_t data;

    if (gReady && (gpI2c != NULL)){
        if (makeAdcReading ()) {
            // Read from the temperature register address
            if (getTwoBytes (0x14, &data)) {
                success = true;

                if (pTemperatureC) {
                    *pTemperatureC = registerToTemperatureC (data);
                }

#ifdef DEBUG_LTC2943
                printf("BatteryGaugeLtc2943 (I2C 0x%02x): temperature registers report 0x%04x, so %d C.\n",
                       gAddress >> 1, data, registerToTemperatureC (data));
#endif
            }
        }
    }

    return success;
}


/// Get the voltage of the battery.
bool BatteryGaugeLtc2943::getVoltage (int32_t *pVoltageMV)
{
    bool success = false;
    uint16_t data;

    if (gReady && (gpI2c != NULL)){
        if (makeAdcReading()) {
            // Read from the voltage register address
            if (getTwoBytes (0x08, &data)) {
                success = true;

                if (pVoltageMV) {
                    *pVoltageMV = registerToVoltageMV (data);
                }

#ifdef DEBUG_LTC2943
                printf("BatteryGaugeLtc2943 (I2C 0x%02x): voltage registers report 0x%04x, giving a voltage of %.3f V.\n",
                       gAddress >> 1, data, (float) registerToVoltageMV (data) / 1000);
#endif
            }
        }
    }
        
    return success;
}

/// Get the current flowing through rSense.
bool BatteryGaugeLtc2943::getCurrent (int32_t *pCurrentMA)
{
    bool success = false;
    uint16_t data;

    if (gReady && (gpI2c != NULL)){
        if (makeAdcReading()) {
            // Read from the current register address
            if (getTwoBytes (0x0E, &data)) {
                success = true;

                if (pCurrentMA) {
                    *pCurrentMA = registerToCurrentMA (data, gRSenseMOhm);;
                }

#ifdef DEBUG_LTC2943
                printf("BatteryGaugeLtc2943 (I2C 0x%02x): current registers report 0x%04x, giving a current of %.3f A.\n",
                       gAddress >> 1, data, (float) registerToCurrentMA (data, gRSenseMOhm) / 1000);
#endif
            }
        }
    }
        
    return success;
}

/// Tell the LTC2943 chip that charging is complete.
bool setChargingComplete (void)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get the remaining battery capacity.
bool BatteryGaugeLtc2943::getRemainingCapacity (int32_t *pCapacityMAh)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get the battery percentage remaining.
bool BatteryGaugeLtc2943::getRemainingPercentage (int32_t *pBatteryPercent)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get the reason for an alert.
BatteryGaugeLtc2943::Alert getAlertReason (void)
{
    BatteryGaugeLtc2943::Alert alert = BatteryGaugeLtc2943::ALERT_NONE;
    
    // TODO complete

    return alert;
}
        
//----------------------------------------------------------------
// PUBLIC FUNCTIONS: setting/getting/clearing thresholds
//----------------------------------------------------------------

/// Set temperature alert upper threshold.
bool setTemperatureHigh(int32_t temperatureC)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// get temperature alert upper threshold.
bool getTemperatureHigh(int32_t *pTemperatureC)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear temperature alert upper threshold.
bool clearTemperatureHigh()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set temperature alert lower threshold.
bool setTemperatureLow(int32_t temperatureC)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// get temperature alert LOWER threshold.
bool getTemperatureLow(int32_t *pTemperatureC)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear temperature alert lower threshold.
bool clearTemperatureLow()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set voltage alert upper threshold.
bool setVoltageHigh(int32_t voltageMV)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get voltage alert upper threshold.
bool getVoltageHigh(int32_t *pVoltageMV)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear voltage alert upper threshold.
bool clearVoltageHigh()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set voltage alert lower threshold.
bool setVoltageLow(int32_t voltageMV)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get voltage alert lower threshold.
bool getVoltageLow(int32_t *pVoltageMV)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear voltage alert lower threshold.
bool clearVoltageLow()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set current alert upper threshold.
bool setCurrentHigh(int32_t currentMA)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get current alert upper threshold.
bool getCurrentHigh(int32_t *pCurrentMA)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear current alert upper threshold.
bool clearCurrentHigh()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set current alert lower threshold.
bool setCurrentLow(int32_t currentMA)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get current alert lower threshold.
bool getCurrentLow(int32_t *pCurrentMA)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear current alert lower threshold.
bool clearCurrentLow()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set capacity alert upper threshold.
bool setCapacityHigh(int32_t capacityMAh)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get capacity alert upper threshold.
bool getCapacityHigh(int32_t *pCapacityMAh)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear capacity alert upper threshold.
bool clearCapacityHigh()
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Set capacity alert lower threshold.
bool setCapacityLow(int32_t capacityMAh)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Get capacity alert lower threshold.
bool getCapacityLow(int32_t *pCapacityMAh)
{
    bool success = false;
    
    // TODO complete

    return success;
}

/// Clear capacity alert lower threshold.
bool clearCapacityLow()
{
    bool success = false;
    
    // TODO complete

    return success;
}

// End Of File
