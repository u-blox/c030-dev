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
#include <battery_charger_bq24295.h>

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

/// Read from a register.
bool BatteryChargerBq24295::getRegister(char address, char *pValue)
{
    bool success = false;

    if (gpI2c != NULL) {
        // Move the address pointer
        if (gpI2c->write(gAddress, &address, 1) == 0) {
            if (pValue != NULL) {
               // Read from the address
                if (gpI2c->read(gAddress, pValue, 1) == 0) {
                    success = true;
#ifdef DEBUG_BQ24295
                    printf("BatteryChargerBq24295 (I2C 0x%02x): read 0x%02x from register 0x%02x.\r\n", gAddress >> 1, *pValue, address);
#endif
                }
            } else {
                success = true;
            }
        }
    }

    return success;
}

/// Write to a register.
bool BatteryChargerBq24295::setRegister(char address, char value)
{
    bool success = false;
    char data[2];

    if (gpI2c != NULL) {
        data[0] = address;
        data[1] = value;
        if (gpI2c->write(gAddress, &(data[0]), 2) == 0) {
            success = true;
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): wrote 0x%02x to register 0x%02x.\r\n", gAddress >> 1, value, address);
#endif
        }
    }

    return success;
}

/// Set a mask of bits in a register.
bool BatteryChargerBq24295::setRegisterBits(char address, char mask)
{
    bool success = false;
    char value;

    if (getRegister(address, &value)) {
        value |= mask;
        if (setRegister(address, value)) {
            success = true;
        }
    }

    return success;
}

/// Clear a mask of bits in a register.
bool BatteryChargerBq24295::clearRegisterBits(char address, char mask)
{
    bool success = false;
    char value;

    if (getRegister(address, &value)) {
        value &= ~mask;
        if (setRegister(address, value)) {
            success = true;
        }
    }

    return success;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
BatteryChargerBq24295::BatteryChargerBq24295(void)
{
    gpI2c = NULL;
    gReady = false;
}

/// Destructor.
BatteryChargerBq24295::~BatteryChargerBq24295(void)
{
}

/// Initialise ourselves.
bool BatteryChargerBq24295::init (I2C * pI2c, uint8_t address)
{
    char reg;

    gpI2c = pI2c;
    gAddress = address << 1;

    if (gpI2c != NULL) {
        gpI2c->lock();
        
        // Read the revision status register
        if (getRegister(0x0a, &reg)) {
            // The expected response is 0xc0
            if (reg == 0xc0) {
                gReady = true;
            }
        }
        gpI2c->unlock();
    }

#ifdef DEBUG_BQ24295
    if (gReady) {
        printf("BatteryChargerBq24295 (I2C 0x%02x): handler initialised.\r\n", gAddress >> 1);
    } else {
        printf("BatteryChargerBq24295 (I2C 0x%02x): chip NOT initialised.\r\n", gAddress >> 1);
    }
#endif

    return gReady;
}

/// Get the charge state.
BatteryChargerBq24295::ChargerState BatteryChargerBq24295::getChargerState(void)
{
    BatteryChargerBq24295::ChargerState chargerState = CHARGER_STATE_UNKNOWN;
    char powerOnConfiguration;
    char systemStatus;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the power-on configuration register
        if (getRegister(0x01, &powerOnConfiguration)) {
            // Read the system status register
            if (getRegister(0x08, &systemStatus)) {
                // Check the charge enable bit
                if ((powerOnConfiguration & (1 << 4)) == 0) {
                    chargerState = CHARGER_STATE_DISABLED;
#ifdef DEBUG_BQ24295
                    printf("BatteryChargerBq24295 (I2C 0x%02x): charging is disabled.\r\n", gAddress >> 1);
#endif
                } else {
                    // BatteryCharger is not disabled, so see if we have
                    // external power (bit 3)
                    if ((systemStatus & 0x04) == 0) {
                        chargerState = CHARGER_STATE_NO_EXTERNAL_POWER;
#ifdef DEBUG_BQ24295
                        printf("BatteryChargerBq24295 (I2C 0x%02x): no external power.\r\n", gAddress >> 1);
#endif
                    } else {
                        // Have power, so see how we're cooking (bits 4 & 5)
                        switch ((systemStatus >> 4) & 0x03) {
                            case 0:
                                chargerState = CHARGER_STATE_NOT_CHARGING;
#ifdef DEBUG_BQ24295
                                printf("BatteryChargerBq24295 (I2C 0x%02x): not charging.\r\n", gAddress >> 1);
#endif
                            break;
                            case 1:
                                chargerState = CHARGER_STATE_PRECHARGE;
#ifdef DEBUG_BQ24295
                                printf("BatteryChargerBq24295 (I2C 0x%02x): pre-charge.\r\n", gAddress >> 1);
#endif
                            break;
                            case 2:
                                chargerState = CHARGER_STATE_FAST_CHARGE;
#ifdef DEBUG_BQ24295
                                printf("BatteryChargerBq24295 (I2C 0x%02x): fast charge.\r\n", gAddress >> 1);
#endif
                            break;
                            case 3:
                                chargerState = CHARGER_STATE_COMPLETE;
#ifdef DEBUG_BQ24295
                                printf("BatteryChargerBq24295 (I2C 0x%02x): charging complete.\r\n", gAddress >> 1);
#endif
                            break;
                            default:
                            break;
                        }
                    }
                }
            }
        }
        gpI2c->unlock();
    }

    return chargerState;
}

