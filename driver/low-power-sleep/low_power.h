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
    // \param enableInterrupts if true, any interrupt can cause
    //        a wake-up from Stop mode, otherwise only the one
    //        interrupt required to wake from Stop mode will be
    //        enabled.  Note: during Stop mode the processor is
    //        running from 32 kHz crystal and so any timer
    //        interrupts will run correspondingly slower.
    // \return true if successful, otherwise false.
    bool enterStop(time_t stopPeriodSeconds, bool enableInterrupts = false);


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
    // \param powerDownBackupSram if true, backup SRAM will also be powered
    //        down in standby mode, otherwise it will be retained.
    void enterStandby(time_t standbyPeriodSeconds, bool powerDownBackupSram = false);

protected:

    /// The calendar year represented by our earliest date/time.
#   define BASE_YEAR 1968
    /// A limit placed on us by the RTC.
#   define YEAR_MAX 99

    /// A structure to hold a date/time
    // in a format that matches the fields
    // of the RTC hardware that this class
    // is interested in.
    typedef struct {
        uint32_t second;   ///< 0 to 59
        uint32_t minute;   ///< 0 to 59
        uint32_t hour;     ///< 0 to 23
        uint32_t day;      ///< 1 to 31, i.e. 1 based
        uint32_t month;    ///< 1 to 12, i.e. 1 based
        uint32_t year;     ///< 0 to YEAR_MAX where 0 = BASE_YEAR
    } DateTime_t;

    /// A copy of the code in the mbed deepsleep() function but with
    // flash powered down to save more power.
    // \return true the period for which we have slept in RTX units.
    uint32_t myDeepSleep(void);

    /// Add a period in seconds to a DateTime_t struct.
    // \param pDateTime a pointer to the DateTime_t struct.
    // \param periodSeconds the period to add to it.
    void addPeriod(LowPower::DateTime_t * pDateTime, time_t periodSeconds);

    /// Set alarm A on the RTC.
    // \param pDateTime a pointer to the DateTime_t struct containing the alarm time.
    // \return true if successful, otherwise false.
    bool setAlarmA(const LowPower::DateTime_t * pDateTime);

    /// Set an alarm for a number of seconds in the future.
    // \param periodSeconds the number of seconds in the future that the
    //        alarm should go off.
    // \return true if successful, otherwise false.
    bool setRtcAlarm(time_t periodSeconds);

    /// Return the number of days in a month, taking into account Feb.
    // \param month the month, between 1 and 12.
    // \param year the calendar year that the month is in
    //        (i.e. 2000 means the year 2000).
    // \return the number of days in the month.
    uint8_t daysInMonth(uint8_t month, uint32_t year);

    /// Determine if a given year is a leap year.
    // \param year the year.
    // \return true if it is a leap year, otherwise false.
    bool isLeapYear(uint32_t year);
};

#endif

// End Of File
