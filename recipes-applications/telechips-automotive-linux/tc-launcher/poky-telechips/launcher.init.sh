#! /bin/sh
### BEGIN INIT INFO
# Provides:             Telechips Launcher
# Required-Start:
# Required-Stop:
# Default-Start:        5
# Default-Stop:         0
# Short-Description:    Launcher Telechips Linux AVN Process
# Description:          LauncherProcess is a simple launcher for Telechips Linux AVN Process
### END INIT INFO
#
# -*- coding: utf-8 -*-
# Debian init.d script for Telechips Launcher
# Copyright © 2014 Wily Taekhyun Shin <thshin@telechips.com>

# Source function library.
. /etc/init.d/functions

DAEMON=/usr/bin/Launcher
RUNDIR=/var/run/tc-launcher
DESC="telechips launcher"
ARGUMENTS="--keyboard=/dev/input/keyboard0"

test -x $DAEMON || exit 0

[ -z "$SYSCONFDIR" ] && SYSCONFDIR=/var/lib/tc-launcher
mkdir -p $SYSCONFDIR

check_for_no_start() {
    # forget it if we're trying to start, and /var/lib/tc-launcher/tc-launcher_not_to_be_run exists
    if [ -e $SYSCONFDIR/tc-launcher_not_to_be_run ]; then
	echo "Telechips Launcher not in use ($SYSCONFDIR/tc-launcher_not_to_be_run)"
	exit 0
    fi
}

check_privsep_dir() {
    # Create the PrivSep empty dir if necessary
    if [ ! -d /var/run/tc-launcher ]; then
		mkdir -p $RUNDIR
    fi
}

case "$1" in
  start)
	check_for_no_start

	. /etc/profile

  	echo -n "Starting $DESC: "
	check_privsep_dir

	if [ "x$TC_LAUNCHER_CONF" != "x" ]; then
		ARGUMENTS="$ARGUMENTS --config=$TC_LAUNCHER_CONF"
	fi

	start-stop-daemon -S -x $DAEMON -- $ARGUMENTS
  	echo "done."
	;;
  stop)
  	echo -n "Stopping $DESC: "
	start-stop-daemon -K -x $DAEMON
  	echo "done."
	;;

  restart)
	. /etc/profile

  	echo -n "Restarting $DESC: "
	start-stop-daemon -K --oknodo -x $DAEMON
	check_for_no_start
	check_privsep_dir
	sleep 2

	if [ "x$TC_LAUNCHER_CONF" != "x" ]; then
		ARGUMENTS="$ARGUMENTS --config=$TC_LAUNCHER_CONF"
	fi

	start-stop-daemon -S -x $DAEMON -- $ARGUMENTS
	echo "done."
	;;

  status)
	status $DAEMON
	exit $?
  ;;

  *)
	echo "Usage: /etc/init.d/tc-launcher {start|stop|status|restart}"
	exit 1
esac

exit 0
