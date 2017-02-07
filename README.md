# C030 Development Area

This repo contains the early development code and associated tests for the C030 board.  This code will later be split into individual libraries.

# Prerequisites

To fetch and build the code in this repository you must first install the [mbed CLI tools](https://github.com/ARMmbed/mbed-cli#installation) and their prerequisites; this will include a compiler (you can use GCC_ARM, ARM or IAR).  You will need to use `mbed config` to tell the mbed CLI tools where that compiler is.

Then, to run the tests, you must install the [mbed test tools](https://github.com/ARMmbed/greentea/blob/master/docs/QUICKSTART.md).

Until we have C030 boards, you will also need to obtain a Vodafone UTM board, along with its LiPo battery and a USB cable to connect it to a PC; we're using this for driver development as it includes the LiPo battery gauge, LiPo battery charger and primary cell battery gauge chips that we will use on the C030 board.

# How To Test This Code

Clone this repo.

Change directory to this repo and run:

`mbed update`

...to get the very latest mbed.

Set the target and the toolchain that you want to use.  At the moment we are using the C027 target (as that pretty much works with the Vodafone UTM board). So the target and toolchain we'll use with this application is `UBLOX_C027` and we will chose the toolchain `ARM`, though note that `GCC_ARM` and `IAR` toolchains are also supported.  To get a list of supported targets and their toolchains enter `mbed compile -S`.

You can set the target and toolchain for this application once by entering the following two commands (while in the top-level directory of the cloned repo):

`mbed target UBLOX_C027`

`mbed toolchain ARM`

Then, *BEFORE* you can begin using the C027 target on the Vodafone UTM board, you will need to hack the contents of one file:

`mbed-os\targets\TARGET_NXP\TARGET_LPC176X\TARGET_UBLOX_C027\C027_api.c`

Edit this file to remove the contents of *all* of the functions, so that they are simply stubs.

Now you are ready to run the tests.  Connect your board to your PC and check that it can be found by entering:

`mbedls`

You should get back something like (this is for a C027 or UTM board):

```
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
| platform_name | platform_name_unique | mount_point | serial_port | target_id                | daplink_version |
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
| UBLOX_C027    | UBLOX_C027[0]        | D:          | COM1        | 12340201E3953CC69934E380 | 0201            |
+---------------+----------------------+-------------+-------------+--------------------------+-----------------+
```

Now you can build and run the tests for this project on that board with the following command line:

`mbed test -n driver*`

You should see output something like:

```
Building library mbed-build (UBLOX_C027, ARM)
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
Scan: ARM
Scan: FEATURE_LWIP
Building project default (UBLOX_C027, ARM)
Scan: ARM
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
Image: BUILD/tests/UBLOX_C027/ARM/driver/lipo-gauge-bq27441/TESTS/unit_tests/default/default.bin


Memory map breakdown for built projects (values in Bytes):
+---------+------------+-----------+------------+-------+------+-----------+-------------+
| name    | target     | toolchain | static_ram | stack | heap | total_ram | total_flash |
+---------+------------+-----------+------------+-------+------+-----------+-------------+
| default | UBLOX_C027 | ARM       |      10272 |     0 |    0 |     10272 |       37025 |
+---------+------------+-----------+------------+-------+------+-----------+-------------+

Build successes:
  * UBLOX_C027::ARM::DRIVER-LIPO-GAUGE-BQ27441-TESTS-UNIT_TESTS-DEFAULT
  * UBLOX_C027::ARM::MBED-BUILD
mbedgt: greentea test automation tool ver. 1.2.2
mbedgt: test specification file 'C:\projects\c030\BUILD\tests\UBLOX_C027\ARM\test_spec.json' (specified with --test-spec option)
mbedgt: using 'C:\projects\c030\BUILD\tests\UBLOX_C027\ARM\test_spec.json' from current directory!
mbedgt: detecting connected mbed-enabled devices...
mbedgt: detected 1 device
mbedgt: processing target 'UBLOX_C027' toolchain 'ARM' compatible platforms... (note: switch set to --parallel 1)
mbedgt: test case filter (specified with -n option)
        test filtered in 'driver-lipo-gauge-bq27441-tests-unit_tests-default'
mbedgt: running 1 test for platform 'UBLOX_C027' and toolchain 'ARM'
mbedgt: mbed-host-test-runner: started
mbedgt: checking for GCOV data...
mbedgt: test on hardware with target id: 12340201E3953CC69934E380
mbedgt: test suite 'driver-lipo-gauge-bq27441-tests-unit_tests-default' .............................. OK in 10.46 sec
        test case: 'Testing initialisation' .......................................................... OK in 0.05 sec
mbedgt: test case summary: 1 pass, 0 failures
mbedgt: all tests finished!
mbedgt: shuffle seed: 0.7592919699
mbedgt: test suite report:
+----------------+---------------+----------------------------------------------------+--------+--------------------+-------------+
| target         | platform_name | test suite                                         | result | elapsed_time (sec) | copy_method |
+----------------+---------------+----------------------------------------------------+--------+--------------------+-------------+
| UBLOX_C027-ARM | UBLOX_C027    | driver-lipo-gauge-bq27441-tests-unit_tests-default | OK     | 10.46              | shell       |
+----------------+---------------+----------------------------------------------------+--------+--------------------+-------------+
mbedgt: test suite results: 1 OK
mbedgt: test case report:
+----------------+---------------+----------------------------------------------------+------------------------+--------+--------+--------+--------------------+
| target         | platform_name | test suite                                         | test case              | passed | failed | result | elapsed_time (sec) |
+----------------+---------------+----------------------------------------------------+------------------------+--------+--------+--------+--------------------+
| UBLOX_C027-ARM | UBLOX_C027    | driver-lipo-gauge-bq27441-tests-unit_tests-default | Testing initialisation | 1      | 0      | OK     | 0.05               |
+----------------+---------------+----------------------------------------------------+------------------------+--------+--------+--------+--------------------+
```

# What To Do If You Are Not Interested In Tests

As well as the tests for the code in here, there is a `main.cpp` file which is usually not used as it gets in the way of compilation of the tests.  If you wish to use it, edit it and change the `#if 0` to be `#if 1`.  Then you can compile it with:

`mbed compile`

You will find the output files in the sub-directory `BUILD/UBLOX_C027/ARM/`.  Drag and drop the `.bin` file onto the mbed mapped drive presented by the Vodafone UTM board and wait for it to program.  Then connect a serial terminal (e.g. PuTTY or TeraTerm) to the mbed COM port (@ 9600/8/N/1) presented by the Vodafone UTM board.  Press the `RESET_FB` button on the board and you should see serial output.

# Other Things

By default mbed builds with maximum optimisation and no debug information, i.e. a release build.  To build in such a way that source level single stepping is available, do a clean build with the following switch added to your compilation command line:

`--profile mbed-os/tools/profiles/debug.json`

...so for instance:

`mbed compile -c --profile mbed-os/tools/profiles/debug.json`

To run just that test, go and find the built binary (down in `BUILD/tests/...`) and drag/drop it onto the mbed mapped drive presented by the board.  To  run the test under a debugger, run `mbedls` as above to determine the COM port that your board is connected to.  Supposing it is `COM1`, you would then start the target board under your debugger and, on the PC side, enter the following to begin the test:

`mbedhtrun --skip-flashing --skip-reset -p COM1:9600`