DESCRIPTION = "HUD Demo Applications for Telechips Linux AVN"
SECTION = "applications"
LICENSE = "Telechips"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-telechips/meta-core/licenses/Telechips;md5=e23a23ed6facb2366525db53060c05a4"

SRC_URI = " \
	file://gst-self-test.c \
	file://Hud_demo_2020.mp4 \
	file://tc-hud-app.service \
"

inherit autotools pkgconfig systemd

DEPENDS += "gstreamer1.0 gstreamer1.0-plugins-base"

# for systemd
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "tc-hud-app.service"

# for sysvinit
INIT_NAME = "tc-hud-app"

INITSCRIPT_NAME = "${INIT_NAME}"
INITSCRIPT_PARAMS = "start 92 5 . stop 20 0 1 6 ."

S = "${WORKDIR}"

do_configure[noexec] = "1"

do_compile() {
	GST_INC=`pkg-config --cflags gstreamer-1.0 gstreamer-pbutils-1.0`
	GST_LIB=`pkg-config --libs gstreamer-1.0 gstreamer-pbutils-1.0`
	$CC ${S}/gst-self-test.c -o g_player_self_test $CFLAGS $GST_INC $LDFLAGS $GST_LIB
}

do_install() {
	install -d	${D}${bindir}
	install -m 755 ${B}/g_player_self_test		${D}${bindir}

	install -d ${D}/${sysconfdir}
	install -m 0644 ${WORKDIR}/Hud_demo_2020.mp4 ${D}${sysconfdir}
	if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
		install -d ${D}${sysconfdir}/init.d
		install -m 0755 ${WORKDIR}/tc-hud-app.init.sh ${D}${sysconfdir}/init.d/${INIT_NAME}
	else
		install -d ${D}/${systemd_unitdir}/system
		install -d ${D}/${sysconfdir}
		install -m 644 ${WORKDIR}/tc-hud-app.service ${D}/${systemd_unitdir}/system/tc-hud-app.service
	fi
}

FILES_${PN} += " \
	${bindir} \
	${sysconfdir} \
	${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_unitdir}', '', d)} \
"

RDEPENDS_${PN} += "gstreamer1.0-meta-video"
