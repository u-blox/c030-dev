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
#include <stm32f4xx_hal_rtc.h>
#include <stm32f4xx_hal_pwr.h>
#include <low_power.h>

#ifdef DEBUG_LOW_POWER
# include <stdio.h>
#endif

/// Bring in the RTX suspend/resume functions.
// Note: the correct way to do this would be to include rt_System.h
// but that drags in rt_TypeDef.h which has its own (conflicting)
// definition for NULL.  So it's simpler to just extern these directly.
extern "C"
{
    /// Suspend function from RTX.
    // \return the suspend time in microseconds.
    uint32_t rt_suspend (void);
    /// Resume function from RTX.
    // \param sleep_time the time that RTX was suspended in microseconds.
    void rt_resume (uint32_t sleep_time);
}

// ----------------------------------------------------------------
// COMPILE-TIME MACROS
// ----------------------------------------------------------------

/// Value that indicates this class has already been initialised.
#define ALREADY_ACTIVE 0x42

/// Location of backup SRAM in memory.
#define BACKUP_SRAM_START_ADDRESS ((uint32_t *) ((uint32_t) BKPSRAM_BASE))

/// Size of backup SRAM.
#define BACKUP_SRAM_SIZE 4096

#if defined (TARGET_LIKE_CORTEX_M4) && defined (NVIC_NUM_VECTORS) && defined (NVIC_USER_IRQ_OFFSET)
/// Only if NVIC_NUM_VECTORS and NVIC_USER_IRQ_OFFSET are defined is
// it possbile to disable interrupts and, until CMSIS 5 comes along, we only have
// the NVIC_GetEnableIRQ() macro for M4 cores.
#define CAN_DISABLE_INTERRUPTS
#endif

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

/// A handle for the RTC, required by the various STM HAL calls.
// Note: the only bits that we care about in this struct are the Instance
// field (which points to the registers) and the State field (which
// the STM HAL calls use to mediate access to the hardware).
static RTC_HandleTypeDef gRtcHandle;

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

/// Set an alarm for a given time.
bool LowPower::setRtcAlarm(struct tm * pAlarmStruct)
{
    RTC_AlarmTypeDef alarm;
    
    alarm.AlarmTime.Seconds = pAlarmStruct->tm_sec;
    alarm.AlarmTime.Minutes = pAlarmStruct->tm_min;
    alarm.AlarmTime.Hours = pAlarmStruct->tm_hour;
    alarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
    alarm.AlarmMask = RTC_ALARMMASK_NONE;  // No masking, all of the above matter
    alarm.AlarmTime.SubSeconds = 0; // Must set them to zero as the mask below is ORed with them
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL; // Subseconds masked (they don't matter)
    alarm.AlarmDateWeekDay = pAlarmStruct->tm_mday;
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE; // Day in the month (rather than day of the week)
    alarm.Alarm = RTC_ALARM_A;
    
#ifdef DEBUG_LOW_POWER
    printf ("LowPower: setting alarm for: day %02d, time %02d:%02d:%02d.\n", alarm.AlarmDateWeekDay,
            alarm.AlarmTime.Hours, alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);
    wait_ms(100); // Let printf leave the building
#endif

    return (HAL_RTC_SetAlarm_IT(&gRtcHandle, &alarm, RTC_FORMAT_BIN) == HAL_OK);
}

