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
#include "lipo_charger_bq24295.h"
#include "lipo_gauge_bq27441.h"

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
    LipoChargerBq24295 * pLipoCharger = NULL;
    LipoGaugeBq27441 * pLipoGauge = NULL;
    I2C * pI2C = NULL;
    bool lipoChargerSuccess = false;
    bool lipoGaugeSuccess = false;
    
    printf("Starting up...\n");
    
    pI2C = new I2C(PIN_I2C_SDA, PIN_I2C_SCL);
    
    if (pI2C != NULL) {
        
        pLipoCharger = new LipoChargerBq24295();
        if (pLipoCharger != NULL) {
            lipoChargerSuccess = pLipoCharger->init(pI2C);
            if (!lipoChargerSuccess) {
                printf ("Unable to initialise LiPo Charger.\n");
            }
        } else {
          printf("Unable to instantiate LiPo Charger.\n");
        }          
      
        pLipoGauge = new LipoGaugeBq27441();
        if (pLipoGauge != NULL) {
            lipoGaugeSuccess = pLipoGauge->init(pI2C);
            if (!lipoGaugeSuccess) {
                printf ("Unable to initialise LiPo Gauge.\n");
            }
        } else {
            printf("Unable to instantiate LiPo Gauge.\n");
        }
        
    } else {
       printf("Unable to instantiate I2C.\n");
    }

    if (lipoChargerSuccess && lipoGaugeSuccess) {
        printf ("LipoCharger and LipoGauge ready.\n");
    }
    
    return 0;
}
#endif

// End Of File