/// Get whether external power is present or not.
bool BatteryChargerBq24295::isExternalPowerPresent(void)
{
    bool isPresent = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the system status register
        if (getRegister(0x08, &reg)) {
           // See if we have external power (bit 3)
            if ((reg & 0x04) != 0) {
                isPresent = true;
            }

#ifdef DEBUG_BQ24295
            if (isPresent) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): external power is present.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): external power is NOT present.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }

    return isPresent;
}

/// Enable charging.
bool BatteryChargerBq24295::enableCharging (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Charge enable is bit 4 of the power-on configuration register
        success = setRegisterBits(0x01, (1 << 4));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): charging now ENABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }
    
    return success;
}

/// Disable charging.
bool BatteryChargerBq24295::disableCharging (void)
{
    bool success = false;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Charge enable is bit 4 of the power-on configuration register
        success = clearRegisterBits(0x01, (1 << 4));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): charging now DISABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }
    
    return success;
}

/// Get the whether charging is enabled or disabled.
bool BatteryChargerBq24295::isChargingEnabled (void)
{
    bool isEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the power-on configuration register
        if (getRegister(0x01, &reg)) {
            // Charge enable is bit 4 of the power-on configuration register
            if ((reg & (1 << 4)) != 0) {
                isEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (isEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging is ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging is DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }
    
    return isEnabled;
}

/// Enable OTG charging.
bool BatteryChargerBq24295::enableOtg (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // OTG enable is bit 5 of the power-on configuration register
        success = setRegisterBits(0x01, (1 << 5));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): OTG charging now ENABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }
    
    return success;
}

/// Disable OTG charging.
bool BatteryChargerBq24295::disableOtg (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // OTG enable is bit 5 of the power-on configuration register
        success = clearRegisterBits(0x01, (1 << 5));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): OTG charging now DISABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }
    
    return success;
}

/// Get whether OTG charging is enabled or not.
bool BatteryChargerBq24295::isOtgEnabled (void)
{
    bool isEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the power-on configuration register
        if (getRegister(0x01, &reg)) {
            // OTG enable is bit 5 of the power-on configuration register
            if ((reg & (1 << 5)) != 0) {
                isEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (isEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): OTG charging is ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): OTG charging is DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }
    
    return isEnabled;
}

