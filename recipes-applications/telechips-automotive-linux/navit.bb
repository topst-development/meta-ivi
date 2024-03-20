DESCRIPTION = "Navit is a car navigation system with routing engine."
SECTION = "applications"
LICENSE = "GPLv2 & LGPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=04bc107195251f7deb453482dea6ff37 \
                    file://LGPL-2;md5=57b6bd56c33df1d0bb3ae3ff02cf9777 \
                    file://GPL-2;md5=751419260aa954499f7abaabaa882bbe \
"

SRC_URI = "git://github.com/navit-gps/navit;protocol=https;branch=master"
SRC_URI += " \
	file://0001-Change-default-navit-config.patch \
	file://0002-Set-pulseaudio-property.patch \
	file://0003-add-mode-manager.patch \
	file://0004-change-center-of-maps.patch \
	file://0005-Update-configuration.patch \
	file://0006-Sync-function-name-rules.patch \
	file://0007-change-getopt-to-getopt_long-and-add-keyboard-click.patch \
	file://0008-Hide-navit-when-another-application-view.patch \
	file://0009-Release-audio-resource-when-speech-end.patch \
	file://0010-Add-espeak-option-using-shel-env.patch \
	file://0011-add-back-home-key-process.patch \
	file://0012-update-geometry-for-bigger-than-1920x720.patch \
	file://0013-change-libdir-when-using-aarch64-for-fix-build-error.patch \
	file://0014-fix-geometry-when-use-passenger-display.patch \
	file://0015-add-window-hide-conditions.patch \
	file://0016-Fix-can-t-process-key-event-that-sometimes.patch \
	file://0017-Remove-KEY_RETURN-action-for-cluster-BVM.patch \
	file://0018-Remove-no-need-options-for-fix-error-logs.patch \
	file://osm_bbox_126.8,37.5,127.1,37.6.bin \
"
SRCREV = "fc1d77ee989efd0fad7259b4105e893a6075fedd"

inherit cmake_qt5 pkgconfig features_check invite_platform_check

# check mandatory features
REQUIRED_DISTRO_FEATURES = "opengl"

DEPENDS += "gettext libxslt fribidi glib-2.0 python3 fontconfig dbus dbus-glib zlib freetype"
DEPENDS += "libsdl2 virtual/libgles2 virtual/egl libpng gpsd"
DEPENDS += "qtbase qtdeclarative qtsvg qtmultimedia qtsensors qtlocation"
DEPENDS += "gettext-native"
DEPENDS += "libtcutils"

EXTRA_OECMAKE += "-DUSE_QML=1 -DUSE_QWIDGET=0"

PATCHTOOL = "git"

S = "${WORKDIR}/git"

do_install_append() {
	install -m 0644 ${WORKDIR}/osm_bbox_126.8,37.5,127.1,37.6.bin		${D}${datadir}/navit/maps
}

FILES_${PN} += " \
	${datadir}/dbus-1 \
	${datadir}/icons \
"

RDEPENDS_${PN} += "gpsd"
