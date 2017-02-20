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

/// Define this to print debug information.
#define DEBUG_BQ27441

#include <mbed.h>
#include <battery_gauge_bq27441.h>

#ifdef DEBUG_BQ27441
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

/// How many loops to wait for a configuration update to be permitted.
#define CONFIG_UPDATE_LOOPS 1000

/// How long to wait for all the ADC readings to be performed.
#define ADC_READ_WAIT_MS 1000

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

/// Read two bytes from an address.
// Note: gpI2c should be locked before this is called.
bool BatteryGaugeBq27441::getTwoBytes (uint8_t registerAddress, uint16_t *pBytes)
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
                *pBytes = (((uint16_t) data[2]) << 8) + data[1];
            }
        }
    }

    return success;
}

/// Compute the checksum over a block of data.
uint8_t BatteryGaugeBq27441::computeChecksum(const char * pData)
{
    uint8_t checkSum = 0;
    uint8_t x;

    if (pData != NULL) {
#ifdef DEBUG_BQ27441
        printf ("BatteryGaugeBq27441 (I2C 0x%02x): computing check sum on data block.\n", gAddress >> 1);
        printf (" 0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");
#endif
        for (x = 1; x <= 32; x++) {
            checkSum += *pData;
            
#ifdef DEBUG_BQ27441
            if (x % 16 == 8) {
                printf ("%02x  ", *pData);
            } else if (x % 16 == 0) {
                printf ("%02x\n", *pData);
            } else {
                printf ("%02x-", *pData);
            }
#endif
            pData++;
        }

        checkSum = 0xff - checkSum;
    }

#ifdef DEBUG_BQ27441
    if (x % 16 !=  1) {
        printf("\n");
    }
    
    printf ("BatteryGaugeBq27441 (I2C 0x%02x): check sum is 0x%02x.\n", gAddress >> 1, checkSum);
#endif    
    
    return checkSum;
}

/// Read data of a given length and class ID.
// Note: gpI2c should be locked before this is called.
bool BatteryGaugeBq27441::readExtendedData(uint8_t subClassId, int32_t offset, int32_t length, char * pData, uint32_t sealCode)
{
    bool success = false;
    char block[32];
    char data[3];

    if ((gpI2c != NULL) && (length <= 32) && (pData != NULL)) {            
        
        // The offset + length combination must not cross a 32-byte boundary
        if (offset / 32 == (offset + length - 1) / 32) {
        
            // TODO handle unsealing
            (void)sealCode;
            
            // Enable Block Data Control (0x61)
            data[0] = 0x61;
            data[1] = 0;

            if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                // Write sub-class ID using Data Block Class (0x3e)
                data[0] = 0x3e;
                data[1] = subClassId;

                if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                    // Write offset using Block Offset (0x3f) and then
                    // read the data block
                    data[0] = 0x3f;
                    data[1] = offset / 32;

                    if ((gpI2c->write(gAddress, &(data[0]), 2, true) == 0) &&
                        (gpI2c->read(gAddress, &(block[0]), 32) == 0)) {
                        // Compute the block checksum and then read it
                        data[2] = computeChecksum(&(block[0]));
                        data[0] = 0x60;
                        data[1] = 0;

                        if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
                            (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {

                            // Does the checksum match?
                            if (data[1] == data[2]) {              
                                // If so read the new data from the block data area at 0x40 plus
                                // the offset (plus the Block Offset already written)
                                data[0] = 0x40 + (offset % 32);
                            
                                if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
                                    (gpI2c->read(gAddress, pData, length) == 0)) {
                                    success = true;
                                } else {
#ifdef DEBUG_BQ27441
                                    printf("BatteryGaugeBq27441 (I2C 0x%02x): couldn't read all %d bytes of config.\r", gAddress >> 1, length);
#endif
                                }
                            } else {
#ifdef DEBUG_BQ27441
                                printf("BatteryGaugeBq27441 (I2C 0x%02x): checksum on data block incorrect (expected 0x%02x, received 0x%02x).\n", gAddress >> 1, data[2], data[1]);
#endif
                            }
                        } else {
#ifdef DEBUG_BQ27441
                            printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to read Block Data Checksum for config.\n", gAddress >> 1);
#endif
                        }
                    } else {
#ifdef DEBUG_BQ27441
                        printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write Block Offset (%d), or maybe read the data block, for config.\n", gAddress >> 1, data[1]);
#endif
                    }
                } else {
#ifdef DEBUG_BQ27441
                    printf("BatteryGaugeBq27441 (I2C 0x%02x): unable set sub-class ID (0x%02x) for config.\n", gAddress >> 1, subClassId);
#endif
                }
            } else {
#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to set Block Data Control for config.\n", gAddress >> 1);
#endif
            }
            // TODO handle sealing
        } else {
#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): offset (%d) is in different 32 byte block to offset + length (%d) [length is %d].\n", gAddress >> 1, offset, offset + length, length);
#endif
        }
    }

    return success;
}

