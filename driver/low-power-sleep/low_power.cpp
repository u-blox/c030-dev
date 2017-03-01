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
//#define DEBUG_LOW_POWER

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
    uint32_t rt_suspend (void);
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

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

/// A handle for the RTC, required by the various STM HAL calls.
// Note: the only bits that we care about in this struct are the Instance
// field (which points to the registers) and the State field (which
// the STM HAL calls use to mediate access to the hardware).
static RTC_HandleTypeDef gRtcHandle;

/// A flag that we can check to see if we have already been active
BACKUP_SRAM
static uint8_t gAlreadyActive;

/// The number of days in a month, used when working out the alarm time
static const uint8_t gDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

/// A copy of the mbed deepsleep() function but with under-drive
// mode and ensuring that flash is powered down to save more power.
void LowPower::underDriveDeepSleep(void)
{
    // Stop HAL systick
    HAL_SuspendTick();

    // Request to enter STOP mode with regulator in low power mode
#if TARGET_STM32L4
    if (__HAL_RCC_PWR_IS_CLK_ENABLED()) {
        HAL_PWREx_EnableLowPowerRunMode();
        HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
        HAL_PWREx_DisableLowPowerRunMode();
    } else {
        __HAL_RCC_PWR_CLK_ENABLE();
        HAL_PWREx_EnableLowPowerRunMode();
        HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
        HAL_PWREx_DisableLowPowerRunMode();
        __HAL_RCC_PWR_CLK_DISABLE();
    }
#else /* TARGET_STM32L4 */
    // These are the lines modified from deepsleep();
    HAL_PWREx_EnableFlashPowerDown();
    //HAL_PWREx_EnableLowRegulatorLowVoltage();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_UNDERDRIVE_ON, PWR_STOPENTRY_WFI);
#endif /* TARGET_STM32L4 */

    // Restart HAL systick
    HAL_ResumeTick();

    // After wake-up from STOP reconfigure the PLL
    SetSysClock();

#if DEVICE_LOWPOWERTIMER
    rtc_synchronize();
#endif
}

/// Add a period in seconds to a DateTime_t struct.
void LowPower::addPeriod(LowPower::DateTime_t * pDateTime, time_t periodSeconds)
{
    uint8_t x;
    uint32_t guardCount = 0;
    
    if ((pDateTime != NULL) && (pDateTime->year <= YEAR_MAX)) {
#ifdef DEBUG_LOW_POWER
        printf ("Adding %d seconds to %04d-%02d-%02d %02d:%02d:%02d.\n", periodSeconds, 
                pDateTime->year + BASE_YEAR, pDateTime->month, pDateTime->day, pDateTime->hour, pDateTime->minute, pDateTime->second);
#endif
        MBED_ASSERT (pDateTime->second < 60);
        MBED_ASSERT (pDateTime->minute < 60);
        MBED_ASSERT (pDateTime->hour < 24);
        MBED_ASSERT ((pDateTime->day >= 1) && (pDateTime->day <= 31));
        MBED_ASSERT ((pDateTime->month >= 1) && (pDateTime->month <= 12));

        // Second
        pDateTime->second += (periodSeconds % 60);
        if (pDateTime->second >= 60) {
            pDateTime->second -= 60;
            (pDateTime->minute)++;
        }
        // Minute
        pDateTime->minute += ((periodSeconds / 60) % 60);
        if (pDateTime->minute >= 60) {
            pDateTime->minute -= 60;
            (pDateTime->hour)++;
        }
        // Hour
        pDateTime->hour += ((periodSeconds / (60 * 60)) % 24);
        if (pDateTime->hour >= 24) {
            pDateTime->hour -= 24;
            (pDateTime->day)++;
        }

        // Lump all the remaining days into one first
        pDateTime->day += periodSeconds / (60 * 60 * 24);
        // Then parcel the days out into the months and years
        // until they are less than the number of days in the
        // last month we arrive at
        for (x = daysInMonth(pDateTime->month, pDateTime->year + BASE_YEAR);
             (pDateTime->day > x) && (guardCount < YEAR_MAX * 12);
             x = daysInMonth(pDateTime->month, pDateTime->year + BASE_YEAR)) {
            if (pDateTime->day > x) {
                pDateTime->month++;
                pDateTime->day -= x;
                // Handle month overflow
                if (pDateTime->month > 12) {
                    pDateTime->year++;
                    pDateTime->month -= 12;
                }
            }
            guardCount++;
        }
        
        MBED_ASSERT (guardCount < YEAR_MAX * 12);
        
        // That should be it, the days handle themselves
#ifdef DEBUG_LOW_POWER
        printf ("Result is %04d-%02d-%02d %02d:%02d:%02d.\n", pDateTime->year + BASE_YEAR, pDateTime->month,
                pDateTime->day, pDateTime->hour, pDateTime->minute, pDateTime->second);
#endif
    }
}

