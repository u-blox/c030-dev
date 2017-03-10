# C030 Development Area

This repo contains the early development code and associated tests for the C030 board.  This code will later be split into individual libraries.

# Prerequisites

To fetch and build the code in this repository you must first install the [mbed CLI tools](https://github.com/ARMmbed/mbed-cli#installation) and their prerequisites; this will include a compiler (you can use GCC_ARM, ARM or IAR).  You will need to use `mbed config` to tell the mbed CLI tools where that compiler is.

Then, to run the tests, you must install the [mbed test tools](https://github.com/ARMmbed/greentea/blob/master/docs/QUICKSTART.md).

Until we have C030 boards, you will also need to obtain a Vodafone UTM board, along with its LiPo battery and a USB cable to connect it to a PC; we're using this for driver development as it includes the LiPo battery gauge, LiPo battery charger and external battery gauge chips that we will use on the C030 board.

# How To Test This Code

Clone this repo.

Change directory to this repo and run:

`mbed update`

...to get the very latest mbed-os development branch that has been forked to u-blox Github.

Change to the `mbed-os` directory and run:

`mbed update c030-dev`

...to switch to the `c030-dev` branch that we are working on in our u-blox mbed-os Github repository.

Set the target and the toolchain that you want to use, which should be `UBLOX_C030` and one of `GCC_ARM`, `ARM` or `IAR`.

You can set the target and toolchain for this application once by entering the following two commands (while in the top-level directory of the cloned repo):

`mbed target UBLOX_C030`

`mbed toolchain GCC_ARM`

Now you are ready to run the tests.  Connect your board to your PC and check that it can be found by entering:

`mbedls`

You should get back something like (this is for a C030 board):

```
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
| platform_name | platform_name_unique | mount_point | serial_port | target_id                | daplink_version |
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
| UBLOX_C030    | UBLOX_C030[0]        | D:          | COM1        | 12340201E3953CC69934E380 | 0201            |
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
```

If you get back the platform NZ32_SC151 (target_id commencing 6660) then you will need to get mbed to pretend it is a C030 board.  You can do this with the following command:

`mbedls --mock 6660:UBLOX_C030`

Now you can build and run the tests for all of mbed with the following command line:

`mbed test`

Or if you want to test just the drivers in the directory, use the following command line:

`mbed test -n driver*`

You should see output something like:

```
Building library mbed-build (UBLOX_C030, GCC_ARM)
Scan: c030
Scan: FEATURE_BLE
Scan: FEATURE_COMMON_PAL
Scan: FEATURE_LWIP
Scan: FEATURE_UVISOR
Scan: FEATURE_ETHERNET_HOST
Scan: FEATURE_LOWPAN_BORDER_ROUTER
Scan: FEATURE_LOWPAN_HOST
Scan: FEATURE_LOWPAN_ROUTER
Scan: FEATURE_NANOSTACK
Scan: FEATURE_NANOSTACK_FULL
Scan: FEATURE_THREAD_BORDER_ROUTER
Scan: FEATURE_THREAD_END_DEVICE
Scan: FEATURE_THREAD_ROUTER
Scan: FEATURE_STORAGE
Scan: GCC_ARM
Scan: FEATURE_LWIP
Building project default (UBLOX_C030, GCC_ARM)
Scan: GCC_ARM
Scan: FEATURE_LWIP
Scan: default
Compile [100.0%]: main.cpp
[Warning] main.cpp@44,0:  #1-D: last line of file ends without a newline
Link: default
Elf2Bin: default
+-----------+-------+-------+------+
| Module    | .text | .data | .bss |
+-----------+-------+-------+------+
| Misc      | 36697 |   328 | 9944 |
| Subtotals | 36697 |   328 | 9944 |
+-----------+-------+-------+------+
Allocated Heap: unknown
Allocated Stack: unknown
Total Static RAM memory (data + bss): 10272 bytes
Total RAM memory (data + bss + heap + stack): 10272 bytes
Total Flash memory (text + data + misc): 37025 bytes
Image: BUILD/tests/UBLOX_C030/GCC_ARM/driver/battery-gauge-bq27441/TESTS/unit_tests/default/default.bin


Memory map breakdown for built projects (values in Bytes):
+---------+------------+-----------+------------+-------+------+-----------+-------------+
| name    | target     | toolchain | static_ram | stack | heap | total_ram | total_flash |
+---------+------------+-----------+------------+-------+------+-----------+-------------+
| default | UBLOX_C030 | GCC_ARM   |      10272 |     0 |    0 |     10272 |       37025 |
+---------+------------+-----------+------------+-------+------+-----------+-------------+

Build successes:
  * UBLOX_C030::GCC_ARM::DRIVER-BATTERY-GAUGE-BQ27441-TESTS-UNIT_TESTS-DEFAULT
  * UBLOX_C030::GCC_ARM::MBED-BUILD
mbedgt: greentea test automation tool ver. 1.2.2
mbedgt: test specification file 'C:\projects\c030\BUILD\tests\UBLOX_C020\GCC_ARM\test_spec.json' (specified with --test-spec option)
mbedgt: using 'C:\projects\c030\BUILD\tests\UBLOX_C030\GCC_ARM\test_spec.json' from current directory!
mbedgt: detecting connected mbed-enabled devices...
mbedgt: detected 1 device
mbedgt: processing target 'UBLOX_C030' toolchain 'GCC_ARM' compatible platforms... (note: switch set to --parallel 1)
mbedgt: test case filter (specified with -n option)
        test filtered in 'driver-battery-gauge-bq27441-tests-unit_tests-default'
mbedgt: running 1 test for platform 'UBLOX_C030' and toolchain 'GCC_ARM'
mbedgt: mbed-host-test-runner: started
mbedgt: checking for GCOV data...
mbedgt: test on hardware with target id: 12340201E3953CC69934E380
mbedgt: test suite 'driver-battery-gauge-bq27441-tests-unit_tests-default' .............................. OK in 10.46 sec
        test case: 'Testing initialisation' .......................................................... OK in 0.05 sec
mbedgt: test case summary: 1 pass, 0 failures
mbedgt: all tests finished!
mbedgt: shuffle seed: 0.7592919699
mbedgt: test suite report:
+--------------------+---------------+-------------------------------------------------------+--------+--------------------+-------------+
| target             | platform_name | test suite                                            | result | elapsed_time (sec) | copy_method |
+--------------------+---------------+-------------------------------------------------------+--------+--------------------+-------------+
| UBLOX_C030-GCC_ARM | UBLOX_C030    | driver-battery-gauge-bq27441-tests-unit_tests-default | OK     | 10.46              | shell       |
+--------------------+---------------+-------------------------------------------------------+--------+--------------------+-------------+
mbedgt: test suite results: 1 OK
mbedgt: test case report:
+---------------------+---------------+-------------------------------------------------------+------------------------+--------+--------+--------+--------------------+
| target              | platform_name | test suite                                            | test case              | passed | failed | result | elapsed_time (sec) |
+---------------------+---------------+-------------------------------------------------------+------------------------+--------+--------+--------+--------------------+
|  UBLOX_C030-GCC_ARM | UBLOX_C030    | driver-battery-gauge-bq27441-tests-unit_tests-default | Testing initialisation | 1      | 0      | OK     | 0.05               |
+---------------------+---------------+-------------------------------------------------------+------------------------+--------+--------+--------+--------------------+
```

# Low Power Modes

The ability to enter and leave low power mdoes can be influenced by the debug chip on the mbed board.  It may not be possible to run test of low power mode using `mbed test` as usual as the process of downloading to the board causes the debug chip to put the target MCU into the wrong state.  A way around this is to download the build, power off the board entirely, power it up again and then run the tests without the download step, using:

`mbedhtrun --skip-flashing -p COMx:9600`

...where `x` is replaced by the number of the COM port where the board is attached.

# What To Do If You Are Not Interested In Tests

As well as the tests for the code in here, there is a `main.cpp` file which is usually not used as it gets in the way of compilation of the tests.  If you wish to use it, edit it and change the `#if 0` to be `#if 1`.  Then you can compile it with:

`mbed compile`

You will find the output files in the sub-directory `BUILD/UBLOX_C030/GCC_ARM/`.  Drag and drop the `.bin` file onto the mbed mapped drive presented by the C030 board and wait for it to program.  Then connect a serial terminal (e.g. PuTTY or TeraTerm) to the mbed COM port (@ 9600/8/N/1) presented by the C030 board.  Press the reset button on the board and you should see serial output. It doesn't do a great deal, that's what the tests are for.

# Debugging

In order to run a debugger on the target, which is best done under Eclipse, you should follow the instructions [here](http://erika.tuxfamily.org/wiki/index.php?title=Tutorial:_STM32_-_Integrated_Debugging_in_Eclipse_using_GNU_toolchain).

This will involve installing [Eclipse (Mars)](https://eclipse.org/mars/) plus the GDB hardware plugin, the [ST-Link utility](http://www.st.com/en/embedded-software/stsw-link004.html), and a GDB server (the page suggests [Texane](https://github.com/texane/stlink).  There are Eclipse projects files in this repo, import them into Eclipse; configure the Eclipse debug launch options as the above web page directs, launch the Texane GDB server (the website says to launch the ST-Link utility also but that doesn't appear to be necessary, the Texane GDB server does everything), compile the target with debug information included (see below) and you should be able to see what's going on with source level debug.

Note: I find that the Texane GDB server crashes when you close a debug session.   If, when starting a debug session, Eclipse reports the somewhat strange error:

```
Error in final launch sequence
Failed to execute MI command:
-target-select remote localhost:4242

Error message from debugger back end:
localhost:4242: The system tried to join a drive to a directory on a joined drive.
```

...then it probably means the Texane GDB server is not running.

By default mbed builds with maximum optimisation and no debug information, i.e. a release build.  To build in such a way that source level single stepping is available, do a clean build with the following switch added to your compilation command line:

`--profile mbed-os/tools/profiles/debug.json`

...so for instance:

`mbed compile -c --profile mbed-os/tools/profiles/debug.json`

To run just that test, go and find the built binary (down in `BUILD/tests/...`) and drag/drop it onto the mbed mapped drive presented by the board.  To  run the test under a debugger, run `mbedls` as above to determine the COM port that your board is connected to.  Supposing it is `COM1`, you would then start the target board under your debugger and, on the PC side, enter the following to begin the test:

`mbedhtrun --skip-flashing --skip-reset -p COM1:9600`