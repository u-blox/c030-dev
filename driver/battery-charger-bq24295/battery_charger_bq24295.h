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

#ifndef BATTERY_CHARGER_BQ24295_H
#define BATTERY_CHARGER_BQ24295_H

/**
 * @file battery_charger_bq24295.h
 * This file defines the API to the TI BQ24295 battery charger chip.
 */

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
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

    /// Charger faults as a bitmap that matches the chip REG09 definitions
    typedef enum {
        CHARGER_FAULT_NONE = 0x00,
        CHARGER_FAULT_THERMISTOR_TOO_HOT = 0x01,
        CHARGER_FAULT_THERMISTOR_TOO_COLD = 0x02,
        // Value 0x04 is reserved
        CHARGER_FAULT_BATTERY_OVER_VOLTAGE = 0x08,
        CHARGER_FAULT_INPUT_FAULT = 0x10,          //!< Note that the value of CHARGER_FAULT_CHARGE_TIMER_EXPIRED overlaps this, be careful when testing the bitmap.
        CHARGER_FAULT_THERMAL_SHUTDOWN = 0x20,     //!< Note that the value of CHARGER_FAULT_CHARGE_TIMER_EXPIRED overlaps this, be careful when testing the bitmap.
        CHARGER_FAULT_CHARGE_TIMER_EXPIRED = 0x30, //!< This looks odd as it overlaps the two above but it matches the register meaning as defined by the chip.
        CHARGER_FAULT_OTG = 0x40,
        CHARGER_FAULT_WATCHDOG_EXPIRED = 0x80,
        MAX_NUM_CHARGER_FAULTS
    } ChargerFault;

    /// Constructor.
    BatteryChargerBq24295(void);
    /// Destructor.
    ~BatteryChargerBq24295(void);

    /// Initialise the BQ24295 chip.
    // After initialisation the chip will be put into its lowest
    // power state and should be configured if the default settings
    // are not satisfactory.  Once the chip is correctly configured,
    // charging should be enabled.
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

    /// Enable charging.
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool enableCharging (void);

    /// Disable charging.
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool disableCharging (void);

    /// Get the state of charging (enabled or disabled).
    // \return true if charging is enabled, otherwise false.
    bool isChargingEnabled (void);

    /// Enable OTG charging.
    // Default is enabled.
    // \return true if successful, otherwise false.
    bool enableOtg (void);

    /// Disable OTG charging.
    // Default is enabled.
    // \return true if successful, otherwise false.
    bool disableOtg (void);

    /// Determine whether OTG charging is enabled or not.
    // \return true if OTG charging is enabled, otherwise false.
    bool isOtgEnabled (void);

    /// Set the system voltage (the voltage which the
    // chip will attempt to maintain the system at).
    // \param voltageMV the voltage limit, in milliVolts.
    //        Range is 3000 mV to 3700 mV, default 3500 mV.
    // \return true if successful, otherwise false.
    bool setSystemVoltage (int32_t voltageMV);

    /// Get the system voltage.
    // \param pVoltageMV a place to put the system voltage limit.
    // \return true if successful, otherwise false.
    bool getSystemVoltage (int32_t *pVoltageMV);

    /// Set the fast charging current limit.
    // \param currentMA the fast charging current limit, in milliAmps.
    //        Range is 512 mA to 3008 mA, default 1024 mA.
    // \return true if successful, otherwise false.
    bool setFastChargingCurrentLimit (int32_t currentMA);

    /// Get the fast charging current limit.
    // \param pCurrentMA a place to put the fast charging current limit.
    // \return true if successful, otherwise false.
    bool getFastChargingCurrentLimit (int32_t *pCurrentMA);

    /// Set the fast charging safety timer.
    // \param timerHours the charging safety timer value.
    //        Use a value of 0 to indicate that the timer should be disabled.
    //        Timer values will be translated to the nearest (lower) value
    //        out of 5, 8, 12, and 20 hours, default 12 hours.
    // \return true if successful, otherwise false.
    bool setFastChargingSafetyTimer (int32_t timerHours);

    /// Get the fast charging safety timer value.
    // \param pTimerHours a place to put the charging safety timer value.
    //        Returned value is zero if the fast charging safety timer is disabled.
    // \return true if charging termination is enabled, otherwise false.
    bool getFastChargingSafetyTimer (int32_t *pTimerHours);

    /// Set ICHG/IPRECH margin (see section 8.3.3.5 of the chip data sheet).
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool enableIcghIprechMargin (void);

    /// Clear the ICHG/IPRECH margin (see section 8.3.3.5 of the chip data sheet).
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool disableIcghIprechMargin (void);
    
    /// Check if the ICHG/IPRECH margin is set (see section 8.3.3.5 of
    //  the chip data sheet).
    // \return true if the ICHG/IPRECH margin is enabled, otherwise false.
    bool isIcghIprechMarginEnabled (void);
    
    /// Set the charging termination current.
    // \param currentMA the charging termination current, in milliAmps.
    //        Range is 128 mA to 2048 mA, default is 256 mA.
    // \return true if successful, otherwise false.
    bool setChargingTerminationCurrent (int32_t currentMA);

    /// Get the charging termination current.
    // \param pCurrentMA a place to put the charging termination current.
    // \return true if successful, otherwise false.
    bool getChargingTerminationCurrent (int32_t *pCurrentMA);

    /// Enable charging termination.
    // Default is enabled.
    // \return true if successful, otherwise false.
    bool enableChargingTermination (void);

    /// Disable charging termination.
    // Default is enabled.
    // \return true if successful, otherwise false.
    bool disableChargingTermination (void);

    /// Get the state of charging termination (enabled or disabled).
    // \return true if charging termination is enabled, otherwise false.
    bool isChargingTerminationEnabled (void);

    /// Set the pre-charging current limit.
    // \param currentMA the pre-charging current limit, in milliAmps.
    //        Range is 128 mA to 2048 mA, default is 256 mA.
    // \return true if successful, otherwise false.
    bool setPrechargingCurrentLimit (int32_t currentMA);

    /// Get the pre-charging current limit.
    // \param pCurrentMA a place to put the pre-charging current limit.
    // \return true if successful, otherwise false.
    bool getPrechargingCurrentLimit (int32_t *pCurrentMA);

    /// Set the charging voltage limit.
    // \param voltageMV the charging voltage limit, in milliVolts.
    //        Range is 3504 mV to 4400 mV, default is 4208 mV.
    // \return true if successful, otherwise false.
    bool setChargingVoltageLimit (int32_t voltageMV);

    /// Get the charging voltage limit.
    // \param pVoltageMV a place to put the charging voltage limit,
    //        in milliVolts.
    // \return true if successful, otherwise false.
    bool getChargingVoltageLimit (int32_t *pVoltageMV);

    /// Set the pre-charge to fast-charge voltage threshold.
    // \param voltageMV the threshold, in milliVolts.
    //        Values will be translated to the nearest (highest)
    //        voltage out of 2800 mV and 3000 mV, default is 3000 mV.
    // \return true if successful, otherwise false.
    bool setFastChargingVoltageThreshold (int32_t voltageMV);

    /// Get the pre-charge to fast-charge voltage threshold.
    // \param pVoltageMV a place to put the threshold, in milliVolts.
    // \return true if successful, otherwise false.
    bool getFastChargingVoltageThreshold (int32_t *pVoltageMV);

    /// Set the recharging voltage threshold.
    // \param voltageMV the recharging voltage threshold, in milliVolts.
    //        Values will be translated to the nearest (highest)
    //        voltage out of 100 mV and 300 mV, default is 100 mV.
    // \return true if successful, otherwise false.
    bool setRechargingVoltageThreshold (int32_t voltageMV);

    /// Get the recharging voltage threshold.
    // \param pVoltageMV a place to put the charging voltage threshold, in milliVolts.
    // \return true if successful, otherwise false.
    bool getRechargingVoltageThreshold (int32_t *pVoltageMV);

    /// Set the boost voltage.
    // \param voltageMV the boost voltage, in milliVolts.
    //        Range is 4550 mV to 5510 mV, default is 5126 mV.
    // \return true if successful, otherwise false.
    bool setBoostVoltage (int32_t voltageMV);

    /// Get the boost voltage.
    // \param pVoltageMV a place to put the boost voltage, in milliVolts.
    // \return true if successful, otherwise false.
    bool getBoostVoltage (int32_t *pVoltageMV);

    /// Set the boost mode upper temperature limit.
    // \param temperatureC the temperature in C.
    //        Values will be translated to the nearest (lower)
    //        of 55 C, 60 C and 65 C (disabled by default).
    // \return true if successful, otherwise false.
    bool setBoostUpperTemperatureLimit (int32_t temperatureC);

    /// Get the boost mode upper temperature limit.
    // If the boost mode upper temperature limit is not
    // enabled then pTemperatureC will remain untouched and false
    // will be returned.
    // \param pTemperatureC a place to put the temperature.
    // \return true if successful and a limit was set, otherwise false.
    bool getBoostUpperTemperatureLimit (int32_t *pTemperatureC);
    
    /// Check whether the boost mode upper temperature limit is enabled.
    // \return true if successful, otherwise false.
    bool isBoostUpperTemperatureLimitEnabled (void);

    /// Disable the boost mode upper temperature limit.
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool disableBoostUpperTemperatureLimit (void);

    /// Set the boost mode low temperature limit.
    // \param temperatureC the temperature in C.
    //        Values will be translated to the nearest (higher)
    //        of -10 C and -20 C, default is -10 C.
    // \return true if successful, otherwise false.
    bool setBoostLowerTemperatureLimit (int32_t temperatureC);

    /// Get the boost mode low temperature limit.
    // \param pTemperatureC a place to put the temperature.
    // \return true if successful, otherwise false.
    bool getBoostLowerTemperatureLimit (int32_t *pTemperatureC);
    
    /// Set the input voltage limit.  If the input falls below
    // this level then charging will be ramped down.  The limit
    // does not take effect until enableInputLimits() is called
    // (default setting is disabled).
    // \param voltageMV the input voltage limit, in milliVolts.
    //        Range is 3880 mV to 5080 mV, default is 4760 mV.
    // \return true if successful, otherwise false.
    bool setInputVoltageLimit (int32_t voltageMV);

    /// Get the input voltage limit.
    // \param pVoltageMV a place to put the input voltage limit.
    // \return true if successful, otherwise false.
    bool getInputVoltageLimit (int32_t *pVoltageMV);

    /// Set the input current limit.  If the current drawn
    // goes above this limit then charging will be ramped down.
    // The limit does not take effect until enableInputLimits()
    // is called (default setting is disabled).
    // \param currentMA the input current limit, in milliAmps.
    //        Range is 100 mA to 3000 mA, default depends upon
    //        hardware configuration, see section 8.3.1.4.3 of
    //        the data sheet.
    // \return true if successful, otherwise false.
    bool setInputCurrentLimit (int32_t currentMA);

    /// Get the input current limit.
    // \param pCurrentMA a place to put the input current limit.
    // \return true if successful, otherwise false.
    bool getInputCurrentLimit (int32_t *pCurrentMA);

    /// Enable input voltage and current limits.
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool enableInputLimits (void);

    /// Remove any input voltage or current limits.
    // Default is disabled.
    // \return true if successful, otherwise false.
    bool disableInputLimits (void);

    /// Check whether input limits are enabled.
    // \return true if input limits are enabled, otherwise false.
    bool areInputLimitsEnabled (void);

    /// Set the thermal regulation threshold for the chip.
    // \param temperatureC the temperature in C.
    //        Values will be translated to the nearest (lower)
    //        of 60 C, 80 C, 100 C and 120 C, default 120 C.
    // \return true if successful, otherwise false.
    bool setChipThermalRegulationThreshold (int32_t temperatureC);

    /// Get the thermal regulation threshold for the chip.
    // \param pTemperatureC a place to put the temperature.
    // \return true if successful, otherwise false.
    bool getChipThermalRegulationThreshold (int32_t *pTemperatureC);
    
    /// Get the charger faults.
    // Note: as with all the other API functions here, this should
    // not be called from an interrupt function as the comms with the
    // chip over I2C will take too long.
    // \return a bit-map of that can be tested against ChargerFault.
    char getChargerFaults(void);
    
    /// Enable shipping mode.
    // In shipping mode the battery is disconnected from the system
    // to avoid leakage.  Default is disabled.
    // \return true if successful, otherwise false.
    bool enableShippingMode (void);

    /// Disable shipping mode.
    // In shipping mode the battery is disconnected from the system
    // to avoid leakage.  Default is disabled.
    // \return true if successful, otherwise false.
    bool disableShippingMode (void);

    /// Check whether shipping mode is enabled.
    // \return true if input limits are enabled, otherwise false.
    bool isShippingModeEnabled (void);

    /// Advanced function to read a register on the chip.
    // \param address the address to read from.
    // \param pValue a place to put the returned value.
    // \return true if successful, otherwise false.
    bool advancedGet(char address, char *pValue);

    /// Advanced function to set a register on the chip.
    // \param address the address to write to.
    // \param value the value to write.
    // \return true if successful, otherwise false.
    bool advancedSet(char address, char value);

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