/// Set alarm A on the RTC.
bool LowPower::setAlarmA(const LowPower::DateTime_t * pDateTime)
{
    RTC_AlarmTypeDef alarm;
    
    alarm.AlarmTime.Seconds = pDateTime->second;
    alarm.AlarmTime.Minutes = pDateTime->minute;
    alarm.AlarmTime.Hours = pDateTime->hour;
    alarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
    alarm.AlarmMask = RTC_ALARMMASK_NONE;  // No masking, all matter
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL; // Subseconds masked out (they don't matter)
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY; // Day of week masked out (just the date matters)
    alarm.AlarmDateWeekDay = pDateTime->day;
    alarm.Alarm = RTC_ALARM_A;

#ifdef DEBUG_LOW_POWER
    printf ("Setting alarm for: day %02d, time %02d:%02d:%02d.\n", alarm.AlarmDateWeekDay,
            alarm.AlarmTime.Hours, alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);
#endif

    return (HAL_RTC_SetAlarm(&gRtcHandle, &alarm, RTC_FORMAT_BIN) == HAL_OK);
}

/// Set an alarm for a number of seconds in the future.
bool LowPower::setRtcAlarm(time_t periodSeconds)
{
    bool success = false;
    RTC_TimeTypeDef timeNow;
    RTC_DateTypeDef dateNow;
    LowPower::DateTime_t alarm;
    
    // Get the current time and date
    if (HAL_RTC_GetTime(&gRtcHandle, &timeNow, RTC_FORMAT_BIN) == HAL_OK) {
        if (HAL_RTC_GetDate(&gRtcHandle, &dateNow, RTC_FORMAT_BIN) == HAL_OK) {
            
#ifdef DEBUG_LOW_POWER
        printf ("Time now is: %04d-%02d-%02d %02d:%02d:%02d.\n", dateNow.Year + BASE_YEAR, dateNow.Month, dateNow.Date,
                timeNow.Hours, timeNow.Minutes, timeNow.Seconds);
#endif
            // Add periodSeconds to it
            alarm.second = timeNow.Seconds;
            alarm.minute = timeNow.Minutes;
            alarm.hour = timeNow.Hours;
            if (timeNow.TimeFormat == RTC_HOURFORMAT12_PM) {
                alarm.hour += 12;
            }
            alarm.day = dateNow.Date;
            alarm.month = dateNow.Month;
            alarm.year = dateNow.Year;
            addPeriod(&alarm, periodSeconds);

            // Now set an alarm at that time/date
            success = setAlarmA(&alarm);
        }
    }

    return success;
}

/// Determine if a year is a leap year, using the algorithm described on Wikipedia:
// https://en.wikipedia.org/wiki/Leap_year
bool LowPower::isLeapYear(uint32_t year)
{
    bool isLeapYear = false;
    
    if (year % 4 != 0) {
    } else if ((year % 100) != 0) {
        isLeapYear = true;
    } else if ((year % 400) != 0) {
    } else {
        isLeapYear = true;
    }

    return isLeapYear;
}

/// Return the number of days in a month.
uint8_t LowPower::daysInMonth(uint8_t month, uint32_t year)
{
    uint8_t days;
    
    MBED_ASSERT ((month >= 1) && (month <= 12));
    
    days = gDaysInMonth[month - 1];
    if ((month == 2) && isLeapYear(year)) {
        days++;
    }
    
    return days;
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
    if (gAlreadyActive != ALREADY_ACTIVE) {
        // If we've not been active previously, then zero the backup SRAM
        memset (BACKUP_SRAM_START_ADDRESS, 0, BACKUP_SRAM_SIZE);
    }
    
    // We've been active now
    gAlreadyActive = ALREADY_ACTIVE;
}

/// Destructor.
LowPower::~LowPower(void)
{
}

/// Enter Stop mode.
bool LowPower::enterStop(time_t stopPeriodSeconds, bool enableInterrupts)
{
    bool success = false;
    time_t startTimeSeconds = time(NULL);
    time_t alarmPeriodSeconds = stopPeriodSeconds;
    uint32_t suspendTimeRtos;
    
    if (!enableInterrupts) {
        // Disable unnecessary interrupts
    }
        
    // Sleep until the time is right
    while (time(NULL) < startTimeSeconds + stopPeriodSeconds) {
        
        // If the stop time is longer than the wrap of the 32-bit microsecond
        // timer used by the RTOS, we need to wake up at intermediate points
        // to service that, then return to sleep.  -1 used for margin.
        if (stopPeriodSeconds > (0xFFFFFFFFU / 1000000U) - 1) {
            alarmPeriodSeconds = (0xFFFFFFFFU / 1000000U) - 1;
        }
        
        // Set the RTC alarm
        success = setRtcAlarm (alarmPeriodSeconds);
        if (success) {
            // Suspend the RTOS
            suspendTimeRtos = rt_suspend();
        
            // Now enter Stop mode
            underDriveDeepSleep();
            
            // Resume RTOS operations
            rt_resume(suspendTimeRtos);
        }
    }
    
    if (!enableInterrupts) {
        // Restore interrupts
    }
    
    return success;
}

/// Enter Standby mode.
void LowPower::enterStandby(time_t standbyPeriodSeconds, bool powerDownBackupSram)
{
    time_t startTimeSeconds = time(NULL);
    
    if (standbyPeriodSeconds > 0) {
        // Set the backup regulator (for backup SRAM) into the right state
        if (powerDownBackupSram) {
            HAL_PWREx_DisableBkUpReg();
        } else {
            HAL_PWREx_EnableBkUpReg();
        }
        
        // Set the RTC alarm
        if (setRtcAlarm (standbyPeriodSeconds)) {
            // Now enter Standby mode
            HAL_PWR_EnterSTANDBYMode();
        }
    }
}

// End Of File
