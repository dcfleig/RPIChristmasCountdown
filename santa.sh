#! /bin/sh
# /etc/init.d/santa
#

# Some things that run always
touch /var/lock/santa

# Carry out specific functions when asked to by the system
case "$1" in
  start)
    echo "Starting script santa "
    sudo /home/pi/RPIChristmasCountdown/countdown -d "`date '+%Y-12-24 16:30:00.000'`" &> /home/rpi/RPIChristmasCountdown/service.log &
    #sudo /home/pi/matrix-clock/matrix-clock --led-chain=4 --led-rows=32 -C 0,0,255 -O 255,255,255 -b20 &

    ;;
  stop)
    echo "Stopping script santa"
    echo "But really doing nothing.  Use kill."
    ;;
  *)
    echo "Usage: /etc/init.d/santa {start|stop}"
    exit 1
    ;;
esac

exit 0
