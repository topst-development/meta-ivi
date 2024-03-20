#! /bin/sh
### BEGIN INIT INFO
# Provides:             Telechips Audio Manager
# Required-Start:    
# Required-Stop:     
# Default-Start:        2 5
# Default-Stop:         0
# Short-Description:    Telechips Audio Manager
# Description:          AudioManager is a simple audio manager with als-lib for Telechips Audio Manager
### END INIT INFO
#
# -*- coding: utf-8 -*-
# Debian init.d script for Telechips Launcher
# Copyright © 2014 Wily Taekhyun Shin <thshin@telechips.com>

# Source function library.
. /etc/init.d/functions

# /etc/init.d/audio-manager: start and stop the audio-manager daemon

DAEMON=/usr/bin/AudioManager
RUNDIR=/var/run/audio-manager
DESC="telechips audio manager"
ARGUMENTS=""

test -x $DAEMON || exit 0

[ -z "$SYSCONFDIR" ] && SYSCONFDIR=/var/lib/audio-manager
mkdir -p $SYSCONFDIR

check_for_no_start() {
    # forget it if we're trying to start, and /var/lib/audio-manager/audio-manager_not_to_be_run exists
    if [ -e $SYSCONFDIR/audio-manager_not_to_be_run ]; then
	echo "Audio Manager not in use ($SYSCONFDIR/audio-manager_not_to_be_run)"
	exit 0
    fi
}

check_privsep_dir() {
    # Create the PrivSep empty dir if necessary
    if [ ! -d /var/run/audio-manager ]; then
		mkdir -p $RUNDIR
    fi
}

case "$1" in
  start)
	check_for_no_start
  	echo -n "Starting $DESC: "
	check_privsep_dir
	start-stop-daemon -S -x $DAEMON -- $ARGUMENTS
  	echo "done."
	;;
  stop)
  	echo -n "Stopping $DESC: "
	start-stop-daemon -K -x $DAEMON
  	echo "done."
	;;

  restart)
  	echo -n "Restarting $DESC: "
	start-stop-daemon -K --oknodo -x $DAEMON
	check_for_no_start
	check_privsep_dir
	sleep 2
	start-stop-daemon -S -x $DAEMON -- $ARGUMENTS
	echo "."
	;;

  status)
	status $DAEMON
	exit $?
  ;;

  *)
	echo "Usage: /etc/init.d/audio-manager {start|stop|status|restart}"
	exit 1
esac

exit 0
