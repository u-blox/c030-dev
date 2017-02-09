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

/// Default prescaler value
#define BATTERY_GAUGE_LTC2943_PRESCALER_DEFAULT 4096

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

class BatteryGaugeLtc2943 {
public:

    /// ALCC usage.
    typedef enum {
        ALCC_OFF = 0,
        ALCC_CHARGE_COMPLETE_INPUT = 1,
        ALCC_ALERT_OUTPUT = 2,
        MAX_NUM_ALCCS = 3
    } Alcc;

    /// The alerts that ALCC can give when in "AL" mode.
    typedef enum {
        ALERT_NONE,
        ALERT_UNDERVOLTAGE_LOCKOUT,
        ALERT_TEMPERATURE,
        ALERT_VOLTAGE,
        ALERT_CURRENT,
        ALERT_CHARGE_LOW,
        ALERT_CHARGE_HIGH,
        ALERT_CHARGE_OVER_UNDER_FLOW,
        MAX_NUM_ALERTS
    } Alert;

    /// Constructor.
    BatteryGaugeLtc2943(void);
    /// Destructor.
    ~BatteryGaugeLtc2943(void);

    /// Initialise the LTC2943 chip.
    // \param pI2c a pointer to the I2C instance to use.
    // \param rSenseMOhm the value of the sense resistor being used, in milli Ohms.
    //\ param address 7-bit I2C address of the LiPo gauge chip.
    //\ param prescaler the prescaler value to use, valid values being 1, 4, 16, 64, 256, 1024 and 4096.
    //        Refer to page 11 of the LTC2943 data sheet for information on chosing the prescaler value
    //        for a given battery capacity and peak current.
    // \param alcc how to configure the Alert/Charge Complete pin, see page 10 of the LTC2943 data sheet.
    // \return true if successful, otherwise false.
    bool init (I2C * pI2c, int32_t rSenseMOhm, uint8_t address = BATTERY_GAUGE_LTC2943_ADDRESS,
               int32_t prescaler = BATTERY_GAUGE_LTC2943_PRESCALER_DEFAULT, Alcc alcc = ALCC_OFF);

    /// Switch on/off the battery capacity monitor
    // \param onNotOff true to begin monitoring battery capacity, false to stop.
    // \param isSlow set this to true to save power if the battery current is not fluctuating very much.
    // \return true if successful, otherwise false.
    bool setMonitor (bool onNotOff, bool isSlow = false);

    /// Read the temperature of the LTC2943 chip.  If monitoring is
    // active the temperature will be that of the last reading; if
    // monitoring is not active then a reading will be taken
    // immediately, which takes around 300 ms.
    // \param pTemperatureC place to put the temperature reading.
    // \return true if successful, otherwise false.
    bool getTemperature (int32_t *pTemperatureC);

    /// Read the voltage of the battery.  If monitoring is
    // active the voltage will be that of the last reading; if
    // monitoring is not active then a reading will be taken
    // immediately, which takes around 300 ms.
    // \param pVoltageMV place to put the voltage reading.
    // \return true if successful, otherwise false.
    bool getVoltage (int32_t *pVoltageMV);

    /// Read the current flowing through rSense.  The value will be
    // positive when the battery is charging and negative when it is
    // discharging.  If monitoring is active the current will be
    // that of the last reading; if monitoring is not active then a
    // reading will be taken immediately, which takes around 300 ms.
    // \param pCurrentMA place to put the current reading.
    // \return true if successful, otherwise false.
    bool getCurrent (int32_t *pCurrentMA);

    /// Tell the LTC2943 chip that charging is complete,
    // after which it can work out the remaining charge.
    // \return true if successful, otherwise false.
    bool setChargingComplete ();
        
    /// Read the remaining available battery energy.
    // NOTE: this function can only return an accurate value
    // if the battery has been fully charged and the setChargingComplete()
    // function called since the LTC2943 chip was last physically reset.
    // If this is not the case the pCapacityMAh value will be filled in
    // from the data available but the return value will be set to false.
    // \param pCapacityMAh place to put the capacity reading.
    // \return true if successful, otherwise false.
    bool getRemainingCapacity (int32_t *pCapacityMAh);

    /// Read the state of charge of the battery as a percentage.
    // NOTE: this function can only return an accurate value
    // if the battery has been fully charged and the setChargingComplete()
    // function called since the LTC2943 chip was last physically reset.
    // If this is not the case the pBatteryPercent value will be filled in
    // from the data available but the return value will be set to false.
    // \param pBatteryPercent place to put the reading.
    // \return true if successful, otherwise false.
    bool getRemainingPercentage (int32_t *pBatteryPercent);
    
    /// Get the reason for an alert.
    // \return the Alert reason.
    Alert getAlertReason ();
        
    /// Set temperature alert upper threshold.
    // \param temperatureC the value to set.
    // \return true if successful, otherwise false.
    bool setTemperatureHigh(int32_t temperatureC);

    /// Get temperature alert upper threshold.
    // \param pTemperatureC place to put the temperature threshold.
    // \return true if successful, otherwise false.
    bool getTemperatureHigh(int32_t *pTemperatureC);

