# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# WARNING: the following LICENSE and LIC_FILES_CHKSUM values are best guesses - it is
# your responsibility to verify that the values are complete and correct.
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=96af5705d6f64a88e035781ef00e98a8"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/gpio-example.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"

# Modify these as desired
PV = "${SRCPV}"
#SRCREV = "${AUTOREV}"
SRCREV = "0e49b50e6aee0464cefd7302a78afafd33560add"
S = "${WORKDIR}/git"

# NOTE: if this software is not capable of being built in a separate build directory
# from the source, you should replace autotools with autotools-brokensep in the
# inherit line
inherit autotools

# Specify any options you want to pass to the configure script using EXTRA_OECONF:
EXTRA_OECONF = ""

#do_compile() {
#        ${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/gpioExample.c -o gpioExample
#}

#do_install() {
#        install -d ${D}${bindir}
#        install -m 0755 ${S}/gpioExample ${D}${bindir}
#}