/// Write data of a given length and class ID to a given offset.  This code taken from
// section 3.1 of the SLUUAC9A application technical reference manual with hints from:
// https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library/blob/master/src/SparkFunBQ27441.cpp.
// Note: gpI2c should be locked before this is called.
bool BatteryGaugeBq27441::writeExtendedData(uint8_t subClassId, int32_t offset, int32_t length, const char * pData, uint32_t sealCode)
{
    bool success = false;
    char data[3 + 32];
    char block[32];
    uint16_t answer;
    uint32_t count = 0;

    if ((gpI2c != NULL) && (length <= 32) && (pData != NULL)) {

        // The offset + length combination must not cross a 32-byte boundary
        if (offset / 32 == (offset + length - 1) / 32) {
        
            // TODO handle unsealing
            (void)sealCode;
            
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
                        // Write sub-class ID using Data Block Class (0x3e)
                        data[0] = 0x3e;
                        data[1] = subClassId;

                        if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                            // Write offset using Block Offset (0x3f) and then
                            // read the data block
                            data[0] = 0x3f;
                            data[1] = offset / 32;

                            if ((gpI2c->write(gAddress, &(data[0]), 2, true) == 0) &&
                                (gpI2c->read(gAddress, &(block[0]), 32) == 0)) {
                                // Compute the existing block checksum and then read it
                                data[2] = computeChecksum(&(block[0]));
                                data[0] = 0x60;
                                data[1] = 0;

                                if ((gpI2c->write(gAddress, &(data[0]), 1, true) == 0) &&
                                    (gpI2c->read(gAddress, &(data[1]), 1) == 0)) {

                                    // Does the checksum match?
                                    if (data[1] == data[2]) {
                                        // If so write the new data to the block data area at 0x40 plus the offset (plus the Block Offset already written)
                                        // NOTE: I tried doing this as two separate writes, one of the offset and then another of the
                                        // data block (so that I could use pData directly rather than having to extend the local
                                        // data array by 32) but the chip didn't like that, hence we have to copy pData into the
                                        // local array and do a single contiguous write.
                                        data[0] = 0x40 + (offset % 32);
                                        memcpy (&(data[1]), pData, length);

                                        if (gpI2c->write(gAddress, &(data[0]), length + 1) == 0) {
                                            // Also write the data into our local block variable 
                                            // on top of the previously read data and use
                                            // that to compute the new block checksum, then write it
                                            memcpy (&(block[offset % 32]), pData, length); 
                                            data[0] = 0x60;
                                            data[1] = computeChecksum(&(block[0]));
                                            
                                            if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
                                                // Exit config mode with SOFT_RESET command 0x42
                                                data[0] = 0;
                                                data[1] = 0x42;
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
                                                        printf("BatteryGaugeBq27441 (I2C 0x%02x): Flags register didn't show config update flag unset in time.\n", gAddress >> 1);
#endif
                                                    }
                                                } else {
#ifdef DEBUG_BQ27441
                                                    printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write config update exit after update.\n", gAddress >> 1);
#endif
                                                }
                                            } else {
#ifdef DEBUG_BQ27441
                                                printf("BatteryGaugeBq27441 (I2C 0x%02x): checksum on modified data block incorrect (0x%02x) during config update.\n", gAddress >> 1, data[1]);
#endif
                                            }
                                        } else {
#ifdef DEBUG_BQ27441
                                            printf("BatteryGaugeBq27441 (I2C 0x%02x): couldn't write all %d bytes during config update.\n", gAddress >> 1, length);
#endif
                                        }
                                    } else {
#ifdef DEBUG_BQ27441
                                        printf("BatteryGaugeBq27441 (I2C 0x%02x): checksum on read data block incorrect (expected 0x%02x, received 0x%02x).\n", gAddress >> 1, data[2], data[1]);
#endif
                                    }
                                } else {
#ifdef DEBUG_BQ27441
                                    printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to read Block Data Checksum for config update.\n", gAddress >> 1);
#endif
                                }
                            } else {
#ifdef DEBUG_BQ27441
                                printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write Block Offset (%d), or possibly read the data block, for config update.\n", gAddress >> 1, data[1]);
#endif
                            }
                        } else {
#ifdef DEBUG_BQ27441
                            printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to set sub-class ID (0x%02x) for config update.\r", gAddress >> 1, subClassId);
#endif
                        }
                    } else {
#ifdef DEBUG_BQ27441
                        printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to set Block Data Control for config update.\n", gAddress >> 1);
#endif
                    }
                } else {
#ifdef DEBUG_BQ27441
                    printf("BatteryGaugeBq27441 (I2C 0x%02x): Flags register didn't show config update flag set in time.\n", gAddress >> 1);
#endif
                }
            } else {
#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): unable to write to control register for config update.\n", gAddress >> 1);
#endif
            }
        } else {
#ifdef DEBUG_BQ27441
            printf("BatteryGaugeBq27441 (I2C 0x%02x): offset (%d) is in different 32 byte block to offset + length (%d) [length is %d].\n", gAddress >> 1, offset, offset + length, length);
#endif
        }
    }

    return success;
}