    /// Clear temperature alert upper threshold.
    // \return true if successful, otherwise false.
    bool clearTemperatureHigh();

    /// Set temperature alert lower threshold.
    // \param temperatureC the value to set.
    // \return true if successful, otherwise false.
    bool setTemperatureLow(int32_t temperatureC);
    
    /// Get temperature alert lower threshold.
    // \param pTemperatureC place to put the temperature threshold.
    // \return true if successful, otherwise false.
    bool getTemperatureLow(int32_t *pTemperatureC);

    /// Clear temperature alert lower threshold.
    // \return true if successful, otherwise false.
    bool clearTemperatureLow();

    /// Set voltage alert upper threshold.
    // \param voltageMV the value to set.
    // \return true if successful, otherwise false.
    bool setVoltageHigh(int32_t voltageMV);
    
    /// Get voltage alert upper threshold.
    // \param pVoltageMV place to put the voltage threshold.
    // \return true if successful, otherwise false.
    bool getVoltageHigh(int32_t *pVoltageMV);

    /// Clear voltage alert upper threshold.
    // \return true if successful, otherwise false.
    bool clearVoltageHigh();

    /// Set voltage alert lower threshold.
    // \param voltageMV the value to set.
    // \return true if successful, otherwise false.
    bool setVoltageLow(int32_t voltageMV);
    
    /// Get voltage alert lower threshold.
    // \param pVoltageMV place to put the voltage threshold.
    // \return true if successful, otherwise false.
    bool getVoltageLow(int32_t *pVoltageMV);

    /// Clear voltage alert lower threshold.
    // \return true if successful, otherwise false.
    bool clearVoltageLow();

    /// Set current alert upper threshold.
    // \param currentMA the value to set.
    // \return true if successful, otherwise false.
    bool setCurrentHigh(int32_t currentMA);

    /// Get current alert upper threshold.
    // \param pCurrentMA place to put the current threshold.
    // \return true if successful, otherwise false.
    bool getCurrentHigh(int32_t *pCurrentMA);
    
    /// Clear current alert upper threshold.
    // \return true if successful, otherwise false.
    bool clearCurrentHigh();

    /// Set current alert lower threshold.
    // \param currentMA the value to set.
    // \return true if successful, otherwise false.
    bool setCurrentLow(int32_t currentMA);
    
    /// Get current alert lower threshold.
    // \param pCurrentMA place to put the current threshold.
    // \return true if successful, otherwise false.
    bool getCurrentLow(int32_t *pCurrentMA);
    
    /// Clear current alert lower threshold.
    // \return true if successful, otherwise false.
    bool clearCurrentLow();

    /// Set capacity alert upper threshold.
    // \param capacityMAh the value to set.
    // \return true if successful, otherwise false.
    bool setCapacityHigh(int32_t capacityMAh);

    /// Get capacity alert upper threshold.
    // \param pCapacityMAh place to put the capacity threshold.
    // \return true if successful, otherwise false.
    bool getCapacityHigh(int32_t *pCapacityMAh);

    /// Clear capacity alert upper threshold.
    // \return true if successful, otherwise false.
    bool clearCapacityHigh();

    /// Set capacity alert lower threshold.
    // \param capacityMAh the value to set.
    // \return true if successful, otherwise false.
    bool setCapacityLow(int32_t capacityMAh);
    
    /// Get capacity alert lower threshold.
    // \param pCapacityMAh place to put the capacity threshold.
    // \return true if successful, otherwise false.
    bool getCapacityLow(int32_t *pCapacityMAh);

    /// Clear capacity alert lower threshold.
    // \return true if successful, otherwise false.
    bool clearCapacityLow();

protected:
    /// Pointer to the I2C interface.
    I2C * gpI2c;
    // Value of RSense.
    int32_t gRSenseMOhm;
    // The address of the device.
    uint8_t gAddress;
    // Flag to indicate device is ready.
    bool gReady;
    // The charge (this value will take into account
    // wraps) of the accumulated charge register.
    int32_t gCharge;
    // Flag to indicate that we are aware of the fully
    // charged battery state and hence can report
    // the remaining charge.
    bool gRemainingChargeKnown;

    /// Read two bytes starting at a given address.
    // \param registerAddress the register address to start reading from.
    // \param pBytes place to put the two bytes.
    // \return true if successful, otherwise false.
    bool getTwoBytes (uint8_t registerAddress, uint16_t *pBytes);
    
    /// Make sure that an ADC reading has been performed recently.
    // \return true if successful, otherwise false.
    bool makeAdcReading (void);
    
    /// Convert a 16 bit register reading into a temperature reading in C
    // \param data the register reading.
    // \return the temperature reading in C.
    int32_t registerToTemperatureC (uint16_t data);

    /// Convert a 16 bit register reading into a voltage reading in mV
    // \param data the register reading.
    // \return the voltage reading in mV.
    int32_t registerToVoltageMV (uint16_t data);

    /// Convert a 16 bit register reading into a current reading in mA
    // \param data the register reading.
    // \param rSenseMOhm the rSense value.
    // \return the current reading in mA.
    int32_t registerToCurrentMA (uint16_t data, int32_t rSenseMOhm);
};

#endif

// End Of File
