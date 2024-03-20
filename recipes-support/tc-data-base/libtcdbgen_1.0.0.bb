DESCRIPTION = "Data-Base Library for Telechips Linux AVN"
SECTION = "libs"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://include/TCDBGen.h;beginline=1;endline=24;md5=a6fe488087d30c8ef3b647064ef3442f"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/libtc-dbgen.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH} \
	   file://tc-dbconf.xml \
"
SRCREV = "b0c46df7216dbbf05e0415cb91e1da7084299367"

inherit autotools pkgconfig

DEPENDS += "sqlite3 libxml2 linux-libc-headers"

PATCHTOOL = "git"
LINKER_HASH_STYLE = "sysv"

S = "${WORKDIR}/git"

EXTRA_OECONF += "${@bb.utils.contains("TUNE_FEATURES", "aarch64", "ARCH=arm64", "", d)}"

do_install_append() {
	install -d ${D}${datadir}/tc-dbgen
	install -m 0644 ${WORKDIR}/tc-dbconf.xml ${D}${datadir}/tc-dbgen/
}

FILES_${PN} += "${datadir}"
