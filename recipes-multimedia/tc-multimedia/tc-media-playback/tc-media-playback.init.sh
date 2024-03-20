#! /bin/sh
### BEGIN INIT INFO
# Provides:             Telechips Media Playback Daemon
# Required-Start:    
# Required-Stop:     
# Default-Start:        2 5
# Default-Stop:         0
# Short-Description:    Telechips Media Playback Daemo
# Description:          Telechips Media Playback Daemon (tc-media-playback) is media play controler for Telechips Linux AVN Process
### END INIT INFO
#
# -*- coding: utf-8 -*-
# Debian init.d script for Telechips Launcher
# Copyright © 2014 Wily Taekhyun Shin <thshin@telechips.com>

# Source function library.
. /etc/init.d/functions

# /etc/init.d/audio-manager: start and stop the audio-manager daemon

DAEMON=/usr/bin/TCMediaPlayback
RUNDIR=/var/run/tc-media-playback
DESC="telechips media playback"
ARGUMENTS="--audio-device pulse"

test -x $DAEMON || exit 0

[ -z "$SYSCONFDIR" ] && SYSCONFDIR=/var/lib/tc-media-playback
mkdir -p $SYSCONFDIR

check_for_no_start() {
    # forget it if we're trying to start, and /var/lib/tc-media-playback/tc-media-playback_not_to_be_run exists
    if [ -e $SYSCONFDIR/tc-media-playback_not_to_be_run ]; then
	echo "tc-media-playback not in use ($SYSCONFDIR/tc-media-playback_not_to_be_run)"
	exit 0
    fi
}

check_privsep_dir() {
    # Create the PrivSep empty dir if necessary
    if [ ! -d /var/run/tc-media-playback ]; then
		mkdir -p $RUNDIR
    fi
}

case "$1" in
  start)
	check_for_no_start
    . /etc/profile.d/profile_local.sh
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
	echo "Usage: /etc/init.d/tc-media-playback {start|stop|status|restart}"
	exit 1
esac

exit 0