/// Make sure that the device is awake and has taken a reading.
// Note: the function does its own locking of gpI2C so that it isn't
// held for the entire time we wait for ADC readings to complete.
bool BatteryGaugeBq27441::makeAdcReading(void)
{
    bool success = false;
    char data[3];
    
    // Send CLEAR_HIBERNATE
    data[0] = 0x00;  // Set address to first register for control
    data[1] = 0x12;  // First byte of CLEAR_HIBERNATE sub-command (0x12)
    data[2] = 0x00;  // Second byte of CLEAR_HIBERNATE sub-command (0x00)

    gpI2c->lock();
    success = (gpI2c->write(gAddress, &(data[0]), 3) == 0);
    gpI2c->unlock();
    wait_ms (ADC_READ_WAIT_MS);
    
    return success;
}

/// Set Hibernate mode.
// Note: gpI2c should be locked before this is called.
bool BatteryGaugeBq27441::setHibernate(void)
{
    char data[3];
    
    // Send SET_HIBERNATE
    data[0] = 0x00;  // Set address to first register for control
    data[1] = 0x11;  // First byte of SET_HIBERNATE sub-command (0x11)
    data[2] = 0x00;  // Second byte of SET_HIBERNATE sub-command (0x00)
    
    return (gpI2c->write(gAddress, &(data[0]), 3) == 0);
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
BatteryGaugeBq27441::BatteryGaugeBq27441(void)
{
    gpI2c = NULL;
    gReady = false;
    gMonitorOn = false;
}

/// Destructor.
BatteryGaugeBq27441::~BatteryGaugeBq27441(void)
{
}

/// Initialise ourselves.
bool BatteryGaugeBq27441::init (I2C * pI2c, uint8_t address)
{
    char data[3];
    uint16_t answer;

    gpI2c = pI2c;
    gAddress = address << 1;

    if (gpI2c != NULL) {
        gpI2c->lock();
        // Send a control command to read the firmware version
        data[0] = 0x00;  // Set address to first register for control
        data[1] = 0x02;  // First byte of FW_VERSION sub-command (0x02)
        data[2] = 0x00;  // Second byte of FW_VERSION sub-command (0x00) (register address will auto-increment)

        if (gpI2c->write(gAddress, &(data[0]), 3) == 0) {
            if (getTwoBytes (0, &answer)) {
                // The expected response is 0x0109
                if (((answer >> 8) == 0x01) && ((answer & 0xff) == 0x09)) {
                    // Set the Sleep Current to the maximum value so that, if
                    // we tell the chip to go to sleep mode, it will do so
                    // straight away.  Sleep Current is offset 31 in the State
                    // sub-class (82) and max value is 1000.  Since offset 31
                    // and a length of 2 crosses a 32 bytes boundary this needs
                    // two separate writes.
                    data[0] = 1000 >> 8;
                    data[1] = 1000 & 0xFF;
                    if (writeExtendedData(82, 31, 1, &(data[0]), 0) &&
                        writeExtendedData(82, 32, 1, &(data[1]), 0)) {
                        // Now enter Hibernate mode to minimise power consumption
                        // Need to set either the Hibernate Current or Hibernate Voltage value
                        // to max, otherwise we won't hibernate, then set the SET_HIBERNATE
                        // bit.  Here we set Hibernate V element (offset 9) in the Power
                        // sub-class (68) to its max value (see section 6.4.1.6.2 of the
                        // BQ27441 technical reference manual).
                        // Note: the cell must also be "relaxed" for this to occur and so
                        // the chip may still not enter Hibernate mode for a little while.                    
                        data[0] = 5000 >> 8;
                        data[1] = 5000 & 0xFF;
                        if (writeExtendedData(68, 9, 2, &(data[0]), 0)) {
                            // Now send SET_HIBERNATE
                            gReady = setHibernate();
                        }
                    }
                    
                }
#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): read 0x%04x as FW_VERSION, expected 0x0109.\n", gAddress >> 1, answer);
#endif
            }
        }
        gpI2c->unlock();
    }

#ifdef DEBUG_BQ27441
    if (gReady) {
        printf("BatteryGaugeBq27441 (I2C 0x%02x): handler initialised.\r\n", gAddress >> 1);
    } else {
        printf("BatteryGaugeBq27441 (I2C 0x%02x): device NOT found.\r\n", gAddress >> 1);
    }
#endif

    return gReady;
}

/// Switch on/off the battery capacity monitor
bool BatteryGaugeBq27441::setMonitor (bool onNotOff, bool isSlow)
{
    bool success = false;
    char data[3];
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        if (onNotOff) {
            // Make sure that we are not in hibernate
            data[0] = 0x00;  // Set address to first register for control
            data[1] = 0x12;  // First byte of CLEAR_HIBERNATE sub-command (0x12)
            data[2] = 0x00;  // Second byte of CLEAR_HIBERNATE sub-command (0x00)
            if (gpI2c->write(gAddress, &(data[0]), 3) == 0) {
                gMonitorOn = true;
                // Read the OpConfig register which is in the Registers sub-class
                // (64) at offset 0.
                if (readExtendedData(64, 0, 2, &(data[0]), 0)) {
                    // SLEEP mode is bit 5 of the low byte of OpConfig.  In SLEEP
                    // mode a reading is taken every 20 seconds.
                    if (isSlow && !(data[0] & (1 << 5))) {
                        // Set the SLEEP bit 'cos it's not set and needs to be
                        data[0] |= 1 << 5;
                        // Write the new value back
                        if (writeExtendedData(64, 0, 2, &(data[0]), 0)) {
                            success = true;
                        }
                    } else if (!isSlow && (data[0] & (1 << 5))) {
                        // Clear the SLEEP bit 'cos it's set and shouldn't be
                        data[0] &= ~(1 << 5);
                        // Write the new value back
                        if (writeExtendedData(64, 0, 2, &(data[0]), 0)) {
                            success = true;
                        }
                    } else {
                        success = true;
                    }
                }
            }
        } else {
            // Send SET_HIBERNATE
            if (setHibernate()) {
                success = true;
                gMonitorOn = false;
            }
        }

        gpI2c->unlock();
    }
    
    return success;
}

