DESCRIPTION = "Software Configuration tool"
SECTION = "applications"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://tc-config"

S = "${WORKDIR}"

do_configure() {
	:
}

do_compile() {
	:
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${WORKDIR}/tc-config ${D}${bindir}
}
