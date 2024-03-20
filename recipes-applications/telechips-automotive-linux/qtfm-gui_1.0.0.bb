DESCRIPTION = "Qt File Manager"
SECTION = "applications"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-telechips/meta-core/licenses/Telechips;md5=e23a23ed6facb2366525db53060c05a4"
 
PV = "0.10+${SRCPV}"
SRCREV = "281d25c7bebdeda1b7f80fca7b02c922bf6acd1a"
SRC_URI = "git://github.com/rodlie/qtfm;branch=master;protocol=https \
"

inherit qmake5 pkgconfig
DEPENDS += "qtbase qtdeclarative libtcutils libtcdbgen"

S = "${WORKDIR}/git"
B = "${S}"
