DESCRIPTION = "Connect Library for Telechips Linux AVN"
SECTION = "libs"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://include/TCConnectInterface.h;beginline=1;endline=24;md5=bc2f98739b5189f8631ffc5a68b4128d"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/libtc-connect.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"
SRCREV = "f1d7b10238b35e1373ec12be185532c0eca74fad"

inherit autotools pkgconfig

DEPENDS += "glib-2.0"
PATCHTOOL = "git"
LINKER_HASH_STYLE = "sysv"

S = "${WORKDIR}/git"
