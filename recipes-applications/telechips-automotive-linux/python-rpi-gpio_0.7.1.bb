
SUMMARY = "A module to control Raspberry Pi GPIO channels"
HOMEPAGE = "http://sourceforge.net/projects/raspberry-gpio-python/"
AUTHOR = "Ben Croston <ben@croston.org>"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://setup.py;md5=f85f6b9f8e6b7437e5f58f73ae33e558"

SRC_URI = "https://files.pythonhosted.org/packages/c4/0f/10b524a12b3445af1c607c27b2f5ed122ef55756e29942900e5c950735f2/RPi.GPIO-0.7.1.tar.gz"
SRC_URI[md5sum] = "22704930a4e674a3d35342bde6d69fe5"
SRC_URI[sha256sum] = "cd61c4b03c37b62bba4a5acfea9862749c33c618e0295e7e90aa4713fb373b70"

S = "${WORKDIR}/RPi.GPIO-0.7.1"

RDEPENDS_${PN} = ""

inherit setuptools3
