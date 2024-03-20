DESCRIPTION = "Audio Manager Applications for Telechips Linux AVN"
SECTION = "applications"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://include/AudioManager.h;beginline=1;endline=24;md5=136a4ec9a6b0aa2e2dee4b81fac8788a"

PROVIDES += "audiomanager"
RPROVIDES_${PN} += "audiomanager"
SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tc-audio-manager.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH} \
		  ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'file://AudioManager.service', 'file://audio-manager.init.sh', d)} \
"
SRCREV = "9984e5277ab255e680c297bdcf440e5fe7af1808"

UPDATE_RCD := "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', 'update-rc.d', d)}"

inherit autotools pkgconfig useradd ${UPDATE_RCD} features_check

CONFLICT_DISTRO_FEATURES = "pulseaudio"

DEPENDS += "dbus libtcutils alsa-lib glib-2.0 libxml2"

PATCHTOOL = "git"

S = "${WORKDIR}/git"

# for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "AudioManager.service"

# for sysvinit
INIT_NAME = "audio-manager"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 40 2 5 . stop 20 0 1 6 ."

USERADD_PACKAGES = "${PN}"
USERADD_PARAM_${PN} = "--system --home ${localstatedir}/lib/${INIT_NAME} \
                       --no-create-home --shell /bin/false \
                       --user-group ${PN}"

EXTRA_OECONF_append = " ${@bb.utils.contains('DISTRO_FEATURES', 'pulseaudio', '--enable-pulseaudio=yes', '', d)}"

do_install_append() {
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/audio-manager.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -m 644 ${WORKDIR}/AudioManager.service ${D}/${systemd_unitdir}/system
		if ${@bb.utils.contains('DISTRO_FEATURES', 'pulseaudio', 'true', 'false', d)}; then
			sed -i '/^Requires/s/$/ pulseaudio.service/g'  ${D}/${systemd_unitdir}/system/AudioManager.service
			sed -i '/^After/s/$/ pulseaudio.service/g'  ${D}/${systemd_unitdir}/system/AudioManager.service
		fi
	fi

	install -d ${D}${sysconfdir}/default/volatiles
	echo "d ${PN} ${PN} 0755 ${localstatedir}/run/${INIT_NAME} none" \
	     > ${D}${sysconfdir}/default/volatiles/90_${INIT_NAME}

	mkdir -p ${D}${localstatedir}/lib/${INIT_NAME}
	chown ${PN}:${PN} ${D}${localstatedir}/lib/${INIT_NAME}
}

FILES_${PN} += " \
		${sysconfdir} \
		${localstatedir} \
		${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
"

