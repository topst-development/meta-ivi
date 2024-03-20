SUMMARY = "Telechips Linux AVN Application Package Groups"
LICENSE = "CLOSED"

PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit packagegroup

RDEPENDS_${PN} = "\
	ttf-nanum-font \
        tc-launcher \
	tc-osd-app \
        audiomanager \
	tc-mode-manager \
	gpio-example \
	camera-example \
	echoserver-example \
	audio-example \
	configuation-tool \
	spi-example \
	python-example \
	python-luma-led-matrix \
	python-rpi-gpio \
	python-spidev \
	python-wiringpi \
	${@bb.utils.contains('INVITE_PLATFORM', 'dispman', 'tc-dispman', '', d)} \
	${@bb.utils.contains('INVITE_PLATFORM', 'sw-audio-dsp', 'tc-eq-manager','', d)} \
	${@bb.utils.contains('INVITE_PLATFORM', 'micom', 'tc-micom-manager', '', d)} \
	${@bb.utils.contains('INVITE_PLATFORM', 'hud-display', 'tc-hud-app', '', d)} \
"
