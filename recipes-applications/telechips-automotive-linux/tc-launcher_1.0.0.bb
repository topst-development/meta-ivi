DESCRIPTION = "Launcher Applications for Telechips Linux AVN"
SECTION = "applications"

LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-telechips/meta-core/licenses/Telechips;md5=e23a23ed6facb2366525db53060c05a4"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tc-launcher.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"
SRC_URI += "${@bb.utils.contains("DISTRO_FEATURES", "systemd", \
			bb.utils.contains("INVITE_PLATFORM", "qt5/wayland", "file://tc-launcher.service file://tc-launcher.path", "file://tc-launcher.service.eglfs", d), \
			"file://launcher.init.sh", d)}"
#SRC_URI += "file://0001-modify-config.xml.patch"

#SRCREV = "${AUTOREV}"
SRCREV = "613b04403691fff0dc17ad66358bda78ec067bab"

UPDATE_RCD := "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', 'update-rc.d', d)}"

inherit qmake5 pkgconfig useradd ${UPDATE_RCD}

DEPENDS += "qtbase qtdeclarative libtcutils"

EXTRA_QMAKEVARS_PRE = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'SYSTEMD=1', '', d)}"

PATCHTOOL = "git"

S = "${WORKDIR}/git"
B = "${S}"

# for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "${@bb.utils.contains("INVITE_PLATFORM", "qt5/wayland", "tc-launcher.service tc-launcher.path", "tc-launcher.service", d)}"
SYSTEMD_AUTO_ENABLE_${PN} = "${@bb.utils.contains("INVITE_PLATFORM", 'ivi-extension', 'disable', 'enable', d)}"

# for sysvinit
INIT_NAME = "tc-launcher"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 92 5 . stop 20 0 1 6 ."

USERADD_PACKAGES = "${PN}"
USERADD_PARAM_${PN} = "--system --home ${localstatedir}/lib/${INIT_NAME} \
                       --no-create-home --shell /bin/false \
                       --user-group ${PN}"

do_install_append() {
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/launcher.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}

		install -d ${D}${sysconfdir}/default/volatiles
		echo "d ${PN} ${PN} 0755 ${localstatedir}/run/${INIT_NAME} none" \
		     > ${D}${sysconfdir}/default/volatiles/92_${INIT_NAME}

		mkdir -p ${D}${localstatedir}/lib/${INIT_NAME}
		chown ${PN}:${PN} ${D}${localstatedir}/lib/${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -d ${D}/${sysconfdir}
		if ${@bb.utils.contains("INVITE_PLATFORM", "qt5/wayland", "true", "false", d)}; then
			install -m 644 ${WORKDIR}/tc-launcher.service ${D}/${systemd_unitdir}/system/tc-launcher.service
			install -m 644 ${WORKDIR}/tc-launcher.path ${D}/${systemd_unitdir}/system/tc-launcher.path
		else
			install -m 644 ${WORKDIR}/tc-launcher.service.eglfs ${D}/${systemd_unitdir}/system/tc-launcher.service
			sed -i "s%\(^Environment=QT_QPA_EGLFS_PHYSICAL_WIDTH=\)LCD_WIDTH%\1${LCD_WIDTH}%g"		${D}/${systemd_unitdir}/system/tc-launcher.service
			sed -i "s%\(^Environment=QT_QPA_EGLFS_PHYSICAL_HEIGHT=\)LCD_HEIGHT%\1${LCD_HEIGHT}%g"	${D}/${systemd_unitdir}/system/tc-launcher.service
		fi
	fi

	install -d ${D}${datadir}/tcc_launcher/app_images
	cp -ap  ${S}/app_images/media_player	${D}/${datadir}/tcc_launcher/app_images/
	cp -ap  ${S}/app_images/navit			${D}/${datadir}/tcc_launcher/app_images/
	cp -ap  ${S}/app_images/launcher		${D}/${datadir}/tcc_launcher/app_images/
	cp -ap  ${S}/app_images/setting			${D}/${datadir}/tcc_launcher/app_images/

	if ${@bb.utils.contains('INVITE_PLATFORM', 'sw-audio-dsp', 'true', 'false', d)}; then
		sed -i 's%.*\(<app .*EQManager.*/>\).*%    \1%g'  ${D}/${datadir}/tcc_launcher/config.xml
	fi
}

FILES_${PN} += " \
		${datadir} \
		${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${sysconfdir}', '', d)} \
		${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${localstatedir}', '', d)} \
		${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
		${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${sysconfdir}', '', d)} \
"

RDEPENDS_${PN} += "\
	tmplayer-gui \
	tc-setting \
	navit \
	${@bb.utils.contains('INVITE_PLATFORM', 'sw-audio-dsp', 'tc-eq-manager', '', d)} \
"

