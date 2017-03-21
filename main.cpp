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
#include "gnss.h"

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

#define GNSS_WAIT_TIME_SECONDS 120
#define STOP_TIME_SECONDS 5
#define STANDBY_TIME_SECONDS 5

#define _CHECK_TALKER(s) ((buffer[3] == s[0]) && (buffer[4] == s[1]) && (buffer[5] == s[2]))

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

#if 0
int main()
{
    GnssSerial *pGnss = new GnssSerial();
    LowPower *pLowPower = new LowPower();
    BatteryChargerBq24295 *pBatteryCharger = NULL;
    BatteryGaugeBq27441 *pBatteryGauge = NULL;
    I2C *pI2C = NULL;
    time_t timeNow;
    int gnssReturnCode;
    char buffer[256];
    bool gotTime = false;


    // Have to exit Debug mode on the chip if we want to go into Standby mode
    pLowPower->exitDebugMode();

    // Power up GNSS
    pGnss->init();

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

    printf ("Waiting up to %d second(s) for GNSS to receive the time...\n", GNSS_WAIT_TIME_SECONDS);
    for (uint32_t x = 0; (x < GNSS_WAIT_TIME_SECONDS) && !gotTime; x+= STOP_TIME_SECONDS) {
        while ((gnssReturnCode = pGnss->getMessage(buffer, sizeof(buffer))) > 0) {
            int32_t length = LENGTH(gnssReturnCode);

            if ((PROTOCOL(gnssReturnCode) == GnssParser::NMEA) && (length > 6)) {
                // talker is $GA=Galileo $GB=Beidou $GL=Glonass $GN=Combined $GP=GNSS
                if ((buffer[0] == '$') || buffer[1] == 'G') {
                    if (_CHECK_TALKER("GGA") || _CHECK_TALKER("GNS")) {
                        const char *pTimeString = NULL;

                        // Retrieve the time
                        pTimeString = pGnss->findNmeaItemPos(1, buffer, buffer + length);
                        if (pTimeString != NULL) {
                            gotTime = true;
                            printf("GNSS: time is %.6s.\n", pTimeString);
                        }
                    }
                }
            }
        }

        if (!gotTime) {
            printf ("  Entering Stop mode for %d second(s) while waiting...\n", STOP_TIME_SECONDS);
            // Let the printf leave the building
            wait_ms(100);
            signalEvent();
            timeNow = time(NULL);
            pLowPower->enterStop(STOP_TIME_SECONDS * 1000);
            printf ("  Awake from Stop mode after %d second(s).\n", (int) (time(NULL) - timeNow));
        }
    }


    printf ("\nPutting \"%s\" into BKPSRAM...\n", BACKUP_SRAM_STRING);
    memcpy (gBackupSram, BACKUP_SRAM_STRING, sizeof(BACKUP_SRAM_STRING));

    printf ("Entering Standby mode for %d second(s)...\n", STANDBY_TIME_SECONDS);
    // Let the printf leave the building
    wait_ms(100);
    signalEvent();
    gTimeNow = time(NULL);
    pLowPower->enterStandby(STANDBY_TIME_SECONDS * 1000);

    printf("Should never get here.\n");
    MBED_ASSERT(false);

    return 0;
}
#endif

// End Of File
