This class provides an API to assist with low power behaviour on an STM32F437 micro.

The principle is that the STM32F437 is put into Stop mode (typical current consumption 100 uA @ 1.8 V), where the main clocks are stopped and it is running from the internal 32 kHz oscillator.  Wake-up from this state is achieved using an alarm from the RTC.  Since lack of a clock involves suspending the RTOS and the RTOS uses microsecond timing in a uint32_t variable, the maximum sleep time is 4294 seconds, or just over 1 hour.  However, intermediate wake-ups are handled internally by this library in order to permit sleep times at a scale of 1 second resolution in a time_t (int32_t) width variable.

In addition to this, it is possible to save further power by putting the STM32F437 into Standby mode (typical current consumption 2.8 uA @ 1.8 V).  In this mode, as well as the main clocks being stopped, all of RAM is also lost; only the 4 kbyte Backup SRAM is retained.  Provided that your code is designed such that none of the following need be  maintained across a low-power sleep cycle:

* RTOS timers,
* dynamically allocated variables (i.e. those on the stack or the heap, for instance allocated with 'new'),
* more than 4 kbytes of statically allocated variables,

...then it may be possible to use this low power mode.

Finally, if your code has another means of retaining state across a low-power sleep cycle then even the Backup SRAM can be powered down, reducing the typical current consumption to 2.3 uA @ 1.8 V.

Note that these functions take position of the RTC hardware block on the microcontroller.