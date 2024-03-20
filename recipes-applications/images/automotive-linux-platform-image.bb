inherit tcc-ivi-image
inherit ${@bb.utils.contains('INVITE_PLATFORM', 'fw-update', 'make-main-updatedir', '',d)}

IMAGE_INSTALL += " \
    packagegroup-telechips-ivi-multimedia \
    packagegroup-telechips-ivi-graphics \
	${@bb.utils.contains('INVITE_PLATFORM', 'fw-update', 'packagegroup-telechips-ivi-update', '', d)} \
	${@bb.utils.contains('INVITE_PLATFORM', 'str', 'packagegroup-telechips-ivi-str', '', d)} \
	${AUTOMOTIVE_LINUX_DEMO_APPS} \
"

# linux avn application packages
AUTOMOTIVE_LINUX_DEMO_APPS ?= "telechips-automotive-linux-apps v4l-utils i2c-tools pciutils"

# set systemd default taget when using systemd
SYSTEMD_DEFAULT_TARGET = "graphical.target"

UPDATE_ROOTFS_NAME ?= "automotive-linux-platform-image-${MACHINE}.ext4"
