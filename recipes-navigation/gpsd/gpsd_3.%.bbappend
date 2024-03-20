FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
	file://gpsfake.service \
	file://munich.nmea \
	file://seoul.nmea \
	file://lasvegas.nmea \
"

PACKAGECONFIG = "usb"

do_install_append() {
	install -d ${D}${datadir}/gpsd
    install -m 0644 ${WORKDIR}/gpsfake.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/munich.nmea ${D}${datadir}/gpsd/
    install -m 0644 ${WORKDIR}/seoul.nmea ${D}${datadir}/gpsd/
    install -m 0644 ${WORKDIR}/lasvegas.nmea ${D}${datadir}/gpsd/

	install -m 0755 ${S}/gpsfake ${D}${bindir}
    echo "GPS_FAKE_FILE=\"/usr/share/gpsd/seoul.nmea\"" >> ${D}/${sysconfdir}/default/gpsd.default
}

SYSTEMD_SERVICE_${PN} += "gpsfake.service"

FILES_${PN} += "${systemd_unitdir}"

RDEPENDS_${PN} += "gps-utils"