// Disable the user interrupts.
uint32_t LowPower::disableInterrupts(bool *pInterruptsActive, uint32_t numInterruptsActive)
{
    uint32_t numInterruptsDisabled = 0;
    
    memset (pInterruptsActive, false, numInterruptsActive);
    
#ifdef DEBUG_LOW_POWER
    printf ("Disabling interrupts...");
#endif
    
    for (uint32_t x = 0; x < numInterruptsActive; x++) {
        if (myNVIC_GetEnableIRQ((IRQn_Type) x)) {
            *(pInterruptsActive + x) = true;
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
    
    // Set up the handle for the RTC
    // mbed has already initalised the RTC, or is about to, and we don't want
    // to initialise it again, we just want to be able to read it and write
    // to the alarm registers.  So say that it is ready and connect up the registers.
    gRtcHandle.State = HAL_RTC_STATE_READY;
    gRtcHandle.Instance = RTC;
    
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
bool LowPower::enterStop(time_t stopPeriodSeconds, bool disableUserInterrupts)
{
    bool success = true;
    time_t sleepTimeSeconds = time(NULL);
    time_t alarmTimeSeconds = sleepTimeSeconds + stopPeriodSeconds;
    struct tm *pAlarmStruct;
    uint32_t numInterruptsDisabled = 0;
#ifdef CAN_DISABLE_INTERRUPTS
    bool interruptActive[NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET] = {false};
#endif

    // Disable unnecessary user interrupts
    if (disableUserInterrupts) {
#ifdef CAN_DISABLE_INTERRUPTS
        numInterruptsDisabled = disableInterrupts(&(interruptActive[0]), sizeof (interruptActive) / sizeof (interruptActive[0]));
# ifdef DEBUG_LOW_POWER
        printf ("\nMaking sure the RTC interrupt is enabled.\n");
# endif
        NVIC_EnableIRQ(RTC_WKUP_IRQn);
#else
        success = false;
#endif
    }

    // Sleep until the time is right
    if (success) {
        while ((time(NULL) < alarmTimeSeconds)) {
            // Update the time
            sleepTimeSeconds = time(NULL);
            
            // If the stop period is longer than the wrap of the 32-bit tick used
            // by the RTOS, we need to wake up at intermediate points to service
            // that, then return to sleep.  -1 used for margin.
            if (alarmTimeSeconds - sleepTimeSeconds > (time_t) (0xFFFFFFFFU / osKernelSysTickFrequency) - 1) {
                alarmTimeSeconds = (0xFFFFFFFFU / osKernelSysTickFrequency) - 1 + sleepTimeSeconds;
            }
            
            // Set the RTC alarm
            pAlarmStruct = localtime (&alarmTimeSeconds);
            success = setRtcAlarm (pAlarmStruct);
            
#ifdef DEBUG_LOW_POWER
            struct tm *pTimeStruct = localtime (&sleepTimeSeconds);
            printf ("LowPower: sleeping for %d second(s) out of %d (time now is %04d-%02d-%02d %02d:%02d:%02d).\n", 
                    alarmTimeSeconds - sleepTimeSeconds, stopPeriodSeconds, pTimeStruct->tm_year + 1900, pTimeStruct->tm_mon,
                    pTimeStruct->tm_mday, pTimeStruct->tm_hour, pTimeStruct->tm_min, pTimeStruct->tm_sec);
            wait_ms(100); // Let printf leave the building
#endif
            if (success) {
                // Suspend the RTOS
                // No RTOS calls (including anything to do with time) must be made until
                // rt_resume() is called.
                // TODO figure out why calling this mucks things up
                // rt_suspend();

                // Now enter Stop mode, allowing flash to power down
                HAL_PWREx_EnableFlashPowerDown();
                deepsleep();
                HAL_PWREx_DisableFlashPowerDown();

                // Resume RTOS operations
                rt_resume(osKernelSysTickMicroSec ((uint64_t) ((uint64_t) time(NULL) - (uint64_t) sleepTimeSeconds) * 1000));
            }
        }
    }
    
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
        
    return success;
}

/// Enter Standby mode.
void LowPower::enterStandby(time_t standbyPeriodSeconds, bool disableUserInterrupts, bool powerDownBackupSram)
{
    time_t alarmTimeSeconds = time(NULL) + standbyPeriodSeconds;
    struct tm *pAlarmStruct;
    
    if (standbyPeriodSeconds > 0) {
        // Set the backup regulator (for backup SRAM) into the right state
        if (powerDownBackupSram) {
            HAL_PWREx_DisableBkUpReg();
        } else {
            HAL_PWREx_EnableBkUpReg();
        }
        
        // Disable unnecessary user interrupts
        if (disableUserInterrupts) {
            for (uint32_t x = 0; x < NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET; x++) {
                if (NVIC_GetActive((IRQn_Type) x)) {
                    NVIC_DisableIRQ((IRQn_Type) x);
                }
            }
            NVIC_EnableIRQ(RTC_WKUP_IRQn);
        }

        // Set the RTC alarm
        pAlarmStruct = localtime (&alarmTimeSeconds);
        if (setRtcAlarm (pAlarmStruct)) {
            // Now enter Standby mode
            HAL_PWR_EnterSTANDBYMode();
        }
    }
}

// End Of File
