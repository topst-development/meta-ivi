DESCRIPTION = "OSD Applications for Telechips Linux AVN"
SECTION = "applications"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-telechips/meta-core/licenses/Telechips;md5=e23a23ed6facb2366525db53060c05a4"

SRC_URI = "${TELECHIPS_AUTOMOTIVE_APP_GIT}/tc-osd-app.git;protocol=${ALS_GIT_PROTOCOL};branch=${ALS_BRANCH}"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'file://tc-osd-app.service file://tc-osd-app.path', 'file://tc-osd-app.init.sh', d)}"
SRCREV = "0366fffa20e0e31416120a59eec5b6c6e37cff55"

UPDATE_RCD := "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', 'update-rc.d', d)}"

inherit qmake5 pkgconfig ${UPDATE_RCD} externalsrc

DEPENDS += "qtbase qtdeclarative libtcutils"

PATCHTOOL = "git"

S = "${WORKDIR}/git"
B = "${S}"

# for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "tc-osd-app.service tc-osd-app.path"

# for sysvinit
INIT_NAME = "tc-osd-app"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 92 5 . stop 20 0 1 6 ."

do_install_append() {
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/tc-osd-app.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -d ${D}/${sysconfdir}
		install -m 644 ${WORKDIR}/tc-osd-app.service ${D}/${systemd_unitdir}/system/tc-osd-app.service
		install -m 644 ${WORKDIR}/tc-osd-app.path ${D}/${systemd_unitdir}/system/tc-osd-app.path
	fi
}

FILES_${PN} += " \
	${datadir} \
	${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${sysconfdir}', '', d)} \
	${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', '${localstatedir}', '', d)} \
	${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
	${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${sysconfdir}', '', d)} \
"
