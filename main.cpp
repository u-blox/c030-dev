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
#include "battery_gauge_ltc2943.h"

/**
 * @file main.cpp
 * main() for testing C030 stuff.
 */

// Required for UTM board
static DigitalOut i2CPullUpBar(P1_1, 0);
// Pin-out
#define PIN_I2C_SDA  P0_27
#define PIN_I2C_SCL  P0_28
 
// ----------------------------------------------------------------
// PUBLIC FUNCTIONS: MAIN
// ----------------------------------------------------------------

#if 0
int main()
{
    BatteryChargerBq24295 * pBatteryCharger = NULL;
    BatteryGaugeBq27441 * pBatteryGaugeBq27441 = NULL;
    BatteryGaugeLtc2943 * pBatteryGaugeLtc2943 = NULL;
    I2C * pI2C = NULL;
    bool chargerSuccess = false;
    bool gaugeBq27441Success = false;
    bool gaugeLtc2943Success = false;
    
    printf("Starting up...\n");
    
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
        
        pBatteryGaugeLtc2943 = new BatteryGaugeLtc2943();
        if (pBatteryGaugeLtc2943 != NULL) {
            gaugeLtc2943Success = pBatteryGaugeLtc2943->init(pI2C);
            if (!gaugeLtc2943Success) {
                printf ("Unable to initialise LTC2943 battery gauge chip.\n");
            }
        } else {
            printf("Unable to instantiate LTC2943 battery gauge chip.\n");
        }
        
    } else {
       printf("Unable to instantiate I2C.\n");
    }

    if (chargerSuccess && gaugeBq27441Success && gaugeLtc2943Success) {
        printf ("BQ24295 battery charger, BQ27441 battery gauge and LTC2943 battery gauge ready.\n");
    }
    
    return 0;
}
#endif

// End Of File
