#####################################################################
#   AVD ShadowMask Setup
#####################################################################
#
# Support API level: 23 - 36
#
# For developing ShadowMask, just use:
# ./build.py emulator
#
# This script will stop zygote, simulate the ShadowMask start up process
# that would've happened before zygote was started, and finally
# restart zygote. This is useful for setting up the emulator for
# developing ShadowMask, testing modules, and developing root apps using
# the official Android emulator (AVD) instead of a real device.
#
# This only covers the "core" features of ShadowMask. For testing
# shadowmaskinit, please checkout avd_patch.sh.
#
#####################################################################

mount_tmpfs() {
  # If a file name 'shadowmask' is in current directory, mount will fail
  mv shadowmask shadowmask.tmp
  mount -t tmpfs -o 'mode=0755' shadowmask $1
  mv shadowmask.tmp shadowmask
}

mount_sbin() {
  mount_tmpfs /sbin
  chcon u:object_r:rootfs:s0 /sbin
}

if [ ! -f /system/build.prop ]; then
  # Running on PC
  echo 'Please run `./build.py emulator` instead of directly executing the script!'
  exit 1
fi

cd /data/local/tmp
chmod 755 busybox

if [ -z "$FIRST_STAGE" ]; then
  export FIRST_STAGE=1
  export ASH_STANDALONE=1
  if [ $(./busybox id -u) -ne 0 ]; then
    # Re-exec script with root
    exec /system/xbin/su 0 /data/local/tmp/busybox sh $0
  else
    # Re-exec script with busybox
    exec ./busybox sh $0
  fi
fi

pm install -r -g $(pwd)/shadowmask.apk

# Extract files from APK
unzip -oj shadowmask.apk 'assets/util_functions.sh' 'assets/stub.apk'
. ./util_functions.sh

api_level_arch_detect

unzip -oj shadowmask.apk "lib/$ABI/*" -x "lib/$ABI/libbusybox.so"
for file in lib*.so; do
  chmod 755 $file
  mv "$file" "${file:3:${#file}-6}"
done

if $IS64BIT && [ -e "/system/bin/linker" ]; then
  unzip -oj shadowmask.apk "lib/$ABI32/libshadowmask.so"
  mv libshadowmask.so shadowmask32
  chmod 755 shadowmask32
fi

# Stop zygote (and previous setup if exists)
shadowmask --stop 2>/dev/null
stop
if [ -d /debug_ramdisk ]; then
  umount -l /debug_ramdisk 2>/dev/null
fi

# Make sure boot completed props are not set to 1
setprop sys.boot_completed 0

# Mount /cache if not already mounted
if ! grep -q ' /cache ' /proc/mounts; then
  mount -t tmpfs -o 'mode=0755' tmpfs /cache
fi

SHADOWMASKTMP=/sbin

# Setup bin overlay
if mount | grep -q rootfs; then
  # Legacy rootfs
  mount -o rw,remount /
  rm -rf /root
  mkdir /root /sbin 2>/dev/null
  chmod 750 /root /sbin
  ln /sbin/* /root
  mount -o ro,remount /
  mount_sbin
  ln -s /root/* /sbin
elif [ -e /sbin ]; then
  # Legacy SAR
  mount_sbin
  mkdir -p /dev/sysroot
  block=$(mount | grep ' / ' | awk '{ print $1 }')
  [ $block = "/dev/root" ] && block=/dev/block/vda1
  mount -o ro $block /dev/sysroot
  for file in /dev/sysroot/sbin/*; do
    [ ! -e $file ] && break
    if [ -L $file ]; then
      cp -af $file /sbin
    else
      sfile=/sbin/$(basename $file)
      touch $sfile
      mount -o bind $file $sfile
    fi
  done
  umount -l /dev/sysroot
  rm -rf /dev/sysroot
else
  # Android Q+ without sbin
  SHADOWMASKTMP=/debug_ramdisk
  mount_tmpfs /debug_ramdisk
fi

# ShadowMask stuff
mkdir -p $SHADOWMASKBIN 2>/dev/null
unzip -oj shadowmask.apk 'assets/*.sh' -d $SHADOWMASKBIN
mkdir /data/adb/modules 2>/dev/null
mkdir /data/adb/post-fs-data.d 2>/dev/null
mkdir /data/adb/service.d 2>/dev/null

for file in shadowmask shadowmask32 shadowmaskpolicy stub.apk; do
  chmod 755 ./$file
  cp -af ./$file $SHADOWMASKTMP/$file
  cp -af ./$file $SHADOWMASKBIN/$file
done
cp -af ./shadowmaskboot $SHADOWMASKBIN/shadowmaskboot
cp -af ./shadowmaskinit $SHADOWMASKBIN/shadowmaskinit
cp -af ./busybox $SHADOWMASKBIN/busybox

ln -s ./shadowmask $SHADOWMASKTMP/su
ln -s ./shadowmask $SHADOWMASKTMP/resetprop
ln -s ./shadowmaskpolicy $SHADOWMASKTMP/supolicy

mkdir -p $SHADOWMASKTMP/.shadowmask/device
mkdir -p $SHADOWMASKTMP/.shadowmask/worker
mount_tmpfs $SHADOWMASKTMP/.shadowmask/worker
mount --make-private $SHADOWMASKTMP/.shadowmask/worker
touch $SHADOWMASKTMP/.shadowmask/config

export SHADOWMASKTMP
MAKEDEV=1 $SHADOWMASKTMP/shadowmask --preinit-device 2>&1

RULESCMD=""
rule="$SHADOWMASKTMP/.shadowmask/preinit/sepolicy.rule"
[ -f "$rule" ] && RULESCMD="--apply $rule"

# SELinux stuffs
if [ -d /sys/fs/selinux ]; then
  if [ -f /vendor/etc/selinux/precompiled_sepolicy ]; then
    ./shadowmaskpolicy --load /vendor/etc/selinux/precompiled_sepolicy --live --shadowmask $RULESCMD 2>&1
  elif [ -f /sepolicy ]; then
    ./shadowmaskpolicy --load /sepolicy --live --shadowmask $RULESCMD 2>&1
  else
    ./shadowmaskpolicy --live --shadowmask $RULESCMD 2>&1
  fi
fi

# Boot up
$SHADOWMASKTMP/shadowmask --post-fs-data
start
$SHADOWMASKTMP/shadowmask --service
# Make sure reset nb prop after zygote starts
sleep 2
$SHADOWMASKTMP/shadowmask --boot-complete
