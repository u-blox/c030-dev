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
 * @file bq27441.cpp
 * This file defines the API to the TI BQ27441 battery gauge chip.
 */

// Define this to print debug information
//#define DEBUG_BQ27441

#include <mbed.h>
#include <battery_gauge_bq27441.h>

#ifdef DEBUG_BQ27441
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

// How many loops to wait for a configuration update to be permitted
#define CONFIG_UPDATE_LOOPS 100

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

// Read two bytes from an address
bool BatteryGaugeBq27441::getTwoBytes (uint8_t registerAddress, uint16_t *pBytes)
{
    bool success = false;
    char data[3];

    if (gpI2c != NULL) {
        // Send a command to read from registerAddress
        data[0] = registerAddress;
        data[1] = 0;
        data[2] = 0;

        if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
            (gpI2c->read(gAddress, &(data[1]), 2) == 0)) {
            success = true;
            if (pBytes) {
                *pBytes = (((uint16_t) data[2]) << 8) + data[1];
            }
        }
    }

    return success;
}

// Compute the checksum of the current set of block data
uint8_t BatteryGaugeBq27441::computeChecksum()
{
    char data[32 + 1];
    uint8_t checkSum = 0;
    uint8_t x;

    data[0] = 0x40;

    if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
        (gpI2c->read(gAddress, &(data[1]), 32) == 0)) {
        for (x = 0; x < 32; x++) {
            checkSum += data[x];
        }

        checkSum = 255 - checkSum;
    }

    return checkSum;
}

// Write data of a given length and class ID to a given offset.  This code taken from
// section 3.1 of the SLUUAC9A application technical reference manual with hints from:
// https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library/blob/master/src/SparkFunBQ27441.cpp.
bool BatteryGaugeBq27441::writeExtendedData(uint8_t classId, uint8_t offset, const char * pData, uint8_t len)
{
    bool success = false;
    char data[3];
    uint16_t answer;
    uint32_t count = 0;

    if ((gpI2c != NULL) && (len <= 32)) {
        // Send Config Update (command 0x13)
        data[0] = 0;
        data[1] = 0x13;
        data[2] = 0;

        if (gpI2c->write(gAddress, &(data[0]), 3) == 0) {
            // Wait for CONFIGUPDATE to be set in Flags register (bit 4)
            count = 0;
            do {
                getTwoBytes(0x06, &answer);
                count++;
                wait_ms(1);
            } while (((answer & (1 << 4)) == 0) && (count < CONFIG_UPDATE_LOOPS));

            if ((answer & (1 << 4)) != 0) {
                // Enable Block Data Control (0x61)
                data[0] = 0x61;
                data[1] = 0;

                if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                    // Write Class ID using Data Block Class (0x3E)
                    data[0] = 0x3E;
                    data[1] = classId;

                    if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                        // Write offset using Block Offset (0x3F)
                        data[0] = 0x3F;
                        data[1] = offset / 32;

                        if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                            // Compute the existing block checksum and then read it (why!?)
                            computeChecksum();
                            data[0] = 0x60;
                            data[1] = 0;

                            if ((gpI2c->write(gAddress, &(data[0]), 1) == 0) &&
                                (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {
                                // Write the new data to the block data area at 0x40 plus the offset (plus the Block Offset already written)
                                data[0] = 0x40 + (offset % 32);
                                
                                if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
                                    (gpI2c->write(gAddress, pData, len) == 0)) {
                                    // Compute the new checksum and write it
                                    data[0] = 0x60;
                                    data[1] = computeChecksum();

                                    if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                                        // Exit config mode with command 0x43
                                        data[0] = 0;
                                        data[1] = 0x43;
                                        data[2] = 0;

                                        if (gpI2c->write(gAddress, &(data[0]), 3) == 0) {
                                            // Finally, wait for CONFIGUPDATE to be unset in Flags register (bit 4)
                                            count = 0;
                                            do {
                                                getTwoBytes(0x06, &answer);
                                                count++;
                                                wait_ms(1);
                                            } while (((answer & (1 << 4)) != 0) && (count < CONFIG_UPDATE_LOOPS));

                                            if ((answer & (1 << 4)) == 0) {
                                                success = true;
                                            } else {
#ifdef DEBUG_BQ27441
                                                printf("BatteryGaugeBq27441 (I2C 0x%02x): Flags register didn't show config update flag unset in time.\r\n", gAddress >> 1);
#endif        
                                            }
                                        } else {
#ifdef DEBUG_BQ27441
                                            printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write config update exit after update.\r\n", gAddress >> 1);
#endif        
                                        }
                                    } else {
#ifdef DEBUG_BQ27441
                                        printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write new checksum (0x%02x) during config update.\r\n", gAddress >> 1, data[1]);
#endif
                                    }
                                } else {
#ifdef DEBUG_BQ27441
                                    printf("BatteryGaugeBq27441 (I2C 0x%02x): couldn't write all %d bytes during config update.\r\n", gAddress >> 1, len);
#endif
                                }
                            } else {
#ifdef DEBUG_BQ27441
                                printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to read Block Data Checksum for config update.\r\n", gAddress >> 1);
#endif
                            }
                        } else {
#ifdef DEBUG_BQ27441
                            printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write Block Offset (%d) for config update.\r\n", gAddress >> 1, data[1]);
#endif
                        }
                    } else {
#ifdef DEBUG_BQ27441
                        printf("BatteryGaugeBq27441 (I2C 0x%02x): unable set Class ID (0x%02x) for config update.\r\n", gAddress >> 1, classId);
#endif
                    }
                } else {
#ifdef DEBUG_BQ27441
                    printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to set Block Data Control for config update.\r\n", gAddress >> 1);
#endif
                }
            } else {
#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): Flags register didn't show config update flag set in time.\r\n", gAddress >> 1);
#endif
            }
        } else {
#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write to control register for config update.\r\n", gAddress >> 1);
#endif
        }
    }

    return success;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
