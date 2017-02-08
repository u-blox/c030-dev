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

#ifndef BATTERY_GAUGE_LTC2943_HPP
#define BATTERY_GAUGE_LTC2943_HPP

/**
 * @file battery_gauge_ltc2943.h
 * This file defines the API to the Linear Technology LTC2943 battery gauge chip.
 */

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

/// Device I2C address
#define BATTERY_GAUGE_LTC2943_ADDRESS 0x64
/// Default RSense (in mOhm)
#define BATTERY_GAUGE_LTC2943_RSENSE_MOHM 68

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

class BatteryGaugeLtc2943 {
public:

    /// Constructor.
    BatteryGaugeLtc2943(void);
    /// Destructor.
    ~BatteryGaugeLtc2943(void);

    /// Initialise the BQ27441 chip.
    // \param pI2c a pointer to the I2C instance to use.
    // \param rSenseMOhm the value of the sense resistor being used, in milli Ohms.
    //\ param address 7-bit I2C address of the LiPo gauge chip.
    // \return true if successful, otherwise false.
    bool init (I2C * pI2c, uint32_t rSenseMOhm = BATTERY_GAUGE_LTC2943_RSENSE_MOHM, uint8_t address = BATTERY_GAUGE_LTC2943_ADDRESS);

    /// Read the temperature of the BQ27441 chip.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getTemperature (int8_t *pTemperatureC);

    /// Read the voltage of the battery.
    // \param pVoltageMV place to put the voltage reading.
    // \return true if successful, otherwise false.
    bool getVoltage (uint16_t *pVoltageMV);

    /// Read the remaining available battery energy.
    // \param pCapacityMAh place to put the capacity reading.
    // \return true if successful, otherwise false.
    bool getRemainingCapacity (uint32_t *pCapacityMAh);

    /// Read the state of charge of the battery as a percentage.
    // \param pBatteryPercent place to put the reading.
    // \return true if successful, otherwise false.
    bool getRemainingPercentage (uint16_t *pBatteryPercent);

protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    // Value of RSense.
    uint8_t gRSenseMOhm;
    // The address of the device.
    uint8_t gAddress;
    // Flag to indicate device is ready
    bool gReady;
};

#endif

// End Of File