/// Check whether a battery has been detected or not.
bool BatteryGaugeBq27441::isBatteryDetected (void)
{
    bool isDetected = false;
    uint16_t data;

    if (gReady && (gpI2c != NULL)) {
        
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();            
            // Read from the flags register address
            if (getTwoBytes (0x06, &data)) {

                // If bit 8 is set then a battery has been detected
                if (data & 0x0080) {
                    isDetected = true;
                }

#ifdef DEBUG_BQ27441
                if (isDetected) {
                    printf("BatteryGaugeBq27441 (I2C 0x%02x): battery detected.\n", gAddress >> 1);
                } else {
                    printf("BatteryGaugeBq27441 (I2C 0x%02x): battery NOT detected.\n", gAddress >> 1);
                }
#endif
            }
            
            // Set hibernate again if we are not monitoring
            if (!gMonitorOn) {
                setHibernate();
            }

            gpI2c->unlock();
        }        
    }
    
    return isDetected;
}

/// Get the temperature of the chip.
bool BatteryGaugeBq27441::getTemperature (int32_t *pTemperatureC)
{
    bool success = false;
    int32_t temperatureC = 0;
    uint16_t data;

    if (gReady && (gpI2c != NULL)) {
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();
            // Read from the temperature register address
            if (getTwoBytes (0x02, &data)) {
                success = true;

                // The answer is in units of 0.1 K, so convert to C
                temperatureC = ((int32_t) data / 10) - 273;

                if (pTemperatureC) {
                    *pTemperatureC = temperatureC;
                }

#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): chip temperature %.1f K, so %d C.\n", gAddress >> 1, ((float) data) / 10, temperatureC);
#endif
            }
        
            // Return to sleep if we can
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }
            
            gpI2c->unlock();
        }
    }

    return success;
}

