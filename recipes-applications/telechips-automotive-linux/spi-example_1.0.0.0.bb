# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# WARNING: the following LICENSE and LIC_FILES_CHKSUM values are best guesses - it is
# your responsibility to verify that the values are complete and correct.
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2c1c00f9d3ed9e24fa69b932b7e7aff2"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/spi-example.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"

# Modify these as desired
PV = "${SRCPV}"
#SRCREV = "${AUTOREV}"
SRCREV = "b3392a9f63ae7022764eb27eee27f1edce502655"
S = "${WORKDIR}/git"

# NOTE: if this software is not capable of being built in a separate build directory
# from the source, you should replace autotools with autotools-brokensep in the
# inherit line
inherit autotools pkgconfig

# Specify any options you want to pass to the configure script using EXTRA_OECONF:
EXTRA_OECONF = ""
