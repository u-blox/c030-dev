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
 * @file low_power.cpp
 * This file defines a class intended to assist with obtaining lowest power
 * operation on an STM32F437 microprocessor.
 */

// Define this to print debug information
#define DEBUG_LOW_POWER

#include <mbed.h>
#include <stm32f4xx_hal_pwr.h>
#include <low_power.h>

#ifdef DEBUG_LOW_POWER
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

/// Location of backup SRAM in memory.
#define BACKUP_SRAM_START_ADDRESS ((uint32_t *) ((uint32_t) BKPSRAM_BASE))

/// Size of backup SRAM.
#define BACKUP_SRAM_SIZE 4096

#if (defined (TARGET_LIKE_CORTEX_M3) || defined (TARGET_LIKE_CORTEX_M4) || defined (TARGET_LIKE_CORTEX_M7)) && \
    defined (NVIC_NUM_VECTORS) && defined (NVIC_USER_IRQ_OFFSET)
/// Only if NVIC_NUM_VECTORS and NVIC_USER_IRQ_OFFSET are defined is it possible
// to disable interrupts and, until CMSIS 5 comes along, I have only pasted into here
// the NVIC_GetEnableIRQ() macro that happens to be the same across M3, M4 and M7 cores.
#define CAN_DISABLE_INTERRUPTS
#endif

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

#ifdef DEBUG_LOW_POWER
/// LED to flash when the alarm interrupt goes off.
static DigitalOut gDebugLed(LED1, 1);
#endif

// ----------------------------------------------------------------
// PRIVATE FUNCTIONS
// ----------------------------------------------------------------

/// Check whether an interrupt is enabled or not.
inline uint32_t LowPower::myNVIC_GetEnableIRQ(IRQn_Type IRQn)
{
    if ((int32_t)(IRQn) >= 0) {
        return((uint32_t)(((NVIC->ISER[(((uint32_t)(int32_t)IRQn) >> 5UL)] & (1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
    } else {
        return(0U);
    }
}

// Disable the user interrupts.
uint32_t LowPower::disableInterrupts(bool *pInterruptsActive, uint32_t numInterruptsActive)
{
    uint32_t numInterruptsDisabled = 0;
    
    if (pInterruptsActive != NULL) {
        memset (pInterruptsActive, false, numInterruptsActive);
    }
    
#ifdef DEBUG_LOW_POWER
    printf ("Disabling interrupts...");
#endif
    
    for (uint32_t x = 0; x < numInterruptsActive; x++) {
        if (myNVIC_GetEnableIRQ((IRQn_Type) x)) {
            if (pInterruptsActive != NULL) {
                *(pInterruptsActive + x) = true;
            }
            NVIC_ClearPendingIRQ((IRQn_Type) x);
            NVIC_DisableIRQ((IRQn_Type) x);
            numInterruptsDisabled++;
#ifdef DEBUG_LOW_POWER
            printf (" %d", x);
#endif
        }
    }
    
#ifdef DEBUG_LOW_POWER
    if (numInterruptsDisabled == 0) {
        printf (" (none were enabled)");
    }
    printf (".\n");
#endif
    
    return numInterruptsDisabled;
}

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
LowPower::LowPower(void)
{
    // Enable back-up SRAM access (following instructions in stm32f4xx_hal_pwr.c)
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    
    // Determine if we've already been active
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_BRR)) {
        // If the backup regulator is not running, we've not woken
        // up from Standby with data in there and so we
        // should zero the backup SRAM to clean it up.
        memset (BACKUP_SRAM_START_ADDRESS, 0, BACKUP_SRAM_SIZE);
    }
}

/// Destructor.
LowPower::~LowPower(void)
{
}

/// Enter Stop mode.
bool LowPower::enterStop(uint32_t stopPeriodMilliseconds, bool disableUserInterrupts)
{
    bool success = true;
    uint32_t numInterruptsDisabled = 0;
#ifdef CAN_DISABLE_INTERRUPTS
    bool interruptActive[NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET] = {false};
#endif

    if (stopPeriodMilliseconds > 0) {
                
        // Disable unnecessary user interrupts
        // This will disable the RTOS tick, so do not
        // call RTOS functions from this point onwards
        if (disableUserInterrupts) {
#ifdef CAN_DISABLE_INTERRUPTS
            numInterruptsDisabled = disableInterrupts(&(interruptActive[0]), sizeof (interruptActive) / sizeof (interruptActive[0]));
            // Ensure the RTC IRQ is enabled
            NVIC_EnableIRQ(RTC_WKUP_IRQn);
#else
            success = false;
#endif
        }

#ifdef DEBUG_LOW_POWER
        time_t timeNow = time(NULL);
        time_t timeThen = timeNow + stopPeriodMilliseconds / 1000;
        char buf[32];
        strftime(buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", localtime(&timeThen));
        printf ("LowPower: going to Stop mode for %.3f second(s) (will awake at ~%s).\n", 
                (float) stopPeriodMilliseconds / 1000, buf);
        wait_ms(100); // Let printf leave the building
#endif

        // Set the RTC alarm
        WakeUp::set_ms(stopPeriodMilliseconds);
        // Now enter Stop mode, allowing flash to power down
        HAL_PWREx_EnableFlashPowerDown();
        deepsleep();
        HAL_PWREx_DisableFlashPowerDown();

        // Re-enable those unnecessary interrupts
        if (numInterruptsDisabled > 0) {
            for (uint32_t x = 0; x < sizeof (interruptActive) / sizeof (interruptActive[0]); x++) {
                if (interruptActive[x]) {
                    NVIC_EnableIRQ((IRQn_Type) x);
                }
            }
#ifdef DEBUG_LOW_POWER
            printf ("Re-enabled the user interrupts previously disabled.\n");
#endif
        }
    }

    return success;
}

/// Enter Standby mode.
void LowPower::enterStandby(uint32_t standbyPeriodMilliseconds, bool disableUserInterrupts, bool powerDownBackupSram)
{
    bool success = false;
    
    if (standbyPeriodMilliseconds > 0) {
        // Set the backup regulator (for backup SRAM) into the right state
        if (powerDownBackupSram) {
            HAL_PWREx_DisableBkUpReg();
        } else {
            HAL_PWREx_EnableBkUpReg();
        }
        
        // Disable unnecessary user interrupts
        // This will disable the RTOS tick, so do not
        // call RTOS functions from this point onwards
        if (disableUserInterrupts) {
#ifdef CAN_DISABLE_INTERRUPTS
            disableInterrupts(NULL, NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET);
            // Ensure the RTC IRQ is enabled
            NVIC_EnableIRQ(RTC_WKUP_IRQn);
            success = true;
#endif
        } else {
            success = true;
        }
        
#ifdef DEBUG_LOW_POWER
        time_t timeNow = time(NULL);
        time_t timeThen = timeNow + standbyPeriodMilliseconds / 1000;
        char buf[32];
        strftime(buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", localtime(&timeThen));
        printf ("LowPower: going to Standby mode for %.3f second(s) (will awake at ~%s).\n", 
                (float) standbyPeriodMilliseconds / 1000, buf);
        wait_ms(100); // Let printf leave the building
#endif

        if (success) {
            // Set the RTC alarm
            WakeUp::set_ms(standbyPeriodMilliseconds);
            // Now enter Standby mode
            HAL_PWR_EnterSTANDBYMode();
        }
    }
}

// End Of File
