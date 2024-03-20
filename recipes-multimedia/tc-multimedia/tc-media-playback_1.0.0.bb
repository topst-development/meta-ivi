DESCRIPTION = "Multi Media Playback for Telechips Linux AVN"
SECTION = "libs"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://include/TCMultiMediaType.h;beginline=1;endline=24;md5=5848093de58b05536cc7b63ee2dfb8cd"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tc-media-playback.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'file://tc-media-playback.service', 'file://tc-media-playback.init.sh', d)}"
SRCREV = "0c5b95ab06d4ed6e2c1cad807ff65f036e42aadf"

DEPENDS += "gstreamer1.0 gstreamer1.0-plugins-base dbus glib-2.0 libtcutils"

DEPENDS += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', '', d)}"

S = "${WORKDIR}/git"

UPDATE_RCD := "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', 'update-rc.d', d)}"

inherit autotools-brokensep pkgconfig ${UPDATE_RCD}

EXTRA_OECONF_append = " ${@bb.utils.contains('VIRTUAL-RUNTIME_init_manager', 'systemd', '--enable-systemd=yes', '', d)}"

#for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "tc-media-playback.service"

#for sysvinit
INIT_NAME = "tc-media-playback"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 40 2 5 . stop 20 0 1 6 ."

do_install_append() {
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/tc-media-playback.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}

		install -d ${D}${sysconfdir}/default/volatiles
		echo "d ${PN} ${PN} 0755 ${localstatedir}/run/${INIT_NAME} none" \
				> ${D}${sysconfdir}/default/volatiles/92_${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -m 644 ${WORKDIR}/tc-media-playback.service ${D}/${systemd_unitdir}/system

		if ${@bb.utils.contains('DISTRO_FEATURES', 'pulseaudio', 'true', 'false', d)}; then
			sed -i '/^ExecStart/s/$/ --audio-sink pulsesink --audio-device Primary/g'  ${D}/${systemd_unitdir}/system/tc-media-playback.service
		fi
		if ${@bb.utils.contains('INVITE_PLATFORM', 'hdmi-ext-output', 'true', 'false', d)}; then
			sed -i '/^ExecStart/s/$/ --video-device \/dev\/video11/g'  ${D}/${systemd_unitdir}/system/tc-media-playback.service
		fi
	fi
}

FILES_${PN} += " \
		${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${sysconfdir}', '', d)} \
		${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${localstatedir}', '', d)} \
		${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
"