BatteryGaugeBq27441::BatteryGaugeBq27441(void)
{
    gpI2c = NULL;
    gReady = false;
}

/// Destructor.
BatteryGaugeBq27441::~BatteryGaugeBq27441(void)
{
}

/// Initialise ourselves.
bool BatteryGaugeBq27441::init (I2C * pI2c, uint32_t rSenseMOhm, uint8_t address)
{
    bool success = false;
    char data[3];
    uint16_t answer;

    gpI2c = pI2c;
    gAddress = address << 1;

    if (gpI2c != NULL) {
        // Send a control command to read the firmware version
        data[0] = 0x00;  // Set address to first register for control
        data[1] = 0x02;  // First byte of FW_VERSION sub-command (0x02)
        data[2] = 0x00;  // Second byte of FW_VERSION sub-command (0x00) (register address will auto-increment)

        if (gpI2c->write(gAddress, &(data[0]), 3) == 0) {
            if (getTwoBytes (0, &answer)) {
                // The expected response is 0x0109
                if (((answer >> 8) == 0x01) && ((answer & 0xFF) == 0x09)) {
                    success = true;
                    gReady = true;
                }
#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): read 0x%04x as FW_VERSION, expected 0x0109.\r\n", gAddress >> 1, answer);
#endif
            }
        }
    }

    if (success)
    {
        // TODO: set up RSense
        
        // Read from the OpConfig register address
        if (getTwoBytes (0x3A, &answer)) {
            // If BATLOWEN (bit 3) is not set, set it
            if ((answer & (1 << 3)) != 0) {
                // Set BATLOWEN (bit 3) in OpConfig register (class ID 0x20)
                data[0] = answer >> 8;
                data[1] = answer & 0xFF;
                // success = writeExtendedData(0x20, 0, &(data[0]), 2);
            }

#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): OpConfig register contents set to 0x%04x.\r\n", gAddress >> 1, answer);
#endif
        } else {
            success = false;
        }
    }

#ifdef DEBUG_BQ27441
    if (success) {
        printf("BatteryGaugeBq27441 (I2C 0x%02x): handler initialised.\r\n", gAddress >> 1);
    } else {
        printf("BatteryGaugeBq27441 (I2C 0x%02x): device NOT found.\r\n", gAddress >> 1);
    }
#endif

    return success;
}

/// Get the temperature of the chip.
bool BatteryGaugeBq27441::getTemperature (int8_t *pTemperatureC)
{
    bool success = false;
    int32_t temperature = 0;
    uint16_t data;

    if (gReady && (gpI2c != NULL)){
        // Read from the temperature register address
        if (getTwoBytes (0x02, &data)) {
            success = true;

            // The answer is in units of 0.1 K, so convert to C
            temperature = data;
            temperature /= 10;
            temperature -= 273;

            if (pTemperatureC) {
                *pTemperatureC = (int8_t) temperature;
            }

#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): chip temperature %.1f K, so %d C.\r\n", gAddress >> 1, ((float) data) / 10, (int) temperature);
#endif
        }
    }

    return success;
}

/// Get the Voltage of the battery.
bool BatteryGaugeBq27441::getVoltage (uint16_t *pVoltageMV)
{
    bool success = false;
    uint16_t voltage = 0;

    if (gReady && (gpI2c != NULL)) {
        // Read from the voltage register address
        if (getTwoBytes (0x04, &voltage)) {
            success = true;

            // The answer is in mV
            if (pVoltageMV) {
                *pVoltageMV = voltage;
            }

#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): battery voltage %.3f V.\r\n", gAddress >> 1, ((float) voltage) / 1000);
#endif
        }
    }

    return success;
}

/// Get the remaining battery capacity.
bool BatteryGaugeBq27441::getRemainingCapacity (uint32_t *pCapacityMAh)
{
    bool success = false;
    uint16_t data = 0;

    if (gReady && (gpI2c != NULL)) {
        // Read from the RemainingCapacity register address
        
        if (getTwoBytes (0x0C, &data)) {
            success = true;

            // The answer is in mAh
            if (pCapacityMAh) {
                *pCapacityMAh = data;
            }

#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): remaining battery capacity %u mAh.\r\n", gAddress >> 1, data);
#endif
        }
    }

    return success;
}

/// Get the battery percentage remaining
bool BatteryGaugeBq27441::getRemainingPercentage (uint16_t *pBatteryPercent)
{
    bool success = false;
    uint16_t data = 0;

    if (gReady && (gpI2c != NULL)) {
        // Read from the StateOfCharge register address
        if (getTwoBytes (0x1C, &data)) {
            success = true;

            if (pBatteryPercent) {
                *pBatteryPercent = data;
            }

#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): remaining battery percentage %u%%.\r\n", gAddress >> 1, data);
#endif
        }
    }

    return success;
}

// End Of File
