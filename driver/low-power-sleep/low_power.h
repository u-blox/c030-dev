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
// have value of 0 when it is first accessed:
//
// BACKUP_SRAM
// uint32_t importantThing = 3;
#define BACKUP_SRAM __attribute__ ((section ("BKPSRAM")));

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
    // \param stopTimeSeconds the amount of time to remain in Stop
    //        mode for.  When the time has expired the function
    //        will return.  Any RTOS timers that expire during
    //        Stop mode will be ignored.
    // \param enableInterrupts if true, any interrupt will cause
    //        a wake-up from Stop mode otherwise only the one
    //        interrupt required to wake from Stop mode will be
    //        enabled.  Note: during Stop mode the processor is
    //        running from 32 kHz crystal and so any timer
    //        interrupts will be correspondingly longer.
    void enterStop(uint32_t stopTimeSeconds, bool enableInterrupts = false);
    
    
    /// Enter Standby mode.  Note that this function does NOT return.
    // \param standbyTimeSeconds the amount of time to remain in Standy
    //        mode for.  When the time has expired the processor will
    //        be reset and begin execution once more from main().  The
    //        values stored in BACKUP_SRAM will be retained, all other
    //        variables will be reset to their initial state. The RTOS
    //        is suspended on entry to Standby mode (i.e. no RTOS timers
    //        will expire) and the RTOS will be reset on return to main().
    void enterStandby(uint32_t standyTimeSeconds);
    
protected:

};

#endif

// End Of File
