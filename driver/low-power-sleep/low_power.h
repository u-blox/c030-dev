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

/// Size of backup SRAM.
#ifdef TARGET_STM
#define BACKUP_SRAM_SIZE 4096
#else
#define BACKUP_SRAM_SIZE 0
#endif

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

    /// Exit debug mode.
    // On a standard mbed board the host microcontroller is held
    // in debug mode by the debug chip on the board.  When the
    // host microcontroller is in debug mode it cannot enter Standby
    // mode normally.  So to be able to use Standby mode, you
    // must call this as the VERY FIRST THING you do on entry
    // to main(); it will perform a soft reset of the microcontroller
    // to cut the debug mode connection with the debug chip.
    void exitDebugMode(void);

    /// Enter Stop mode.
    // \param stopPeriodMilliseconds the amount of time to remain in Stop
    //        mode for.  When the time has expired the function
    //        will return.  The maximum delay is one calender month.
    //        It is up to the caller to ensure that the requested
    //        sleep time does not overflow the number of days in the
    //        current calender month.  This function will disable the RTOS
    //        tick and so any RTOS timers will be frozen for the duration of
    //        Stop mode; they will not tick and will not expire during this
    //        time.  Any other enabled interrupts can bring the processor out of
    //        Stop mode.
    //        Note: during Stop mode the processor is running from a 32 kHz
    //        clock and so any interrupt that is triggered will run
    //        correspondingly slower.
    //        Note: if the watchdog is being used, the caller should set the
    //        watchdog timeout to longer period than the Stop period.
    void enterStop(uint32_t stopPeriodMilliseconds);


    /// Enter Standby mode.  Note that this function does NOT return.  Or
    // rather, if this function returns, there has been an error.
    // \param standbyPeriodMilliseconds the amount of time to remain in Standby
    //        mode for.  When the time has expired the processor will
    //        be reset and begin execution once more from main().  The
    //        values stored in BACKUP_SRAM will be retained, all other
    //        variables will be reset to their initial state. The RTOS
    //        is suspended on entry to Standby mode (i.e. no RTOS timers
    //        will run) and the RTOS will be reset on return to main().
    //        The maximum delay is one calender month.
    //        It is up to the caller to ensure that the requested
    //        sleep time does not overflow the number of days in the
    //        current calender month.
    //        Note: Standby mode will only work on a standard mbed board
    //        if exitDebugMode has previously been called.
    //        Note: any enabled external interrupt can wake the processor from
    //        Standby mode, just as if the end of the period has expired.
    //        Note: if the watchdog is being used, the caller should set the
    //        watchdog timeout to longer period than the Standby period.
    // \param powerDownBackupSram if true, backup SRAM will also be powered
    //        down in standby mode, otherwise it will be retained.
    void enterStandby(uint32_t standbyPeriodMilliseconds, bool powerDownBackupSram = false);

    /// Get the number of user interrupts that are enabled, sometimes helpful when debugging
    // power-down modes.  User interrupts start at 0 and the number of them varies
    // with the microcontroller.
    // \param pList a pointer to an area in which the list of enabled user interrupts
    //        will be stored.
    // \param sizeOfList the size of the memory pointer to be pList, in bytes.
    // \return the number of enabled user interrupts.
    int32_t numUserInterruptsEnabled(uint8_t *pList = NULL, uint32_t sizeOfList = 0);
    
protected:

     /// Check whether an interrupt is enabled or not.
    inline uint32_t myNVIC_GetEnableIRQ(IRQn_Type IRQn);
};

#endif

// End Of File
