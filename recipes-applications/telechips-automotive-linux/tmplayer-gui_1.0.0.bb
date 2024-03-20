DESCRIPTION = "Media Player Qt Applications for Telechips Automotive Linux SDK"
SECTION = "applications"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-telechips/meta-core/licenses/Telechips;md5=e23a23ed6facb2366525db53060c05a4"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tmplayer-gui.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"
SRCREV = "7908d8598acefd3fdb4496517665e61ce39d61a1"

inherit qmake5 pkgconfig

DEPENDS += "qtbase qtdeclarative libtcutils libtcdbgen"
RDEPENDS_${PN} += "tmplayer tc-media-playback"
PATCHTOOL = "git"

S = "${WORKDIR}/git"
B = "${S}"
