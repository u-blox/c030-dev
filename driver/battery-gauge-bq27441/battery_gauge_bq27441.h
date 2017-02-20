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

/// Device I2C address.
#define BATTERY_GAUGE_BQ27441_ADDRESS 0x55

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

/// BQ27441 battery gauge driver.
class BatteryGaugeBq27441 {
public:

    /// Constructor.
    BatteryGaugeBq27441(void);
    /// Destructor.
    ~BatteryGaugeBq27441(void);

    /// Initialise the BQ27441 chip.
    // \param pI2c a pointer to the I2C instance to use.
    //\ param address 7-bit I2C address of the battery gauge chip.
    // \return true if successful, otherwise false.
    bool init (I2C * pI2c, uint8_t address = BATTERY_GAUGE_BQ27441_ADDRESS);

    /// Switch on/off the battery capacity monitor
    // \param onNotOff true to begin monitoring battery capacity, false to stop.
    // \param isSlow set this to true to save power if the battery current is not fluctuating very much.
    // \return true if successful, otherwise false.
    bool setMonitor (bool onNotOff, bool isSlow = false);

    /// Determine whether a battery has been detected or not.
    // \return true if a battery has been detected, otherwise false.
    bool isBatteryDetected (void);
    
    /// Read the temperature of the BQ27441 chip.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getTemperature (int32_t *pTemperatureC);

    /// Read the voltage of the battery.
    // \param pVoltageMV place to put the voltage reading.
    // \return true if successful, otherwise false.
    bool getVoltage (int32_t *pVoltageMV);

    /// Read the current flowing from the battery.
    // \param pCurrentMA place to put the current reading.
    // \return true if successful, otherwise false.
    bool getCurrent (int32_t *pCurrentMA);

    /// Read the remaining available battery energy.
    // \param pCapacityMAh place to put the capacity reading.
    // \return true if successful, otherwise false.
    bool getRemainingCapacity (int32_t *pCapacityMAh);

    /// Read the state of charge of the battery as a percentage.
    // \param pBatteryPercent place to put the reading.
    // \return true if successful, otherwise false.
    bool getRemainingPercentage (int32_t *pBatteryPercent);

    /// An advanced function to read configuration data from the BQ27441 chip memory.
    // Please refer to the TI BQ27441 technical reference manual for details of classId,
    // offset, the meanings of the data structures and their lengths.
    // Note: the chip handles the data for each sub-class in 32 byte blocks and the offset/
    // length combination used must respect this.  For instance:
    //
    // Sub-class N (length 87 bytes)
    //          bytes 0 to 31                      bytes 32 to 63                  bytes 64 to 86
    //  --------------------------------  --------------------------------  -----------------------
    // |         Data Block 0           ||    xx    Data Block 1  yy      ||zz       Data Block 2  |
    //  --------------------------------  --------------------------------  -----------------------
    //
    // To read item xx, 2 bytes long at offset 36, one would specify an offset of 36 and a length
    // of 2.  To read both xx and yy at the same time (yy being 2 bytes long at offset 56),
    // one could specify an offset of 36 and a length of 21.  However, one could not read xx, yy
    // and zz at the same time, or yy and zz at the same time, since they fall into different blocks;
    // two separate reads would be required.
    // \param subClassId the sub-class ID of the block.
    // \param offset the offset of the data within the class.
    // \param length the amount of data to read.
    // \param pData a place to put the read data.
    // \param sealCode the 32 bit seal code that will unseal the device if it is sealed.
    // \return true if successful, otherwise false.
    bool advancedGetConfig(uint8_t subClassId, int32_t offset, int32_t length, char * pData, uint32_t sealCode = 0);

    /// An advanced function to write configuration data to the BQ27441 chip memory.
    // Please refer to the TI BQ27441 technical reference manual for details of classId,
    // offset, the meanings of the data structures and their lengths.  See also the note above
    // advancedGetConfig() about how to use offset and length.
    // \param subClassId the sub-class ID of the block.
    // \param offset the offset of the data within the class.
    // \param length the length of the data to be written.
    // \param pData a pointer to the data to be written.
    // \param sealCode the 32 bit seal code that will unseal the device if it is sealed.
    // \return true if successful, otherwise false.
    bool advancedSetConfig(uint8_t subClassId, int32_t offset, int32_t length, const char * pData, uint32_t sealCode = 0);

protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    /// The address of the device.
    uint8_t gAddress;
    /// Flag to indicate device is ready
    bool gReady;
    /// Flag to indicate that monitor mode is active
    bool gMonitorOn;

    /// Read two bytes starting at a given address.
    // \param registerAddress the register address to start reading from.
    // \param pBytes place to put the two bytes.
    // \return true if successful, otherwise false.
    bool getTwoBytes (uint8_t registerAddress, uint16_t *pBytes);

    /// Compute the checksum of a block of memory in the chip.
    // \param pData a pointer to the 32 byte data block.
    // \return the checksum value.
    uint8_t computeChecksum(const char * pData);

    /// Read data of a given length and class ID.
    // \param subClassId the sub-class ID of the block.
    // \param offset the offset of the data within the class.
    // \param pData a place to put the read data.
    // \param length the size of the place to put the data block.
    // \param sealCode the 32 bit seal code that will unseal the device if it is sealed.
    // \return true if successful, otherwise false.
    bool readExtendedData(uint8_t subClassId, int32_t offset, int32_t length, char * pData, uint32_t sealCode);
    
    /// Write an extended data block.
    // \param subClassId the sub-class ID of the block.
    // \param offset the offset of the data within the class.
    // \param pData a pointer to the data to be written.
    // \param length the size of the data to be written.
    // \param sealCode the 32 bit seal code that will unseal the device if it is sealed.
    // \return true if successful, otherwise false.
    bool writeExtendedData(uint8_t subClassId, int32_t offset, int32_t length, const char * pData, uint32_t sealCode);

    /// Make sure that the device is awake and has taken a reading.
    // \return true if successful, otherwise false.
    bool makeAdcReading(void);

    /// Set Hibernate mode.
    // \return true if successful, otherwise false.
    bool setHibernate(void);
};

#endif

// End Of File
