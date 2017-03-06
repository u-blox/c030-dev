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
#include <low_power.h>
#ifdef TARGET_STM
#include <stm32f4xx_hal_pwr.h>
#endif

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

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

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

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
LowPower::LowPower(void)
{
#ifdef TARGET_STM
    // Enable back-up SRAM access, see section 5.1.2 of the STM32F437
    // detailed manual and stm32f4xx_hal_pwr.c
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKPSRAM_CLK_ENABLE();
    
    // Determine if we've already been active
    if (!__HAL_PWR_GET_FLAG(PWR_FLAG_BRR)) {
        // If the backup regulator is not running, we've not woken
        // up from Standby with data in there and so we
        // should zero the backup SRAM to clean it up.
        memset (BACKUP_SRAM_START_ADDRESS, 0, BACKUP_SRAM_SIZE);
    }
#endif        
}

/// Destructor.
LowPower::~LowPower(void)
{
}

/// Enter Stop mode.
void LowPower::enterStop(uint32_t stopPeriodMilliseconds)
{
    if (stopPeriodMilliseconds > 0) {
        // If time is -1 the RTC is not running, so call set_time with
        // a value of zero to kick it into life
        if (time(NULL) == (time_t) -1) {
            set_time(0);
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
        // also if we can
#ifdef TARGET_STM
        HAL_PWREx_EnableFlashPowerDown();
#endif        
        deepsleep();
#ifdef TARGET_STM
        HAL_PWREx_DisableFlashPowerDown();
#endif
    }
}

/// Enter Standby mode.
void LowPower::enterStandby(uint32_t standbyPeriodMilliseconds, bool powerDownBackupSram)
{
    if (standbyPeriodMilliseconds > 0) {
        // If time is -1 the RTC is not running, so call set_time with
        // a value of zero to kick it into life
        if (time(NULL) == (time_t) -1) {
            set_time(0);
        }
        
#ifdef TARGET_STM
        // Set the backup regulator (for backup SRAM) into the right state
        if (powerDownBackupSram) {
            HAL_PWREx_DisableBkUpReg();
        } else {
            HAL_PWREx_EnableBkUpReg();
        }
#endif
        
#ifdef DEBUG_LOW_POWER
        time_t timeNow = time(NULL);
        time_t timeThen = timeNow + standbyPeriodMilliseconds / 1000;
        char buf[32];
        strftime(buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", localtime(&timeThen));
        printf ("LowPower: going to Standby mode for %.3f second(s) (will awake at ~%s).\n", 
                (float) standbyPeriodMilliseconds / 1000, buf);
        wait_ms(100); // Let printf leave the building
#endif

        // Set the RTC alarm
        WakeUp::set_ms(standbyPeriodMilliseconds);
#ifdef TARGET_STM
        // Now enter Standby mode
        HAL_PWR_EnterSTANDBYMode();
#else
# error This function is currently only implemented for STM microcontrollers.
#endif
    }
}

/// Get the number of user interrupts that are enabled.
int32_t LowPower::numUserInterruptsEnabled(uint8_t *pList, uint32_t sizeOfList)
{
    int32_t userInterruptsEnabled = -1;
    uint8_t *pEndOfList = pList + sizeOfList;
    
#if defined (NVIC_NUM_VECTORS) && defined (NVIC_USER_IRQ_OFFSET)
# ifdef DEBUG_LOW_POWER
    printf ("Checking enabled interrupts...");
# endif
    userInterruptsEnabled = 0;
    for (uint8_t x = 0; x < NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET; x++) {
        if (myNVIC_GetEnableIRQ((IRQn_Type) x)) {
            userInterruptsEnabled++;
            if ((pList != NULL) && (pList < pEndOfList)) {
                *pList = x;
                pList++;
            }
# ifdef DEBUG_LOW_POWER
            printf (" %d", x);
# endif
        }
    }

# ifdef DEBUG_LOW_POWER
    if (userInterruptsEnabled == 0) {
        printf (" (none were enabled)");
    }
    printf (".\n");
# endif
#else
    printf ("Can't determine the number of user interrupts enabled.\n");
#endif
    
    return userInterruptsEnabled;
}

// End Of File
