#! /bin/sh
# If the cluster is not flagged as down and a particular host
# process is not running, then restart the cluster

. /playnet/etc/wwiiolrc

HOST=$1
PIDOF=/bin/pidof
PATH=$PATH:/playnet/bin:/playnet/wwiiol/bin export PATH

if [ ! -r /playnet/wwiiol/bin/${HOST} ] ; then
  echo "ERROR: Unrecognized host process: ${HOST}"
  exit 1
fi

CLUSTER_DOWN=${CLUSTER_DOWN:-/tmp/CLUSTER_DOWN}

if [ -e $CLUSTER_DOWN ] ; then
    echo "Cluster is already down, exiting"
    exit 0
fi

PID=`$PIDOF $HOST`
if [ -z "$PID" ]; then
 (echo -n "* Restarting $HOST: " ; date) >>/tmp/cluster.watch
 echo "Restarting cluster"
 stophost
 sleep 15
 starthost
fi
