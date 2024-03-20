
SUMMARY = "A library to drive a MAX7219 LED serializer (using SPI) and WS2812 NeoPixels (using DMA)"
HOMEPAGE = "https://github.com/rm-hull/luma.led_matrix"
AUTHOR = "Richard Hull <richard.hull@destructuring-bind.org>"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.rst;md5=614e59f1ee7d887b84f2112e3ba26d7c"

SRC_URI = "https://files.pythonhosted.org/packages/cc/41/8cf7078e77da1ededef2bcc958114afc52725e948b01f16fa3542c914bbd/luma.led_matrix-1.7.0.tar.gz"
SRC_URI[md5sum] = "868576a7860dae3f6710ca418c2e7af0"
SRC_URI[sha256sum] = "0e1803384bd1d44b2e9a91bb1e139910ceb8684a59703d2241f0bbc002f1374f"

S = "${WORKDIR}/luma.led_matrix-1.7.0"

RDEPENDS_${PN} = ""

inherit setuptools3
