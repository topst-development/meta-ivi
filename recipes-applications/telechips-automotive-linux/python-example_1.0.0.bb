
SUMMARY = "python example"
LICENSE = "CLOSED"
# LIC_FILES_CHKSUM = "file://LICENSE.rst;md5=614e59f1ee7d887b84f2112e3ba26d7c"

SRC_URI = " \
    file://matrix_demo.py \
    "

# S = "${WORKDIR}"

# FILES_${PN} += "${base_prefix}/home/root/*"

# DEPENDS += "python3-modules python3-setuptools python3-setuptools-native"
# RDEPENDS_${PN} += "python3-modules python3-setuptools python3-setuptools-native"

do_install() {
    # install -d ${D}/home/root/
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/matrix_demo.py ${D}${bindir}
}
