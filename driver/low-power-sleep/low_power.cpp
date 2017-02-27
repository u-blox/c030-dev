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
#include <low_power.h>

#ifdef DEBUG_LOW_POWER
# include <stdio.h>
#endif

// ----------------------------------------------------------------
// GENERAL COMPILE-TIME CONSTANTS
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// PRIVATE VARIABLES
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GENERIC PRIVATE FUNCTIONS
// ----------------------------------------------------------------

//----------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------

/// Constructor.
LowPower::LowPower(void)
{
}

/// Destructor.
LowPower::~LowPower(void)
{
}

// End Of File
