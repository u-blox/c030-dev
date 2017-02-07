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
 * @file bq24295.cpp
 * This file defines the API to the TI BQ24295 battery charging chip.
 */

// Define this to print debug information
//#define DEBUG_BQ24295

#include <mbed.h>
#include <lipo_charger_bq24295.h>

#ifdef DEBUG_BQ24295
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
LipoChargerBq24295::LipoChargerBq24295(void)
{
    gpI2c = NULL;
    gReady = false;
}

/// Destructor.
LipoChargerBq24295::~LipoChargerBq24295(void)
{
}

/// Initialise ourselves.
bool LipoChargerBq24295::init (I2C * pI2c, uint8_t address)
{
    bool success = false;
    char data[2];

    gpI2c = pI2c;
    gAddress = address << 1;

    if (gpI2c != NULL) {
        // Read the revision status register
        data[0] = 0x0a;  // Revision status register gAddress
        data[1] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            // The expected response is 0xc0
            if (data[1] == 0xc0) {
                success = true;
                gReady = true;
            }
#ifdef DEBUG_BQ24295
            printf("LipoChargerBq24295 (I2C 0x%02x): read 0x%02x from register 0x%02x, expected 0xc0.\r\n", gAddress >> 1, data[1], data[0]);
#endif
        }
    }

#ifdef DEBUG_BQ24295
    if (success) {
        printf("LipoChargerBq24295 (I2C 0x%02x): handler initialised.\r\n", gAddress >> 1);
    } else {
        printf("LipoChargerBq24295 (I2C 0x%02x): device NOT found.\r\n", gAddress >> 1);
    }
#endif

    return success;
}

/// Get the charge state.
LipoChargerBq24295::ChargerState LipoChargerBq24295::getChargerState(void)
{
    LipoChargerBq24295::ChargerState chargerState = CHARGER_STATE_UNKNOWN;
    char data[4];

    if (gReady && (gpI2c != NULL)) {
        // Read the power-on configuration register
        data[0] = 0x01;  // Power-on configuration register gAddress
        data[1] = 0;
        if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            // Read the system status register
            data[2] = 0x08;  // System status register gAddress
            data[3] = 0;
            if ((gpI2c->write(gAddress, &(data[2]), 1) == 0) &&
                (gpI2c->read(gAddress, &(data[3]), 1) == 0)) {
                // Now have the power-on configuration register
                // in data[1] and the system status register in
                // data[3]
                // Check the charge enable bit
                if ((data[1] & (1 << 4)) == 0) {
                    chargerState = CHARGER_STATE_DISABLED;
#ifdef DEBUG_BQ24295
                    printf("LipoChargerBq24295 (I2C 0x%02x): charging is disabled.\r\n", gAddress >> 1);
#endif
                } else {
                    // Charger is not disabled, so see if we have
                    // external power (bit 3)
                    if ((data[3] & 0x04) == 0) {
                        chargerState = CHARGER_STATE_NO_EXTERNAL_POWER;
#ifdef DEBUG_BQ24295
                        printf("LipoChargerBq24295 (I2C 0x%02x): no external power.\r\n", gAddress >> 1);
#endif
                    } else {
                        // Have power, so see how we're cooking (bits 4 & 5)
                        switch ((data[3] >> 4) & 0x03) {
                            case 0:
                                chargerState = CHARGER_STATE_NOT_CHARGING;
#ifdef DEBUG_BQ24295
                                printf("LipoChargerBq24295 (I2C 0x%02x): not charging.\r\n", gAddress >> 1);
#endif
                            break;
                            case 1:
                                chargerState = CHARGER_STATE_PRECHARGE;
#ifdef DEBUG_BQ24295
                                printf("LipoChargerBq24295 (I2C 0x%02x): pre-charge.\r\n", gAddress >> 1);
#endif
                            break;
                            case 2:
                                chargerState = CHARGER_STATE_FAST_CHARGE;
#ifdef DEBUG_BQ24295
                                printf("LipoChargerBq24295 (I2C 0x%02x): fast charge.\r\n", gAddress >> 1);
#endif
                            break;
                            case 3:
                                chargerState = CHARGER_STATE_COMPLETE;
#ifdef DEBUG_BQ24295
                                printf("LipoChargerBq24295 (I2C 0x%02x): charging complete.\r\n", gAddress >> 1);
#endif
                            break;
                            default:
                            break;
                        }
                    }
                }
            }
        }
    }

    return chargerState;
}

