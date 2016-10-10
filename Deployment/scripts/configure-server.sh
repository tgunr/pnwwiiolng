#! /bin/bash
######################################################################
# Usage: (as root user)
#  ROLE="strat|chat|map|cell" prepare-host.sh
#
# Installs packages, configures nfs, etc.

######################################################################
## Helper functions

ww2copy() {
  rm -rf "${1}"
  su -c "cp -dR --preserve=mode,timestamps '/playnet/old${1}' '${1}'" -l wwiiol
}
ww2mkdir() {
  rm -rf ${1}
  su -c "mkdir -p '${1}'" -l wwiiol
}

######################################################################
## Script entry point.

# Require this script to be run as root.
if [ $(id -u) -ne 0 ] ; then
  echo "ERROR: Must be run as superuser or via sudo"
  exit -1
fi

# Configure Apt proxy
_proxyFile=/etc/apt/apt.conf.d/02proxy
if [ ! -f "${_proxyFile}" ] ; then
	echo "* Configuring APT Proxy/Cache"
	echo 'Acquire::http { Proxy "http://66.28.224.242:3142"; };' >$_proxyFile
fi

# Determine which role we are running.
Role=""
Strat=0 ; Chat=0 ; Map=0 ; Cell=0
case ${ROLE} in
	strat) Role="stratd" ; Strat=1 ;;
	chat) Role="chatd"; Chat=1 ;;
	map) Role="mmhostd"; Map=1 ;;
	cell) Role="cellhost"; Cell=1 ;;
	*)
		echo "USAGE: ROLE=strat|map|chat|cell $0"
		exit 1
	;;
esac

# Get the user ID of the wwiiol user.
Uid=$(id -u wwiiol)
if [ -z ${Uid} ] ; then echo "ERROR: No 'wwiiol' user found" ; exit 2 ; fi
Gid=$(id -g wwiiol)
if [ -z ${Gid} ] ; then echo "ERROR: No 'wwiiol' group found" ; exit 3 ; fi

# Create the playnet directory.
if [ ! -d /playnet ] ; then
	echo "* Creating /playnet"
	mkdir /playnet
fi
chown wwiiol.wwiiol /playnet

# LVM2 will be required on the strat server, everyone needs vim and nfs common.
echo "* Installing stock system components"
apt-get install -q -q -y nfs-common vim lvm2 byobu
apt-get purge -q -q -y ppp pppoeconf pppconfig apparmor apparmor-utils apport apport-symptoms \
	laptop-detect powermgmt-base wpasupplicant wireless-tools

# Strat server should have it's prior incarnation available as an LVM volume.
if [ ${Strat} -eq 1 ] ; then
  echo "* Activating pre-existing LVM2 mounts"
  vgchange -a y
  lvscan
  ww2mkdir /playnet/old

  mounted=0

  # Can we find the old root lvm partition?
  echo "* Locating previous root partition."
  if [ -e /dev/VolGroup00 ] ; then
    # get a list of logical volume names (lvs -o lv_name)
    # sort them by descending size (-O -lv_size)
    # take the first one (head -1)
    # and that should be the old root partition.
	if [ -e /dev/VolGroup00/lvRoot ] ; then
		rootGuess='lvRoot'
	else
		rootGuess=$(lvs --noheadings -o lv_name -O -lv_size VolGroup00 | head -1)
	fi
    if [ ! -z "${rootGuess}" ] ; then
      lvPath="/dev/VolGroup00/${rootGuess// }"	# Strip spaces
      if [ -e "${lvPath}" ] ; then
        echo "* Guessing old root folder is ${lvPath}"
        echo >>/etc/fstab "# Mount the old root filesystem"
        echo >>/etc/fstab "${lvPath} /playnet/old ext3 defaults,noatime,nodiratime,ro 1 1"
        mount /playnet/old
        if [ -d /playnet/old/playnet ] ; then mounted=1 ; fi
      fi
    fi
  fi

  if [ ${mounted} -eq 0 ] ; then
    # Ask them to configure /playnet/old
    PS1="# Please mount old drive as /playnet/old and then type exit.\n\w# " \
      bash --norc -i
  fi

  # Check they did it.
  if [ ! -d /playnet/old/playnet ] ; then
    echo "ERROR: Unable to locate /playnet/old/playnet -- aborting."
	exit 1
  fi
fi

ww2mkdir /home/wwiiol/etc
echo "${Role}" >/home/wwiiol/etc/berole.rc ; chown wwiiol.wwiiol /home/wwiiol/etc/berole.rc

echo "* Installing additional components"
apt-get install -y lua5.1 liblua5.1-0 liblua5.1-json subversion mysql-client gdb

if [ ${Strat} -eq 1 ] ; then
  echo "* Installing strat-specific features"
  apt-get install -y nfs-kernel-server mysql-server

  echo "* Configuring exports"
  echo "/playnet 192.168.10.0/24(sec=sys,ro,async,no_subtree_check,all_squash,anonuid=1000,anongid=1000,fsid=0)" >>/etc/exports
else
  echo "* Installing non-strat specific features"

  echo "* Configuring NFS mounts"
  echo "192.168.10.1:/ /playnet nfs4 auto,async,ro,nolock,intr 0 0" >>/etc/fstab
fi

# Need to modify .bashrc
tmpRc=/tmp/new.bashrc
cat >/tmp/new.bashrc <<END_BASHRC
# Load the necessary paths before we do any fancy stuff.
if [ -f /playnet/etc/wwiiolrc ]; then
        . /playnet/etc/wwiiolrc
fi

for dir in /playnet/bin /playnet/wwiiol/bin ~/bin
do
        [ -d \$dir -a -x \$dir ] && PATH=\$PATH:\$dir
done

export PATH
END_BASHRC
cat /home/wwiiol/.bashrc >>${tmpRc}
mv ${tmpRc} /home/wwiiol/.bashrc
chown wwiiol.wwiiol /home/wwiiol/.bashrc

ln -s /playnet/wwiiol /usr/local/wwiiol
ln -s /playnet/teulKit /usr/local/teulKit

echo "* Configuring NFS services"
perl -i -pe 's/^(Domain\s*=)\s*.*/\1 playnet.com/; s/^(Nobody-User|Nobody-Group\s*=)\s*.*/\1 wwiiol/;' /etc/idmapd.conf
perl -i -pe 's/^(NEED_STATD|NEED_GSSD)=.*/${1}=no/; s/^(NEED_IDMAPD)=.*/$1=yes/;' /etc/default/nfs-common

modprobe nfs
service lockd restart
service idmapd stop
service idmapd start

if [ ${Strat} -eq 1 ] ; then
  service nfs-kernel-server restart

  echo "Todo:"
  echo "- Re-populate /playnet,"
  echo "- Re-populate /home/wwiiol,"
  echo "- Copy my.cnf to /etc/mysql/my.cnf and edit,"
  echo "- Fix & restart the database"
else
  mount /playnet
fi

# Add the playnet librarys folder to ldconfig.
echo "* Adding /playnet/wwiiol/lib to library config"
echo -e "# PlayNet product specific libraries.\n/playnet/wwiiol/lib" >/etc/ld.so.conf.d/playnet.conf
ldconfig
ldconfig /playnet/wwiiol/lib

