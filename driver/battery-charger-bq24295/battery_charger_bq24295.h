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

/// Device I2C address.
#define BATTERY_CHARGER_BQ24295_ADDRESS 0x6B

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

/// BQ27441 battery charger driver.
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
        MAX_NUM_CHARGER_STATES
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

    /// Read the temperature of the battery.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getBatteryTemperature (int32_t *pTemperatureC);

    /// Read the temperature of the BQ24295 chip.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getChipTemperature (int32_t *pTemperatureC);

    /// Get the charger fault status.
    // \return the charger fault status.
    ChargerFault getChargerFault(void);
    
    /// Enable OTG charging.
    // \return true if successful, otherwise false.
    bool enableOtg (void);

    /// Disable OTG charging.
    // \return true if successful, otherwise false.
    bool disableOtg (void);

    /// Get whether OTG charging is enabeld or not.
    // \return true if OTG charging is enabled, otherwise false.
    bool isOtgEnabled (void);

    /// Enable charging.
    // \return true if successful, otherwise false.
    bool enableCharging (void);

    /// Disable charging.
    // \return true if successful, otherwise false.
    bool disableCharging (void);

    /// Get the state of charging (enabled or disabled).
    // \return true if charging is enabled, otherwise false.
    bool isChargingEnabled (void);

    /// Set the system voltage (the voltage which the
    // chip will attempt to maintain the system at if both
    // external and internal power are present).
    // \param voltageMV the voltage limit, in milliVolts.
    // \return true if successful, otherwise false.
    bool setSystemVoltage (int32_t voltageMV);

    /// Get the system voltage.
    // \param pVoltageMV a place to put the system voltage limit.
    // \return true if successful, otherwise false.
    bool getSystemVoltage (int32_t *pVoltageMV);

    /// Set the input voltage limit.  If the input falls below
    // this level then charging will be ramped down.  The limit
    // does not take effect until enableInputLimits() is called.
    // \param voltageMV the input voltage limit, in milliVolts.
    //        A value of 0 will be ignored, use clearInputLimits()
    //        instead.
    // \return true if successful, otherwise false.
    bool setInputVoltageLimit (int32_t voltageMV);

    /// Get the input voltage limit.
    // \param pVoltageMV a place to put the input voltage limit.
    // \return true if successful, otherwise false.
    bool getInputVoltageLimit (int32_t *pVoltageMV);

    /// Set the input current limit.  If the current drawn
    // goes above this limit then charging will be ramped down.
    // The limit does not take effect until enableInputLimits()
    // is called.
    // \param currentMA the input current limit, in milliAmps.
    //        A value of 0 will be ignored, use clearInputLimits()
    //        instead.
    // \return true if successful, otherwise false.
    bool setInputCurrentLimit (int32_t currentMA);

    /// Get the input current limit.
    // \param pCurrentMA a place to put the input current limit.
    // \return true if successful, otherwise false.
    bool getInputCurrentLimit (int32_t *pCurrentMA);

    /// Enable input voltage and current limits.
    // \return true if successful, otherwise false.
    bool enableInputLimits (void);

    /// Remove any input voltage or current limits.
    // \return true if successful, otherwise false.
    bool disableInputLimits (void);

    /// Check whether input limits are enabled.
    // \return true if input limits are enabled, otherwise false.
    bool areInputLimitsEnabled (void);

    /// Set the charging voltage limit.
    // \param voltageMV the charging voltage limit, in milliVolts.
    // \return true if successful, otherwise false.
    bool setChargingVoltageLimit (int32_t voltageMV);

    /// Get the charging voltage limit.
    // \param pVoltageMV a place to put the charging voltage limit, in milliVolts.
    // \return true if successful, otherwise false.
    bool getChargingVoltageLimit (int32_t *pVoltageMV);

    /// Set the recharging voltage threshold.
    // \param voltageMV the recharging voltage threshold, in milliVolts.
    // \return true if successful, otherwise false.
    bool setRechargingVoltageThreshold (int32_t voltageMV);

    /// Get the recharging voltage threshold.
    // \param pVoltageMV a place to put the charging voltage threshold, in milliVolts.
    // \return true if successful, otherwise false.
    bool getRechargingVoltageThreshold (int32_t *pVoltageMV);

    /// Set the fast charging current limit.
    // \param currentMA the fast charging current limit, in milliAmps.
    // \return true if successful, otherwise false.
    bool setFastChargingCurrentLimit (int32_t currentMA);

    /// Get the fast charging current limit.
    // \param pCurrentMA a place to put the fast charging current limit.
    // \return true if successful, otherwise false.
    bool getFastChargingCurrentLimit (int32_t *pCurrentMA);

    /// Set the pre-charging current limit.
    // \param currentMA the pre-charging current limit, in milliAmps.
    // \return true if successful, otherwise false.
    bool setPrechargingCurrentLimit (int32_t currentMA);

    /// Get the pre-charging current limit.
    // \param pCurrentMA a place to put the pre-charging current limit.
    // \return true if successful, otherwise false.
    bool getPrechargingCurrentLimit (int32_t *pCurrentMA);

    /// Set the charging termination current limit.
    // \param currentMA the charging termination current limit, in milliAmps.
    // \return true if successful, otherwise false.
    bool setChargingTerminationCurrentLimit (int32_t currentMA);

    /// Get the charging termination current limit.
    // \param pCurrentMA a place to put the charging termination current limit.
    // \return true if successful, otherwise false.
    bool getChargingTerminationCurrentLimit (int32_t *pCurrentMA);

    /// Enable charging termination.
    // \return true if successful, otherwise false.
    bool enableChargingTermination (void);

    /// Disable charging termination.
    // \return true if successful, otherwise false.
    bool disableChargingTermination (void);

    /// Get the state of charging termination (enabled or disabled).
    // \return true if charging termination is enabled, otherwise false.
    bool isChargingTerminationEnabled (void);

    /// Set the boost voltage.
    // \param voltageMV the boost voltage, in milliVolts.
    // \return true if successful, otherwise false.
    bool setBoostVoltage (int32_t voltageMV);

    /// Get the boost voltage.
    // \param pVoltageMV a place to put the boost voltage, in milliVolts.
    // \return true if successful, otherwise false.
    bool getBoostVoltage (int32_t *pVoltageMV);

    // TODO: make sense of the thermal regulation stuff (in current and voltage.
    // TODO: make sense of FORCE_20PCT.
    
    /// Set the fast charging safety timer.
    // \param timerHours the charging safety timer value.
    //        Use a value of 0 to indicate that the timer should be disabled.
    // \return true if successful, otherwise false.
    bool setFastChargingSafetyTimer (int32_t timerHours);

    /// Get the fast charging safety timer value (in hours).
    // \param pTimerHours a place to put the charging safety timer value.
    //        Returned value is zero if the fast charging safety timer is disabled.
    // \return true if charging termination is enabled, otherwise false.
    bool getFastChargingSafetyTimer (int32_t *pTimerHours);

    /// Enable shipping mode (lowest possible power state).
    // \return true if successful, otherwise false.
    bool setShippingMode(void);

    /// Get the reason(s) for an interrupt occurring.
    // Note: as with all the other API functions here, this should
    // not be called from an interrupt function as the comms with the
    // chip over I2C will take too long.
    // \return a bit-map containing the Int reasons that
    //         can be tested against the values of the Int enum.
    char getIntReason (void);
        
protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    /// The address of the device.
    uint8_t gAddress;
    /// Flag to indicate device is ready
    bool gReady;

    /// Read a register.
    // Note: gpI2c should be locked before this is called.
    // \param address the address to read from.
    // \param pValue a place to put the returned value.
    // \return true if successful, otherwise false.
    bool getRegister(char address, char *pValue);

    /// Set a register.
    // Note: gpI2c should be locked before this is called.
    // \param address the address to write to.
    // \param value the value to write.
    // \return true if successful, otherwise false.
    bool setRegister(char address, char value);

    /// Set a mask of bits in a register.
    // Note: gpI2c should be locked before this is called.
    // \param address the address to write to.
    // \param mask the mask of bits to set.
    // \return true if successful, otherwise false.
    bool setRegisterBits(char address, char mask);

    /// Clear a mask of bits in a register.
    // Note: gpI2c should be locked before this is called.
    // \param address the address to write to.
    // \param mask the mask of bits to clear.
    // \return true if successful, otherwise false.
    bool clearRegisterBits(char address, char mask);
};

#endif

// End Of File