/// Get whether external power is present or not.
bool LipoChargerBq24295::isExternalPowerPresent(void)
{
    bool externalPowerPresent = false;
    char data[2];

    if (gReady && (gpI2c != NULL)) {
        // Read the system status register
        data[0] = 0x08;  // System status register address
        data[1] = 0;
        if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
           // See if we have external power (bit 3)
            if ((data[1] & 0x04) != 0) {
                externalPowerPresent = true;
            }

#ifdef DEBUG_BQ24295
            if (externalPowerPresent) {
                printf("LipoChargerBq24295 (I2C 0x%02x): external power is present.\r\n", gAddress >> 1);
            } else {
                printf("LipoChargerBq24295 (I2C 0x%02x): external power is NOT present.\r\n", gAddress >> 1);
            }
#endif                
        }
    }

    return externalPowerPresent;
}

/// Get the charger fault status.
LipoChargerBq24295::ChargerFault LipoChargerBq24295::getChargerFault(void)
{
    LipoChargerBq24295::ChargerFault chargerFault = CHARGER_FAULT_UNKNOWN;
    char data[2];

    if (gReady && (gpI2c != NULL)) {
        // Read the fault register
        data[0] = 0x09;  // Fault register gAddress
        data[1] = 0;
        if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
            chargerFault = CHARGER_FAULT_NONE;
            if (data[1] & (1 << 8)) {
                chargerFault = CHARGER_FAULT_WATCHDOG_EXPIRED;
#ifdef DEBUG_BQ24295
                printf("LipoChargerBq24295 (I2C 0x%02x): watchdog expired.\r\n", gAddress >> 1);
#endif
            } else if (data[1] & (1 << 7)) {
                chargerFault = CHARGER_FAULT_BOOST;
#ifdef DEBUG_BQ24295
                printf("LipoChargerBq24295 (I2C 0x%02x): boost fault.\r\n", gAddress >> 1);
#endif
            } else if (data[1] & 0x30) {
                switch ((data[1] >> 4) & 0x03) {
                    case 1:
                        chargerFault = CHARGER_FAULT_INPUT_FAULT;
#ifdef DEBUG_BQ24295
                        printf("LipoChargerBq24295 (I2C 0x%02x): input fault.\r\n", gAddress >> 1);
#endif
                    break;
                    case 2:
                        chargerFault = CHARGER_FAULT_THERMAL_SHUTDOWN;
#ifdef DEBUG_BQ24295
                        printf("LipoChargerBq24295 (I2C 0x%02x): thermal shutdown.\r\n", gAddress >> 1);
#endif
                    break;
                    case 3:
                        chargerFault = CHARGER_FAULT_CHARGE_TIMER_EXPIRED;
#ifdef DEBUG_BQ24295
                        printf("LipoChargerBq24295 (I2C 0x%02x): charge timer expired.\r\n", gAddress >> 1);
#endif
                    break;
                    default:
                    break;
                }
            } else if (data[1] & (1 << 3)) {
                chargerFault = CHARGER_FAULT_BATTERY_OVER_VOLTAGE;
#ifdef DEBUG_BQ24295
                printf("LipoChargerBq24295 (I2C 0x%02x): battery over-voltage fault.\r\n", gAddress >> 1);
#endif
            } else if (data[1] & (1 << 1)) {
                chargerFault = CHARGER_FAULT_THERMISTOR_TOO_COLD;
#ifdef DEBUG_BQ24295
                printf("LipoChargerBq24295 (I2C 0x%02x): thermistor too cold.\r\n", gAddress >> 1);
#endif
            } else if (data[1] & (1 << 0)) {
                chargerFault = CHARGER_FAULT_THERMISTOR_TOO_HOT;
#ifdef DEBUG_BQ24295
                printf("LipoChargerBq24295 (I2C 0x%02x): thermistor too hot.\r\n", gAddress >> 1);
#endif
            }
        }
    }

    return chargerFault;
}

// End Of File