/// Get the Voltage of the battery.
bool BatteryGaugeBq27441::getVoltage (int32_t *pVoltageMV)
{
    bool success = false;
    uint16_t data = 0;

    if (gReady && (gpI2c != NULL)) {
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();            
            // Read from the voltage register address
            if (getTwoBytes (0x04, &data)) {
                success = true;

                // The answer is in mV
                if (pVoltageMV) {
                    *pVoltageMV = (int32_t) data;
                }

#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): battery voltage %.3f V.\n", gAddress >> 1, ((float) data) / 1000);
#endif
            }

            // Return to sleep if we can
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }

            gpI2c->unlock();
        }
    }
    
    return success;
}

/// Get the current flowing from the battery.
bool BatteryGaugeBq27441::getCurrent (int32_t *pCurrentMA)
{
    bool success = false;
    int32_t currentMA = 0;
    uint16_t data;

    if (gReady && (gpI2c != NULL)) {
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();            
            // Read from the average current register address
            if (getTwoBytes (0x10, &data)) {
                success = true;

                if (pCurrentMA) {
                    *pCurrentMA = currentMA;
                }

#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): current %d mA.\n", gAddress >> 1, currentMA);
#endif
            }

            // Return to sleep if we can
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }

            gpI2c->unlock();
        }
    }
    
    return success;
}

/// Get the remaining battery capacity.
bool BatteryGaugeBq27441::getRemainingCapacity (int32_t *pCapacityMAh)
{
    bool success = false;
    uint16_t data = 0;

    if (gReady && (gpI2c != NULL)) {
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();            
            // Read from the RemainingCapacity register address
            
            if (getTwoBytes (0x0c, &data)) {
                success = true;

                // The answer is in mAh
                if (pCapacityMAh) {
                    *pCapacityMAh = (int32_t) data;
                }

#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): remaining battery capacity %u mAh.\n", gAddress >> 1, data);
#endif
            }

            // Return to sleep if we can
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }

            gpI2c->unlock();
        }
    }
    
    return success;
}

/// Get the battery percentage remaining
bool BatteryGaugeBq27441::getRemainingPercentage (int32_t *pBatteryPercent)
{
    bool success = false;
    uint16_t data = 0;

    if (gReady && (gpI2c != NULL)) {
        // Make sure there's recent reading
        if (gMonitorOn || makeAdcReading()) {            
            gpI2c->lock();            
            
            // Wake up and take a reading if we have to
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }

            // Read from the StateOfCharge register address
            if (getTwoBytes (0x1c, &data)) {
                success = true;

                if (pBatteryPercent) {
                    *pBatteryPercent = (int32_t) data;
                }

#ifdef DEBUG_BQ27441
                printf("BatteryGaugeBq27441 (I2C 0x%02x): remaining battery percentage %u%%.\n", gAddress >> 1, data);
#endif
            }

            // Return to sleep if we can
            if (!gMonitorOn && !setHibernate()) {
                success = false;
            }

            gpI2c->unlock();
        }
    }
    
    return success;
}

/// Advanced function to read a configuration data block.
bool BatteryGaugeBq27441::advancedGetConfig(uint8_t subClassId, int32_t offset, int32_t length, char * pData, uint32_t sealCode)
{
    bool success = false;

    if (gReady && (gpI2c != NULL) && (pData != NULL)) {
        gpI2c->lock();
        // Read the extended configuration data
        success = readExtendedData(subClassId, offset, length, pData, sealCode);
#ifdef DEBUG_BQ27441
        if (success) {
            printf("BatteryGaugeBq27441 (I2C 0x%02x): read extended data with subClassId %d from offset %d.\n", gAddress >> 1, subClassId, offset);
        }
#endif

        // Return to sleep if we can
        if (!gMonitorOn && !setHibernate()) {
            success = false;
        }

        gpI2c->unlock();
    }

    return success;
}

/// Advanced function to write a configuration data block.
bool BatteryGaugeBq27441::advancedSetConfig(uint8_t subClassId, int32_t offset, int32_t length, const char * pData, uint32_t sealCode)
{
    bool success = false;

    if (gReady && (gpI2c != NULL) && (pData != NULL)) {
        gpI2c->lock();
        // Write the extended configuration data
        success = writeExtendedData(subClassId, offset, length, pData, sealCode);
#ifdef DEBUG_BQ27441
        if (success) {
            printf("BatteryGaugeBq27441 (I2C 0x%02x): written %d bytes of extended data with subClassId %d from offset %d.\n", gAddress >> 1, length, subClassId, offset);
        }
#endif

        // Return to sleep if we can
        if (!gMonitorOn && !setHibernate()) {
            success = false;
        }

        gpI2c->unlock();
    }

    return success;
}

// End Of File
