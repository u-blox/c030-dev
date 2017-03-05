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

#ifndef LOW_POWER_H
#define LOW_POWER_H

#include <WakeUp.h>

/**
 * @file low_power.h
 * This file defines a class intended to assist with obtaining lowest power
 * operation on an STM32F437 microprocessor.
 */

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

/// Macro to force a variable into backup SRAM.
// Place this on the line preceding the declaration of a variable 
// if that variable must be retained when the processor is in Standby
// mode.  For example:
//
// BACKUP_SRAM
// uint32_t importantThing;
//
// Note that variables marked in this way cannot be statically
// initialised, their initial values at cold-start will always be zero,
// i.e. even with the following declaration importantThing will
// have a value of 0 when it is first accessed:
//
// BACKUP_SRAM
// uint32_t importantThing = 3;
#define BACKUP_SRAM __attribute__ ((section ("BKPSRAM")))

// ----------------------------------------------------------------
// CLASSES
// ----------------------------------------------------------------

/// Low power library.
// Note: as it handles a hardware resource, there can only be one
// instance of this class; it is best to instantiate it statically
// at the top of your code or it can be instantiated once at the
// top of main().
class LowPower: public WakeUp  {
public:

    /// Constructor.
    LowPower(void);
    /// Destructor.
    ~LowPower(void);

    /// Enter Stop mode.
    // \param stopPeriodMilliseconds the amount of time to remain in Stop
    //        mode for.  When the time has expired the function
    //        will return.  The maximum delay is one calender month.
    //        It is up to the caller to ensure that the requested
    //        sleep time does not overflow the number of days in the
    //        current calender month.
    // \param disableUserInterrupts if true, all user interrupts aside from
    //        the RTC interrupt will be disabled, otherwise it is up to
    //        the caller to disable any interrupts which should not cause
    //        exit from Stop mode.  If this is set to true and it is not
    //        possible to disable interrupts on your chosen platform then
    //        this function will return false immediately. Note: during
    //        Stop mode the processor is running from a 32 kHz clock
    //        and so any interrupt that is triggered will run correspondingly
    //        slower.
    // \return true if successful, otherwise false.
    bool enterStop(uint32_t stopPeriodMilliseconds, bool disableUserInterrupts = false);


    /// Enter Standby mode.  Note that this function does NOT return.  Or
    // rather, if this function returns, there has been an error.
    // \param standbyPeriodMilliseconds the amount of time to remain in Standby
    //        mode for.  When the time has expired the processor will
    //        be reset and begin execution once more from main().  The
    //        values stored in BACKUP_SRAM will be retained, all other
    //        variables will be reset to their initial state. The RTOS
    //        is suspended on entry to Standby mode (i.e. no RTOS timers
    //        will expire) and the RTOS will be reset on return to main().
    //        The maximum delay is one calender month.
    //        It is up to the caller to ensure that the requested
    //        sleep time does not overflow the number of days in the
    //        current calender month.
    // \param disableUserInterrupts if true, all user interrupts aside from
    //        the RTC interrupt will be disabled, otherwise it is up to
    //        the caller to disable any interrupts which should not cause
    //        exit from Standby mode.  If this is set to true and it is not
    //        possible to disable interrupts on your chosen platform then
    //        this function will return false immediately. Note: during
    //        Standby mode the processor is running from a 32 kHz clock
    //        and so any interrupt that is triggered will run correspondingly
    //        slower.
    // \param powerDownBackupSram if true, backup SRAM will also be powered
    //        down in standby mode, otherwise it will be retained.
    void enterStandby(uint32_t standbyPeriodMilliseconds, bool disableUserInterrupts = false, bool powerDownBackupSram = false);

protected:

    /// Check whether an interrupt is enabled or not.  
    // Note: available in CMSIS 5 but not earlier.  The version here is for
    // an M4 core.
    inline uint32_t myNVIC_GetEnableIRQ(IRQn_Type IRQn);
    
    // Disable the user interrupts.  The given number of user interrupts,
    // counting from zero, will be disabled and, if pInterruptsActive is
    // provided, it will be filled in with true for those user interrupts 
    // that were active.
    // \param pInterruptsActive pointer to an array of bools that will be
    //        filled with true were an interrupt was active, othewise false.
    //        If parameter is NULL, disabling of the given number of user
    //        interrupts will still occur.  There must be space for at least
    //        numInterruptsActive bools.
    // \param numInterruptsActive the number of user interrupts to disable.
    // \return the number of interrupts that were disabled.
    uint32_t disableInterrupts(bool *pInterruptsActive, uint32_t numInterruptsActive);
};

#endif

// End Of File
