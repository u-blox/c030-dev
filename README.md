# C030 Development Area

This repo contains the early development code for the C030 board.  This code will later be split into individual libraries.

# Prerequisites

To fetch and build the code in this repository you need first to install the [mbed CLI tools](https://github.com/ARMmbed/mbed-cli#installation) and their prerequisites; this will include a compiler (you can use GCC_ARM, ARM or IAR).  You will need to use `mbed config` to tell the mbed CLI tools where that compiler is.

You will also need to obtain a Vodafone UTM board, along with its LiPo battery and a USB cable to connect it to a PC.

# How To Use This Code

* Clone this repo.
* Change directory to this repo and run:

`mbed update`

...to get the very latest mbed.

You must set the target and the toolchain that you want to use.  At the moment we are using the C027 target (as that pretty much works with the Vodafone UTM board). So the target and toolchain we'll use with this application is `UBLOX_C027` and we will chose the toolchain `ARM`, though note that `GCC_ARM` and `IAR` toolchains are also supported.  To get a list of supported targets and their toolchains enter `mbed compile -S`.

You can set the target and toolchain for this application once by entering the following two commands (while in the top-level directory of the cloned repo):

`mbed target UBLOX_C027`

`mbed toolchain ARM`

Then, *BEFORE* you can begin using the C027 target on the Vodafone UTM board, you will need to hack the contents of one file:

`mbed-os\targets\TARGET_NXP\TARGET_LPC176X\TARGET_UBLOX_C027\C027_api.c`

Edit this file to remove the contents of *all* of the functions in there, so that they are simply stubs.

Once this is done, build the code with:

`mbed compile`

You will find the output files in the sub-directory `.build\UBLOX_C027\ARM\`.  Drag and drop the `.bin` file onto the mbed mapped drive presented by the Vodafone UTM board and wait for it to program.  Then connect a serial terminal (e.g. PuTTY or TeraTerm) to the mbed COM port presented by the Vodafone UTM board.  Press the `RESET_FB` button on the board and you should see serial output.

# Other Things

* By default mbed builds with maximum optimisation and no debug information, i.e. a release build.  To build in such a way that source level single stepping is available, do a clean build with the following switch added to your compilation command line:

`--profile mbed-os/tools/profiles/debug.json`

...so for instance:

`mbed compile -c --profile mbed-os/tools/profiles/debug.json`