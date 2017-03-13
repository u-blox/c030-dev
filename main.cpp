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

#include "mbed.h"
#include "battery_charger_bq24295.h"
#include "battery_gauge_bq27441.h"
#include "low_power.h"

/**
 * @file main.cpp
 * main() for testing C030 stuff.
 *
 * IMPORTANT: this code puts the C030 MCU chip into
 * its lowest power state.  The ability to do this
 * is affected by the state of the debug chip on the board.
 * To be sure that this code executes correctly, you must
 * completely power off the board after downloading code,
 * and power it back on again.
 */

// Pin-out
#define PIN_I2C_SDA  PC_9
#define PIN_I2C_SCL  PA_8

// Backup SRAM stuff
BACKUP_SRAM
static time_t gTimeNow;

BACKUP_SRAM
static char gBackupSram[BACKUP_SRAM_SIZE - sizeof (gTimeNow)];

#define BACKUP_SRAM_STRING "Back from the dead!"

// LEDs
DigitalOut gLedRed(LED1);
DigitalOut gLedGreen(LED2);
DigitalOut gLedBlue(LED3);

// ----------------------------------------------------------------
// PRIVATE FUNCTIONS
// ----------------------------------------------------------------

static void signalGood(void)
{
    gLedGreen = 0;
    gLedRed = 1;
    gLedBlue = 1;
    wait_ms(1000);
    gLedGreen = 1;
}

static void signalBad(void)
{
    gLedRed = 0;
    gLedGreen = 1;
    gLedBlue = 1;
    wait_ms(1000);
    gLedRed = 1;
}

static void signalEvent(void)
{
    gLedBlue = 0;
    gLedGreen = 1;
    gLedRed = 1;
    wait_ms(1000);
    gLedBlue = 1;
}

static void signalOff(void)
{
    gLedGreen = 1;
    gLedRed = 1;
    gLedBlue = 1;
}

// ----------------------------------------------------------------
// PUBLIC FUNCTIONS: MAIN
// ----------------------------------------------------------------

#if 1
int main()
{
    LowPower *pLowPower = new LowPower();
    BatteryChargerBq24295 *pBatteryCharger = NULL;
    BatteryGaugeBq27441 *pBatteryGauge = NULL;
    I2C *pI2C = NULL;
    time_t timeNow;
    
    // Have to exit Debug mode on the chip if we want to go into Standby mode
    pLowPower->exitDebugMode();

    if (time(NULL) != 0) {
        // If the RTC is running, we must have been awake previously
        printf ("Awake from Standby mode after %d second(s).\n", (int) (time(NULL) - gTimeNow));
        printf ("Backup RAM contains \"%.*s\".\n", sizeof(BACKUP_SRAM_STRING), gBackupSram);
    } else {
        printf("\n\nStarting up from a cold start.\n");
        printf("IMPORTANT: this code puts the STM32F4xx chip into its lowest power state.\n");
        printf("The ability to do this is affected by the state of the debug chip on the C030\n");
        printf("board. To be sure that this code executes correctly, you must completely power\n");
        printf("off the board after downloading code, and power it back on again.\n\n");
        signalOff();
    }

    pI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SCL);
    
    if (pI2C != NULL) {
        
        pBatteryCharger = new BatteryChargerBq24295();
        if (pBatteryCharger != NULL) {
            if (pBatteryCharger->init(pI2C)) {
                printf ("BQ24295 battery charger ready.\n");
                signalGood();
            } else {
                printf ("Unable to initialise BQ24295 charger chip.\n");
                signalBad();
            }
        } else {
          printf("Unable to instantiate BQ24295 charger chip.\n");
          signalBad();
        }          
      
        pBatteryGauge = new BatteryGaugeBq27441();
        if (pBatteryGauge != NULL) {
            if (pBatteryGauge->init(pI2C)) {
                printf ("BQ27441 battery gauge ready.\n");
                signalGood();
            } else {
                printf ("Unable to initialise BQ27441 battery gauge chip.\n");
                signalBad();
            }
        } else {
            printf("Unable to instantiate BQ27441 battery gauge chip.\n");
            signalBad();
        }
        
    } else {
       printf("Unable to instantiate I2C.\n");
       signalBad();
    }

    printf ("Entering Stop mode for 5 seconds...\n");
    // Let the printf leave the building
    wait_ms(100);
    signalEvent();
    timeNow = time(NULL);
    pLowPower->enterStop(5000);
    printf ("Awake from Stop mode after %d second(s).\n", (int) (time(NULL) - timeNow));

    printf ("Putting \"%s\" into BKPSRAM...\n", BACKUP_SRAM_STRING);
    memcpy (gBackupSram, BACKUP_SRAM_STRING, sizeof(BACKUP_SRAM_STRING));

    printf ("Entering Standby mode for 5 seconds...\n");
    // Let the printf leave the building
    wait_ms(100);
    signalEvent();
    gTimeNow = time(NULL);
    pLowPower->enterStandby(5000);

    printf("Should never get here.\n");
    MBED_ASSERT(false);
    
    return 0;
}
#endif

// End Of File
