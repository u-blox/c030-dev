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

#ifndef BATTERY_GAUGE_BQ27441_HPP
#define BATTERY_GAUGE_BQ27441_HPP

/**
 * @file battery_gauge_bq27441.h
 * This file defines the API to the TI BQ27441 battery gauge chip.
 */

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

/// Device I2C address
#define BATTERY_GAUGE_BQ27441_ADDRESS 0x55

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

class BatteryGaugeBq27441 {
public:

    /// Constructor.
    BatteryGaugeBq27441(void);
    /// Destructor.
    ~BatteryGaugeBq27441(void);

    /// Initialise the BQ27441 chip.
    // \param pI2c a pointer to the I2C instance to use.
    // \param rSenseMOhm the value of the sense resistor being used, in milli Ohms.
    //\ param address 7-bit I2C address of the battery gauge chip.
    // \return true if successful, otherwise false.
    bool init (I2C * pI2c, int32_t rSenseMOhm, uint8_t address = BATTERY_GAUGE_BQ27441_ADDRESS);

    /// Read the temperature of the BQ27441 chip.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getTemperature (int32_t *pTemperatureC);

    /// Read the voltage of the battery.
    // \param pVoltageMV place to put the voltage reading.
    // \return true if successful, otherwise false.
    bool getVoltage (int32_t *pVoltageMV);

    /// Read the remaining available battery energy.
    // \param pCapacityMAh place to put the capacity reading.
    // \return true if successful, otherwise false.
    bool getRemainingCapacity (int32_t *pCapacityMAh);

    /// Read the state of charge of the battery as a percentage.
    // \param pBatteryPercent place to put the reading.
    // \return true if successful, otherwise false.
    bool getRemainingPercentage (int32_t *pBatteryPercent);

protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    // The address of the device.
    uint8_t gAddress;
    // Flag to indicate device is ready
    bool gReady;

    /// Compute the checksum of a block of memory in the chip.
    // \return  the checksum value.
    uint8_t computeChecksum();

    /// Write an extended data block.
    // \param classId the class ID of the block.
    // \param offset the offset of the data within the class.
    // \param pData a pointer to the data to be written.
    // \param len the length of the data to be written.
    // \return true if successful, otherwise false.
    bool writeExtendedData(uint8_t classId, uint8_t offset, const char * pData, uint8_t len);

    /// Read two bytes starting at a given address.
    // \param registerAddress the register address to start reading from.
    // \param pBytes place to put the two bytes.
    // \return true if successful, otherwise false.
    bool getTwoBytes (uint8_t registerAddress, uint16_t *pBytes);
};

#endif

// End Of File
