#SHADOWMASK
############################################
# ShadowMask Flash Script (updater-script)
############################################

##############
# Preparation
##############

# Default permissions
umask 022

OUTFD=$2
COMMONDIR=$INSTALLER/assets
CHROMEDIR=$INSTALLER/assets/chromeos

if [ ! -f $COMMONDIR/util_functions.sh ]; then
  echo "! Unable to extract zip file!"
  exit 1
fi

# Load utility functions
. $COMMONDIR/util_functions.sh

setup_flashable

############
# Detection
############

if echo $SHADOWMASK_VER | grep -q '\.'; then
  PRETTY_VER=$SHADOWMASK_VER
else
  PRETTY_VER="$SHADOWMASK_VER($SHADOWMASK_VER_CODE)"
fi
print_title "ShadowMask $PRETTY_VER Installer"

is_mounted /data || mount /data || is_mounted /cache || mount /cache
mount_partitions
check_data
get_flags
find_boot_image

[ -z $BOOTIMAGE ] && abort "! Unable to detect target image"
ui_print "- Target image: $BOOTIMAGE"

# Detect version and architecture
api_level_arch_detect

[ $API -lt 23 ] && abort "! ShadowMask only support Android 6.0 and above"

ui_print "- Device platform: $ABI"

BINDIR=$INSTALLER/lib/$ABI
cd $BINDIR
for file in lib*.so; do mv "$file" "${file:3:${#file}-6}"; done
cd /
cp -af $INSTALLER/lib/$ABI32/libshadowmask.so $BINDIR/shadowmask32 2>/dev/null

# Check if system root is installed and remove
$BOOTMODE || remove_system_su

##############
# Environment
##############

ui_print "- Constructing environment"

# Copy required files
rm -rf $SHADOWMASKBIN 2>/dev/null
mkdir -p $SHADOWMASKBIN 2>/dev/null
cp -af $BINDIR/. $COMMONDIR/. $BBBIN $SHADOWMASKBIN

# Remove files only used by the ShadowMask app
rm -f $SHADOWMASKBIN/bootctl $SHADOWMASKBIN/main.jar \
  $SHADOWMASKBIN/module_installer.sh $SHADOWMASKBIN/uninstaller.sh

chmod -R 755 $SHADOWMASKBIN

# addon.d
if [ -d /system/addon.d ]; then
  ui_print "- Adding addon.d survival script"
  blockdev --setrw /dev/block/mapper/system$SLOT 2>/dev/null
  mount -o rw,remount /system || mount -o rw,remount /
  ADDOND=/system/addon.d/99-shadowmask.sh
  cp -af $COMMONDIR/addon.d.sh $ADDOND
  chmod 755 $ADDOND
fi

##################
# Image Patching
##################

install_shadowmask

# Cleanups
$BOOTMODE || recovery_cleanup
rm -rf $TMPDIR

ui_print "- Done"
exit 0
