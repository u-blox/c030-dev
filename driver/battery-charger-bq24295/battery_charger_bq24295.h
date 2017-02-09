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

#ifndef BATTERY_CHARGER_BQ24295_HPP
#define BATTERY_CHARGER_BQ24295_HPP

/**
 * @file battery_charger_bq24295.h
 * This file defines the API to the TI BQ24295 battery charger chip.
 */

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

/// Device I2C address
#define BATTERY_CHARGER_BQ24295_ADDRESS 0x6B

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

class BatteryChargerBq24295 {
public:
    /// Charger state.
    typedef enum {
        CHARGER_STATE_UNKNOWN,
        CHARGER_STATE_DISABLED,
        CHARGER_STATE_NO_EXTERNAL_POWER,
        CHARGER_STATE_NOT_CHARGING,
        CHARGER_STATE_PRECHARGE,
        CHARGER_STATE_FAST_CHARGE,
        CHARGER_STATE_COMPLETE,
        MAX_NUM_CHARGE_STATES
    } ChargerState;

    /// Charger faults
    typedef enum {
        CHARGER_FAULT_UNKNOWN,
        CHARGER_FAULT_NONE,
        CHARGER_FAULT_INPUT_FAULT,
        CHARGER_FAULT_THERMAL_SHUTDOWN,
        CHARGER_FAULT_CHARGE_TIMER_EXPIRED,
        CHARGER_FAULT_BATTERY_OVER_VOLTAGE,
        CHARGER_FAULT_THERMISTOR_TOO_COLD,
        CHARGER_FAULT_THERMISTOR_TOO_HOT,
        CHARGER_FAULT_WATCHDOG_EXPIRED,
        CHARGER_FAULT_BOOST,
        MAX_NUM_CHARGER_FAULTS
    } ChargerFault;

    /// Constructor.
    BatteryChargerBq24295(void);
    /// Destructor.
    ~BatteryChargerBq24295(void);

    /// Initialise the BQ24295 chip.
    // \param pI2c a pointer to the I2C instance to use.
    //\ param address 7-bit I2C address of the battery charger chip.
    // \return true if successful, otherwise false.
    bool init (I2C * pI2c, uint8_t address = BATTERY_CHARGER_BQ24295_ADDRESS);

    /// Get the charger state.
    // \return the charge state.
    ChargerState getChargerState(void);

    /// Get whether external power is present or not.
    // \return true if external power is present, otherwise false.
    bool isExternalPowerPresent(void);

    /// Get the charger fault status.
    // \return the charger fault status.
    ChargerFault getChargerFault(void);

protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    // The address of the device.
    uint8_t gAddress;
    // Flag to indicate device is ready
    bool gReady;
};

#endif

// End Of File
