DESCRIPTION = "Media Player Applications for Telechips Automoitve Linux SDK"
SECTION = "applications"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://include/TMPlayer.h;beginline=1;endline=24;md5=930d29d182ca0d85c8e1c16f92a30481"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tmplayer.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH} \
           file://tmplayer.init.sh \
	  	   file://tmplayer.service \
"
SRCREV = "103e7d3617e245abb78aae23e010a195cfb766a3"

UPDATE_RCD := "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', 'update-rc.d', d)}"

inherit autotools pkgconfig ${UPDATE_RCD}

DEPENDS += "dbus glib-2.0 libtcutils libtcconnect libtcdbgen alsa-lib pulseaudio"
S = "${WORKDIR}/git"

# for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "tmplayer.service"

# for sysvinit
INIT_NAME = "tmplayer"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 90 2 5 . stop 20 0 1 6 ."

PACKAGECONFIG = " \
	${@bb.utils.contains('INVITE_PLATFORM', 'apple', 'iap2', '', d)} \
	${@bb.utils.contains('INVITE_PLATFORM', 'bluetooth', 'a2dp', '', d)} \
"

PACKAGECONFIG[iap2] = "--enable-iap2,,tc-iap2-process,tc-iap2-process"
PACKAGECONFIG[a2dp] = "--enable-a2dp,,,tc-bluetooth-manager"

do_install_append() {
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/tmplayer.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -m 644 ${WORKDIR}/tmplayer.service ${D}/${systemd_unitdir}/system
	fi


	install -d ${D}${sysconfdir}/default/volatiles
	echo "d ${PN} ${PN} 0755 ${localstatedir}/run/${INIT_NAME} none" \
	     > ${D}${sysconfdir}/default/volatiles/90_${INIT_NAME}
}

FILES_${PN} += " \
		${sysconfdir} \
		${localstatedir} \
		${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
"
