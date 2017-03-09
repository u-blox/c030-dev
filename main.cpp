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
 */

// Pin-out
#define PIN_I2C_SDA  PC_9
#define PIN_I2C_SCL  PA_8

BACKUP_SRAM
static char gBackupSram[BACKUP_SRAM_SIZE];

#define BACKUP_SRAM_STRING "Back from the dead!\n"

// ----------------------------------------------------------------
// PUBLIC FUNCTIONS: MAIN
// ----------------------------------------------------------------

#if 0
int main()
{
    LowPower *pLowPower = new LowPower();
    BatteryChargerBq24295 *pBatteryCharger = NULL;
    BatteryGaugeBq27441 *pBatteryGaugeBq27441 = NULL;
    I2C *pI2C = NULL;
    bool chargerSuccess = false;
    bool gaugeBq27441Success = false;
    
    // Have to exit Debug mode on the chip if we want to go into Standby mode
    pLowPower->exitDebugMode();

    printf("Starting up...\n");
    
    printf("Checking if we've just come back from Standby mode...\n");
    if (time(NULL) != 0) {
        // If the RTC is running, we must have been awake previously
        printf ("%*s", sizeof(BACKUP_SRAM_STRING), gBackupSram);
    } else {
        printf ("No, this is a cold start.\n");
    }

    pI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SCL);
    
    if (pI2C != NULL) {
        
        pBatteryCharger = new BatteryChargerBq24295();
        if (pBatteryCharger != NULL) {
            chargerSuccess = pBatteryCharger->init(pI2C);
            if (!chargerSuccess) {
                printf ("Unable to initialise BQ24295 charger chip.\n");
            }
        } else {
          printf("Unable to instantiate BQ24295 charger chip.\n");
        }          
      
        pBatteryGaugeBq27441 = new BatteryGaugeBq27441();
        if (pBatteryGaugeBq27441 != NULL) {
            gaugeBq27441Success = pBatteryGaugeBq27441->init(pI2C);
            if (!gaugeBq27441Success) {
                printf ("Unable to initialise BQ27441 battery gauge chip.\n");
            }
        } else {
            printf("Unable to instantiate BQ27441 battery gauge chip.\n");
        }
        
    } else {
       printf("Unable to instantiate I2C.\n");
    }

    if (chargerSuccess && gaugeBq27441Success) {
        printf ("BQ24295 battery charger and BQ27441 battery gauge ready.\n");
    }

    printf ("Entering Stop mode for 5 seconds...");
    // Let the printf leave the building
    wait_ms(100);
    pLowPower->enterStop(5000);
    printf (" awake now.\n");

    printf ("Putting \"Back from the dead!\" into BKPSRAM...\n");
    memcpy (gBackupSram, BACKUP_SRAM_STRING, sizeof(BACKUP_SRAM_STRING));

    printf ("Entering Standby mode for 5 seconds...\n");
    // Let the printf leave the building
    wait_ms(100);
    pLowPower->enterStandby(5000);

    printf("Should never get here.\n");
    MBED_ASSERT(false);
    
    return 0;
}
#endif

// End Of File