/// Set the system voltage.
bool BatteryChargerBq24295::setSystemVoltage (int32_t voltageMV)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the power-on configuration register
        if (getRegister(0x01, &reg)) {
            // System voltage is in bits 1 to 3,
            // coded to base "100 mV" with
            // an offset of 3000 mV.
            if ((voltageMV >= 3000) && (voltageMV <= 3700)) {
                codedValue = voltageMV;
                codedValue = (codedValue - 3000) / 100;
                // If the voltage minus the base is not an exact multiple of 100,
                // add one to the coded value to make sure we don't
                // go under the requested system voltage
                if ((voltageMV - 3000) % 100 != 0) {
                    codedValue++;
                }
                
                reg &= ~(0x07 << 1);
                reg |= (char) ((codedValue & 0x07) << 1);
                
                // Write to the power-on configuration register
                success = setRegister (0x01, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): system voltage now set to %.3f V.\r\n", gAddress >> 1, (float) voltageMV / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the system voltage.
bool BatteryChargerBq24295::getSystemVoltage (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the power-on configuration register
        if (getRegister(0x01, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                // Input voltage limit is in bits 1 to 3
                // Base voltage
                *pVoltageMV = 3000;
                // Shift reg down and add the number of multiples
                // of 100 mV
                reg = (reg >> 1) & 0x07;
                *pVoltageMV += ((int32_t) reg) * 100;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): system voltage is %.3f V.\r\n", gAddress >> 1, (float) *pVoltageMV / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the fast charging current limit.
bool BatteryChargerBq24295::setFastChargingCurrentLimit (int32_t currentMA)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charge current control register
        if (getRegister(0x02, &reg)) {
            // Fast charging current limit is in
            // bits 2 to 7, coded to base "64 mA" with
            // an offset of 512 mA.
            if ((currentMA >= 512) && (currentMA <= 3008)) {
                codedValue = currentMA;
                codedValue = (codedValue - 512) / 64;
                
                reg &= ~(0x3f << 2);
                reg |= (char) ((codedValue & 0x3f) << 2);

                // Write to the charge current control register
                success = setRegister (0x02, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): fast charging current limit now set to %.3f A.\r\n", gAddress >> 1, (float) currentMA / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the fast charging current limit.
bool BatteryChargerBq24295::getFastChargingCurrentLimit (int32_t *pCurrentMA)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charge current control register
        if (getRegister(0x02, &reg)) {
            success = true;
            if (pCurrentMA != NULL) {
                // Fast charging current limit is in bits 2 to 7
                // Base current
                *pCurrentMA = 512;
                // Shift reg down and add the number of multiples
                // of 64 mA
                reg = (reg >> 2) & 0x3f;
                *pCurrentMA += ((int32_t) reg) * 64;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): fast charge current limit is %.3f A.\r\n", gAddress >> 1, (float) *pCurrentMA / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Enable the ICHG/IPRECH margin.
bool BatteryChargerBq24295::enableIcghIprechMargin (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // FORCE_20PCT is bit 0 of the charge current control register
        success = setRegisterBits(0x02, (1 << 0));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): ICGH/IPRECH margin now ENABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Disable the ICHG/IPRECH margin.
bool BatteryChargerBq24295::disableIcghIprechMargin (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // FORCE_20PCT is bit 0 of the charge current control register
        success = clearRegisterBits(0x02, (1 << 0));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): ICGH/IPRECH margin now DISABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Check if the ICHG/IPRECH margin is set.
bool BatteryChargerBq24295::isIcghIprechMarginEnabled (void)
{
    bool isEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charge current control register
        if (getRegister(0x02, &reg)) {
            // FORCE_20PCT is bit 0
            if ((reg & (1 << 0)) != 0) {
                isEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (isEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): ICGH/IPRECH margin is ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): ICGH/IPRECH margin is DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }

    return isEnabled;
}
    
/// Set the fast charging safety timer.
bool BatteryChargerBq24295::setFastChargingSafetyTimer (int32_t timerHours)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Mustn't be silly
        if (timerHours >= 0) {
            // Read the charge termination/timer control register
            if (getRegister(0x05, &reg)) {
                // Timer setting is in bits 1 & 2, enable is in bit 3
                if (timerHours == 0) {
                    reg &= ~(1 << 3);
                } else {
                    reg |= (1 << 3);
                    if (timerHours < 8) {
                        codedValue = 0;
                    } else if (timerHours < 12) {
                        codedValue = 1;
                    } else if (timerHours < 20) {
                        codedValue = 2;
                    } else {
                        codedValue = 3;
                    }
                    reg &= ~(0x03 << 1);
                    reg |= (char) (codedValue << 1);
                }
                // Write to the charge termination/timer control register
                success = setRegister (0x05, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): fast charging safety timer now set to %d hours.\r\n", gAddress >> 1, timerHours);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the value of the fast charging safety timer.
bool BatteryChargerBq24295::getFastChargingSafetyTimer (int32_t *pTimerHours)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charge termination/timer control register
        if (getRegister(0x05, &reg)) {
            success = true;
            if (pTimerHours != NULL) {
                *pTimerHours = 0;
                // Timer enable is in bit 3
                if ((reg & (1 << 3)) != 0) {
                    // Timer value is in bits 1 & 2
                    switch ((reg >> 1) & 0x03) {
                        case 0:
                            *pTimerHours = 5;
                        break;
                        case 1:
                            *pTimerHours = 8;
                        break;
                        case 2:
                            *pTimerHours = 12;
                        break;
                        case 3:
                            *pTimerHours = 20;
                        break;
                        default:
                            MBED_ASSERT(false);
                        break;
                    }
                }
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): fast charging safety timer is %d hours.\r\n", gAddress >> 1, *pTimerHours);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the charging termination current.
bool BatteryChargerBq24295::setChargingTerminationCurrent (int32_t currentMA)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the pre-charge/termination current control register
        if (getRegister(0x03, &reg)) {
            // Charging termination current limit is in
            // bits 0 to 3, coded to base "128 mA" with
            // an offset of 128 mA.
            if ((currentMA >= 128) && (currentMA <= 2048)) {
                codedValue = currentMA;
                codedValue = (codedValue - 128) / 128;
                
                reg &= ~0x0f;
                reg |= (char) (codedValue & 0x0f);

                // Write to the pre-charge/termination current control register
                success = setRegister (0x03, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination current now set to %.3f A.\r\n", gAddress >> 1, (float) currentMA / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the charging termination current.
bool BatteryChargerBq24295::getChargingTerminationCurrent (int32_t *pCurrentMA)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the pre-charge/termination current control register
        if (getRegister(0x03, &reg)) {
            success = true;
            if (pCurrentMA != NULL) {
                // Pre-charging current limit is in bits 0 to 3
                // Base current
                *pCurrentMA = 128;
                // Add the number of multiples of 128 mA
                reg = reg & 0x0f;
                *pCurrentMA += ((int32_t) reg) * 128;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination current is %.3f A.\r\n", gAddress >> 1, (float) *pCurrentMA / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Enable charging termination.
bool BatteryChargerBq24295::enableChargingTermination (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // EN_TERM is bit 7 of the charge termination/timer control register
        success = setRegisterBits(0x05, (1 << 7));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination now ENABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Disable charging termination.
bool BatteryChargerBq24295::disableChargingTermination (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // EN_TERM is bit 7 of the charge termination/timer control register
        success = clearRegisterBits(0x05, (1 << 7));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination now DISABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Get the state of charging termination (enabled or disabled)
bool BatteryChargerBq24295::isChargingTerminationEnabled (void)
{
    bool isEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charge termination/timer control register
        if (getRegister(0x05, &reg)) {
            // EN_TERM is bit 7
            if ((reg & (1 << 7)) != 0) {
                isEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (isEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination is ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging termination is DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }

    return isEnabled;
}

/// Set the pre-charging current limit.
bool BatteryChargerBq24295::setPrechargingCurrentLimit (int32_t currentMA)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the pre-charge/termination current control register
        if (getRegister(0x03, &reg)) {
            // Pre-charging current limit is in
            // bits 4 to 7, coded to base "128 mA" with
            // an offset of 128 mA.
            if ((currentMA >= 128) && (currentMA <= 2048)) {
                codedValue = currentMA;
                codedValue = (codedValue - 128) / 128;
                
                reg &= ~(0x0f << 4);
                reg |= (char) ((codedValue & 0x0f) << 4);

                // Write to the pre-charge/termination current control register
                success = setRegister (0x03, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): pre-charging current limit now set to %.3f A.\r\n", gAddress >> 1, (float) currentMA / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the pre-charging current limit.
bool BatteryChargerBq24295::getPrechargingCurrentLimit (int32_t *pCurrentMA)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the pre-charge/termination current control register
        if (getRegister(0x03, &reg)) {
            success = true;
            if (pCurrentMA != NULL) {
                // Pre-charging current limit is in bits 4 to 7
                // Base current
                *pCurrentMA = 128;
                // Shift reg down and add the number of multiples
                // of 128 mA
                reg = (reg >> 4) & 0x0f;
                *pCurrentMA += ((int32_t) reg) * 128;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): pre-charging current limit is %.3f A.\r\n", gAddress >> 1, (float) *pCurrentMA / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the charging voltage limit.
bool BatteryChargerBq24295::setChargingVoltageLimit (int32_t voltageMV)
{
    bool success = false;
    char reg;
    int32_t codedLimit;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charging voltage control register
        if (getRegister(0x04, &reg)) {
            // Charging voltage limit is in bits 2 to 7
            // but it is coded to base "16 mV" with
            // an offset of 3504 mV.
            if ((voltageMV >= 3504) && (voltageMV <= 4400)) {
                codedLimit = voltageMV;
                codedLimit = (codedLimit - 3504) / 16;
                
                reg &= ~(0x3f << 2);
                reg |= (char) ((codedLimit & 0x3f) << 2);
                
               // Write to the charging voltage control register
                success = setRegister (0x04, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): charging voltage limit now set to %.3f V.\r\n", gAddress >> 1, (float) voltageMV / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the charging voltage limit.
bool BatteryChargerBq24295::getChargingVoltageLimit (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the charging voltage control register
        if (getRegister(0x04, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                // Charging voltage limit is in bits 2 to 7
                // Base voltage
                *pVoltageMV = 3504;
                // Shift reg down and add the number of multiples
                // of 16 mV
                reg = (reg >> 2) & 0x3f;
                *pVoltageMV += ((int32_t) reg) * 16;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): charging voltage limit is %.3f V.\r\n", gAddress >> 1, (float) *pVoltageMV / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the pre-charge to fast-charge voltage threshold.
bool BatteryChargerBq24295::setFastChargingVoltageThreshold (int32_t voltageMV)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // There are only two possible values, 2.8 V and 3.0 V.
        // BATLOWV is bit 1 of the charging voltage control register
        if (voltageMV > 2800) {
            success = setRegisterBits(0x04, (1 << 1));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold now set to 3.000 V.\r\n", gAddress >> 1);
#endif
        } else {
            success = clearRegisterBits(0x04, (1 << 1));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold now set to 2.800 V.\r\n", gAddress >> 1);
#endif
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the pre-charge to fast-charge voltage threshold.
bool BatteryChargerBq24295::getFastChargingVoltageThreshold (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // BATLOWV is bit 1 of the charging voltage control register
        if (getRegister(0x04, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                *pVoltageMV = 2800;
                if ((reg & (1 << 1)) != 0) {
                    *pVoltageMV = 3000;
                }
            }
        }
        
#ifdef DEBUG_BQ24295
        if (reg & (1 << 1)) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold is 3.000 V.\r\n", gAddress >> 1);
        } else {
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold is 2.800 V.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Set the recharging voltage threshold.
bool BatteryChargerBq24295::setRechargingVoltageThreshold (int32_t voltageMV)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // There are only two possible values, 100 mV and 300 mV.
        // VRECHG is bit 0 of the charging voltage control register
        if (voltageMV > 100) {
            success = setRegisterBits(0x04, (1 << 0));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold now set to 0.300 V.\r\n", gAddress >> 1);
#endif
        } else {
            success = clearRegisterBits(0x04, (1 << 0));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold now set to 0.100 V.\r\n", gAddress >> 1);
#endif
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the recharging voltage threshold.
bool BatteryChargerBq24295::getRechargingVoltageThreshold (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // VRECHG is bit 0 of the charging voltage control register
        if (getRegister(0x04, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                *pVoltageMV = 100;
                if ((reg & (1 << 0)) != 0) {
                    *pVoltageMV = 300;
                }
            }
        }
        
#ifdef DEBUG_BQ24295
        if (reg & (1 << 1)) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold is 0.300 V.\r\n", gAddress >> 1);
        } else {
            printf("BatteryChargerBq24295 (I2C 0x%02x): recharge voltage threshold is 0.100 V.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Set the boost voltage.
bool BatteryChargerBq24295::setBoostVoltage (int32_t voltageMV)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            // Boost voltage is in bits 4 to 7, coded to base "64 mV"
            // with an offset of 4550 mV.
            if ((voltageMV >= 4550) && (voltageMV <= 5510)) {
                codedValue = voltageMV;
                codedValue = (codedValue - 4550) / 64;
                // If the voltage minus the base is not an exact multiple of 64,
                // add one to the coded value to make sure we don't
                // go under the requested boost voltage
                if ((voltageMV - 4550) % 64 != 0) {
                    codedValue++;
                }
                
                reg &= ~(0x0f << 4);
                reg |= (char) ((codedValue & 0x0f) << 4);
                
                // Write to the boost voltage/thermal regulation control register
                success = setRegister (0x06, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): boost voltage now set to %.3f V.\r\n", gAddress >> 1, (float) voltageMV / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }
    
    return success;
}

/// Get the boost voltage.
bool BatteryChargerBq24295::getBoostVoltage (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                // Boost voltage is in bits 4 to 7
                // Base voltage
                *pVoltageMV = 4550;
                // Shift reg down and add the number of multiples
                // of 64 mV
                reg = (reg >> 4) & 0x0f;
                *pVoltageMV += ((int32_t) reg) * 64;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): boost voltage is %.3f V.\r\n", gAddress >> 1, (float) *pVoltageMV / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the boost mode low temperature limit.
bool BatteryChargerBq24295::setBoostUpperTemperatureLimit (int32_t temperatureC)
{
    bool success = false;
    char reg;
    char codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            // BHOT is in bits 2 & 3
            if (temperatureC < 60) {
                codedValue = 0;
            } else if (temperatureC < 65) {
                codedValue = 1;
            } else {
                codedValue = 2;
            }
            reg &= ~(0x03 << 2);
            reg |= (char) (codedValue << 2);
            // Write to boost voltage/thermal regulation control register
            success = setRegister (0x06, reg);
#ifdef DEBUG_BQ24295
            if (success) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode lower temperature limit now set to %d C.\r\n", gAddress >> 1, temperatureC);
            }
#endif
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the boost mode upper temperature limit.
bool BatteryChargerBq24295::getBoostUpperTemperatureLimit (int32_t *pTemperatureC)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // BHOT is in bits 2 & 3 of the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            // Only proceed (and return true) if the limit is enabled
            if (((reg >> 2) & 0x03) != 0x03) {
                success = true;
                if (pTemperatureC != NULL) {
                    switch ((reg >> 2) & 0x03) {
                        case 0:
                            *pTemperatureC = 55;
                        break;
                        case 1:
                            *pTemperatureC = 60;
                        break;
                        case 2:
                            *pTemperatureC = 65;
                        break;
                        default:
                            MBED_ASSERT(false);
                        break;
                    }
#ifdef DEBUG_BQ24295
                    printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode upper temperature limit is %d C.\r\n", gAddress >> 1, *pTemperatureC);
#endif
                }
            }
        }
        gpI2c->unlock();
    }

    return success;
}
    
/// Check whether the boost mode upper temperature limit is enabled.
bool BatteryChargerBq24295::isBoostUpperTemperatureLimitEnabled (void)
{
    bool isEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // BHOT is in bits 2 & 3 of the boost voltage/thermal regulation control register
        // and it is enabled if any bit is 0
        if (getRegister(0x06, &reg)) {
            if (((reg >> 2) & 3) != 3) {
                isEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (isEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode upper temperature limit is ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode upper temperature limit is DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }

    return isEnabled;
}

/// Disable the boost mode upper temperature limit.
bool BatteryChargerBq24295::disableBoostUpperTemperatureLimit (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // BHOT is in bits 2 & 3 of the boost voltage/thermal regulation control register
        // and setting all the bits indicates disabled
        success = setRegisterBits(0x06, (3 << 2));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode upper temperature limit now DISABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Set the boost mode upper temperature limit.
bool BatteryChargerBq24295::setBoostLowerTemperatureLimit (int32_t temperatureC)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // There are only two possible values, -10 C and -20 C.
        // BCOLD is bit 1 of the charge current control register
        if (temperatureC < -10) {
            success = setRegisterBits(0x02, (1 << 1));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode lower temperature limit now set to -20 C.\r\n", gAddress >> 1);
#endif
        } else {
            success = clearRegisterBits(0x02, (1 << 1));
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode lower temperature limit now set to -10 C.\r\n", gAddress >> 1);
#endif
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the boost mode low temperature limit.
bool BatteryChargerBq24295::getBoostLowerTemperatureLimit (int32_t *pTemperatureC)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // BCOLD is bit 1 of the charge current control register
        if (getRegister(0x02, &reg)) {
            success = true;
            if (pTemperatureC != NULL) {
                *pTemperatureC = -10;
                if ((reg & (1 << 1)) != 0) {
                    *pTemperatureC = -20;
                }
            }
        }
        
#ifdef DEBUG_BQ24295
        if (reg & (1 << 1)) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode lower temperature limit is -20 C.\r\n", gAddress >> 1);
        } else {
            printf("BatteryChargerBq24295 (I2C 0x%02x): boost mode lower temperature limit is -10 C.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}
    
/// Set the input voltage limit.
bool BatteryChargerBq24295::setInputVoltageLimit (int32_t voltageMV)
{
    bool success = false;
    char reg;
    int32_t codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the input source control register
        if (getRegister(0x00, &reg)) {
            // Input voltage limit is in bits 3 to 6
            // but it is coded to base "80 mV" with
            // an offset of 3880 mV.
            if ((voltageMV >= 3880) && (voltageMV <= 5080)) {
                codedValue = voltageMV;
                codedValue = (codedValue - 3880) / 80;
                
                reg &= ~(0x0f << 3);
                reg |= (char) ((codedValue & 0x0f) << 3);
                
               // Write to the input source control register
                success = setRegister (0x00, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): input voltage limit now set to %.3f V.\r\n", gAddress >> 1, (float) voltageMV / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the input voltage limit.
bool BatteryChargerBq24295::getInputVoltageLimit (int32_t *pVoltageMV)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the input source control register
        if (getRegister(0x00, &reg)) {
            success = true;
            if (pVoltageMV != NULL) {
                // Input voltage limit is in bits 3 to 6
                // Base voltage
                *pVoltageMV = 3880;
                // Shift reg down and add the number of multiples
                // of 80 mV
                reg = (reg >> 3) & 0x0f;
                *pVoltageMV += ((int32_t) reg) * 80;
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): input voltage limit is %.3f V.\r\n", gAddress >> 1, (float) *pVoltageMV / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set the input current limit.
bool BatteryChargerBq24295::setInputCurrentLimit (int32_t currentMA)
{
    bool success = false;
    char reg;
    char codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the input source control register
        if (getRegister(0x00, &reg)) {
            // Input current limit is in bits 0 to 2, coded
            // such that the smallest limit is applied for
            // a range (e.g. 120 mA ends up as 100 mA rather
            // than 150 mA)
            if ((currentMA >= 100) && (currentMA <= 3000)) {
                if (currentMA < 150) {
                    codedValue = 0;
                } else if (currentMA < 500) {
                    codedValue = 1;
                } else if (currentMA < 900) {
                    codedValue = 2;
                } else if (currentMA < 1000) {
                    codedValue = 3;
                } else if (currentMA < 1500) {
                    codedValue = 4;
                } else if (currentMA < 2000) {
                    codedValue = 5;
                } else if (currentMA < 3000) {
                    codedValue = 6;
                } else {
                    codedValue = 7;
                }                
                reg &= ~(0x07 << 0);
                reg |= codedValue;
                
               // Write to the input source control register
                success = setRegister (0x00, reg);
#ifdef DEBUG_BQ24295
                if (success) {
                    printf("BatteryChargerBq24295 (I2C 0x%02x): input current limit now set to %.3f A.\r\n", gAddress >> 1, (float) currentMA / 1000);
                }
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the input current limit.
bool BatteryChargerBq24295::getInputCurrentLimit (int32_t *pCurrentMA)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the input source control register
        if (getRegister(0x00, &reg)) {
            success = true;
            if (pCurrentMA != NULL) {
                *pCurrentMA = 0;
                // Input current limit is in bits 0 to 2
                switch (reg & 0x07) {
                    case 0:
                        *pCurrentMA = 100;
                    break;
                    case 1:
                        *pCurrentMA = 150;
                    break;
                    case 2:
                        *pCurrentMA = 500;
                    break;
                    case 3:
                        *pCurrentMA = 900;
                    break;
                    case 4:
                        *pCurrentMA = 1000;
                    break;
                    case 5:
                        *pCurrentMA = 1500;
                    break;
                    case 6:
                        *pCurrentMA = 2000;
                    break;
                    case 7:
                        *pCurrentMA = 3000;
                    break;
                    default:
                        MBED_ASSERT(false);
                    break;
                }
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): input current limit is %.3f A.\r\n", gAddress >> 1, (float) *pCurrentMA / 1000);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Enable input voltage or current limits.
bool BatteryChargerBq24295::enableInputLimits (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Input limit enable is bit 7 of the source control register
        success = setRegisterBits(0x00, (1 << 7));
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): input limits now ENABLED.\r\n", gAddress >> 1);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Remove any input voltage or current limits.
bool BatteryChargerBq24295::disableInputLimits (void)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Input limit enable is bit 7 of the source control register
        success = clearRegisterBits(0x00, (1 << 7));
#ifdef DEBUG_BQ24295
            if (success) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): input limits now DISABLED.\r\n", gAddress >> 1);
            }
#endif
        gpI2c->unlock();
    }

    return success;
}

/// Check if input limits are enabled.
bool BatteryChargerBq24295::areInputLimitsEnabled (void)
{
    bool areEnabled = false;
    char reg;
    
    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the input source control register
        if (getRegister(0x00, &reg)) {
            // Input limit enable is bit 7 of the source control register
            if ((reg & (1 << 7)) != 0) {
                areEnabled = true;
            }
#ifdef DEBUG_BQ24295
            if (areEnabled) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): input limits are ENABLED.\r\n", gAddress >> 1);
            } else {
                printf("BatteryChargerBq24295 (I2C 0x%02x): input limits are DISABLED.\r\n", gAddress >> 1);
            }
#endif
        }
        gpI2c->unlock();
    }

    return areEnabled;
}

/// Set the thermal regulation threshold for the chip.
bool BatteryChargerBq24295::setChipThermalRegulationThreshold (int32_t temperatureC)
{
    bool success = false;
    char reg;
    char codedValue;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            // TREG is in bits 0 & 1
            if (temperatureC < 80) {
                codedValue = 0;
            } else if (temperatureC < 100) {
                codedValue = 1;
            } else if (temperatureC < 120) {
                codedValue = 2;
            } else {
                codedValue = 3;
            }
            reg &= ~0x03;
            reg |= (char) codedValue;
            // Write to boost voltage/thermal regulation control register
            success = setRegister (0x06, reg);
#ifdef DEBUG_BQ24295
            if (success) {
                printf("BatteryChargerBq24295 (I2C 0x%02x): chip thermal regulation threshold now set to %d C.\r\n", gAddress >> 1, temperatureC);
            }
#endif
        }
        gpI2c->unlock();
    }

    return success;
}

/// Get the thermal regulation threshold for the chip.
bool BatteryChargerBq24295::getChipThermalRegulationThreshold (int32_t *pTemperatureC)
{
    bool success = false;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        /// TREG is in bits 0 & 1 of the boost voltage/thermal regulation control register
        if (getRegister(0x06, &reg)) {
            success = true;
            if (pTemperatureC != NULL) {
                switch (reg & 0x03) {
                    case 0:
                        *pTemperatureC = 60;
                    break;
                    case 1:
                        *pTemperatureC = 80;
                    break;
                    case 2:
                        *pTemperatureC = 100;
                    break;
                    case 3:
                        *pTemperatureC = 120;
                    break;
                    default:
                        MBED_ASSERT(false);
                    break;
                }
#ifdef DEBUG_BQ24295
                printf("BatteryChargerBq24295 (I2C 0x%02x): chip thermal regulation threshold is %d C.\r\n", gAddress >> 1, *pTemperatureC);
#endif
            }
        }
        gpI2c->unlock();
    }

    return success;
}
    
/// Get the charger fault status as a bitmap.
char BatteryChargerBq24295::getChargerFaults(void)
{
    char bitmap = (char) CHARGER_FAULT_NONE;
    char reg;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the fault register
        if (getRegister(0x09, &reg)) {
            bitmap = reg;
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): charge fault register 0x%02x.\r\n", gAddress >> 1, bitmap);
#endif
        }
        gpI2c->unlock();
    }

    return bitmap;
}

/// Read a register on the chip.
// TODO test
bool BatteryChargerBq24295::advancedGet(char address, char *pValue)
{
    bool success = false;
    char value;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Read the register
        if (getRegister(address, &value)) {
            success = true;
#ifdef DEBUG_BQ24295
            printf("BatteryChargerBq24295 (I2C 0x%02x): read 0x%02x from address 0x%02x.\n", gAddress >> 1, value, address);
#endif
            if (pValue != NULL) {
                *pValue = value;
            }
        }
        gpI2c->unlock();
    }

    return success;
}

/// Set a register on the chip.
// TODO test
bool BatteryChargerBq24295::advancedSet(char address, char value)
{
    bool success = false;

    if (gReady && (gpI2c != NULL)) {
        gpI2c->lock();
        // Set the register
        success = setRegister (address, value);
#ifdef DEBUG_BQ24295
        if (success) {
            printf("BatteryChargerBq24295 (I2C 0x%02x): wrote 0x%02x  to address 0x%02x.\n", gAddress >> 1, value, address);
        }
#endif
        gpI2c->unlock();
    }

    return success;
}

// End Of File
