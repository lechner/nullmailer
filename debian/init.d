#!/bin/sh -e
#
# /etc/init.d/nullmailer: start or stop nullmailer
#
# written by Adam McKenna <adam@flounder.net>

test -x /usr/sbin/nullmailer-send || exit 0

case "$1" in
    start)
	echo -n "Starting mail-transport-agent: nullmailer"
	sh -c "start-stop-daemon --start --quiet -c mail\
		--exec /usr/sbin/nullmailer-send > /dev/null 2>&1 &"
	echo " ."
	;;
    stop)
	echo -n "Stopping mail-transport-agent: nullmailer"
	if [ "`pidof /usr/sbin/nullmailer-send`" ]; then
	    start-stop-daemon --user mail --stop \
	    	--quiet --oknodo --exec /usr/sbin/nullmailer-send
	fi
	echo " ."
	;;
    restart)
	$0 stop
	$0 start
	;;
    reload|force-reload)
    	echo -n "Reloading nullmailer configuration files"
	start-stop-daemon --stop --quiet --oknodo --signal HUP --exec /usr/sbin/nullmailer-send
	;;
    *)
	echo 'Usage: /etc/init.d/nullmailer {start|stop|restart}'
	exit 1
esac

exit 0
