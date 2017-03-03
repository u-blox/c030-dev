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
class LowPower {
public:

    /// Constructor.
    LowPower(void);
    /// Destructor.
    ~LowPower(void);

    /// Enter Stop mode.
    // \param stopPeriodSeconds the amount of time to remain in Stop
    //        mode for.  When the time has expired the function
    //        will return.  The maximum delay is one calender month.
    //        It is up to the caller to ensure that the requested
    //        sleep time does not overflow the number of days in the
    //        current calender month.
    // \param disableInterrupts if true, all user interrupts aside from
    //        the RTC interrupt will be disabled, otherwise it is up to
    //        the caller to disable any interrupts which should not cause
    //        exit from Stop mode.  Note: during Stop mode the processor is
    //        running from a 32 kHz clock and so any interrupt that is
    //        triggered will run correspondingly slower.
    // \return true if successful, otherwise false.
    bool enterStop(time_t stopPeriodSeconds, bool disableInterrupts = false);


    /// Enter Standby mode.  Note that this function does NOT return.  Or
    // rather, if this function returns, there has been an error.
    // \param standbyPeriodSeconds the amount of time to remain in Standby
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
    // \param disableInterrupts if true, all user interrupts aside from
    //        the RTC interrupt will be disabled, otherwise it is up to
    //        the caller to disable any interrupts which should not cause
    //        exit from Standby mode.  Note: during Standby mode the processor
    //        is running from a 32 kHz clock and so any interrupt that is
    //        triggered will run correspondingly slower.
    // \param powerDownBackupSram if true, backup SRAM will also be powered
    //        down in standby mode, otherwise it will be retained.
    void enterStandby(time_t standbyPeriodSeconds, bool disableInterrupts = false, bool powerDownBackupSram = false);

protected:

    /// Set an alarm for a number of seconds in the future.
    // \param pAlarmStruct pointer to a tm struct representing the alarm time.
    // \return true if successful, otherwise false.
    bool setRtcAlarm(struct tm * pAlarmStruct);
};

#endif

// End Of File
