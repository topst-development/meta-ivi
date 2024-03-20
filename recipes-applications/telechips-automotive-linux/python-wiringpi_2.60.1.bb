
SUMMARY = "A python interface to WiringPi 2.0 library which allows for easily interfacing with the GPIO pins of the Raspberry Pi. Also supports i2c and SPI."
HOMEPAGE = "https://github.com/WiringPi/WiringPi-Python/"
AUTHOR = "Philip Howard <phil@gadgetoid.com>"
LICENSE = "LGPL-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=e6a600fd5e1d9cbde2d983680233ad02"

SRC_URI = "https://files.pythonhosted.org/packages/d3/9c/08eb589c718cda9f2168649f1952aa9cb3711dc93d765991a6741cf6a8b7/wiringpi-2.60.1.tar.gz"
SRC_URI[md5sum] = "78beceab8a759be2e6ab6e6d507f180c"
SRC_URI[sha256sum] = "b0c65d5d7c65d0bbef25c56d90237ca4098b1edabc528fb48dc6b61d62cd4b7d"

S = "${WORKDIR}/wiringpi-2.60.1"

RDEPENDS_${PN} = ""

inherit setuptools3
