#!/usr/bin/env bash

set -euxo pipefail

# disk will be partitioned as 200MB fat32 + rest of it as luks encrypted
# encrypted partition will use f2fs and will be formatted with 16k block size to match 16k kernel page size
# will use kernel with 16kb page sizes
# boot will use plymouth splash screen
# wifi and ethernet interface will use DHCP and have mDNS enabled
# non-root access will have access to sudo and will auto-login on boot
# root user password will be disabled

# settings

SETUP_DEV=sda

DEBIAN_MIRROR='http://ftp.us.debian.org/debian' # https://www.debian.org/mirror/list

SETUP_MIRROR='https://ca.us.mirror.archlinuxarm.org' # choose from https://archlinuxarm.org/about/mirrors
SETUP_TIMEZONE='America/Los_Angeles'

SETUP_HOSTNAME=cm5
SETUP_FDE=!!YOUR_FDE_PASSWORD!!

SETUP_WIFI_SSID=!!YOUR_WIFI_NAME!!
SETUP_WIFI_PASS=!!YOUR_WIFI_PASSWORD!!

SETUP_USER=!!YOUR_LOGIN_USERNAME!!
SETUP_PASS=!!YOUR_LOGIN_PASSWORD!!

# must run as root

if [ "${EUID}" -ne 0 ]; then
  echo "Must run as root!"
  exit 1
fi

# make sure required packages are installed

pacman -Q cpio                    > /dev/null
pacman -Q curl                    > /dev/null
pacman -Q binutils                > /dev/null
pacman -Q cryptsetup              > /dev/null
pacman -Q util-linux              > /dev/null
pacman -Q dosfstools              > /dev/null
pacman -Q f2fs-tools-git          > /dev/null
pacman -Q qemu-system-aarch64     > /dev/null
pacman -Q qemu-user-static        > /dev/null
pacman -Q qemu-user-static-binfmt > /dev/null

# find disk by path

SETUP_DISK=/dev/disk/by-path/$(udevadm info --query=property --property=ID_PATH --value -n ${SETUP_DEV})
BOOT_PART=${SETUP_DISK}-part1
ROOT_PART=${SETUP_DISK}-part2

lsblk -o 'NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS' ${SETUP_DISK}

read -p '!!! WARNING !!! This will erase everything on the device above. Continue (Y/N)? ' CONTINUE
[[ "${CONTINUE,,}" == "y" ]] || exit 0

# folder for qemu initrd - busybox & kernel for 16k pages

mkdir -p bootstrap-16k

# download 16k page kernel for arm64, plus virtio_blk and f2fs modules

if [[ ! -f bootstrap-16k/vmlinuz ]]; then

  set +x
  DEB_KERNEL_HTML=$(curl -sfL https://packages.debian.org/sid/linux-image-arm64-16k)
  [[ ${DEB_KERNEL_HTML} =~ /sid/(linux-image-[a-z0-9.+]+-arm64-16k) ]] && DEB_KERNEL_IMAGE="${BASH_REMATCH[1]}"

  DEB_KERNEL_HTML=$(curl -sfL https://packages.debian.org/sid/arm64/${DEB_KERNEL_IMAGE}/download)
  [[ ${DEB_KERNEL_HTML} =~ (linux-image-[a-z0-9.+-]+-arm64-16k_[a-z0-9.-]+_arm64.deb) ]] && DEB_KERNEL_NAME=${BASH_REMATCH[1]}
  set -x

  curl -fLO "${DEBIAN_MIRROR}/pool/main/l/linux-signed-arm64/${DEB_KERNEL_NAME}"
  [[ ${DEB_KERNEL_NAME} =~ linux-image-([a-z0-9.+-]+-arm64-16k) ]] && DEB_KERNEL_VERSION=${BASH_REMATCH[1]}

  ar p ${DEB_KERNEL_NAME} data.tar.xz | tar -xJ --strip-components=2 ./boot/vmlinuz-${DEB_KERNEL_VERSION}
  ar p ${DEB_KERNEL_NAME} data.tar.xz | tar -xJ --strip-components=8 ./usr/lib/modules/${DEB_KERNEL_VERSION}/kernel/fs/f2fs/f2fs.ko.xz
  ar p ${DEB_KERNEL_NAME} data.tar.xz | tar -xJ --strip-components=8 ./usr/lib/modules/${DEB_KERNEL_VERSION}/kernel/lib/lz4/lz4_compress.ko.xz
  ar p ${DEB_KERNEL_NAME} data.tar.xz | tar -xJ --strip-components=8 ./usr/lib/modules/${DEB_KERNEL_VERSION}/kernel/lib/lz4/lz4hc_compress.ko.xz
  ar p ${DEB_KERNEL_NAME} data.tar.xz | tar -xJ --strip-components=8 ./usr/lib/modules/${DEB_KERNEL_VERSION}/kernel/drivers/block/virtio_blk.ko.xz

  mv vmlinuz-${DEB_KERNEL_VERSION} bootstrap-16k/vmlinuz
  mv f2fs.ko.xz lz4_compress.ko.xz lz4hc_compress.ko.xz virtio_blk.ko.xz bootstrap-16k/

  rm ${DEB_KERNEL_NAME}

fi

# download static busybox binary for arm64

if [[ ! -f bootstrap-16k/busybox ]]; then

  set +x
  DEB_BUSYBOX_HTML=$(curl -sfL https://packages.debian.org/sid/arm64/busybox-static/download)
  [[ ${DEB_BUSYBOX_HTML} =~ (busybox-static_[a-z0-9.+-]+_arm64.deb) ]] && DEB_BUSYBOX_NAME=${BASH_REMATCH[1]}
  set -x

  curl -fLO "${DEBIAN_MIRROR}/pool/main/b/busybox/${DEB_BUSYBOX_NAME}"
  ar p ${DEB_BUSYBOX_NAME} data.tar.xz | tar -xJ --strip-components=3 ./usr/bin/busybox
  mv busybox bootstrap-16k/

  rm ${DEB_BUSYBOX_NAME}

fi

# commands to remove mounts/luks

LUKS_NAME=cm5-${SRANDOM}

function cleanup {
  find sysroot/etc/pacman.d/gnupg -type s -delete || true # removing sockets exits gpg-agent daemon
  umount -R sysroot/{boot,dev,sys,proc,run,etc/resolv.conf} || true
  cryptsetup close ${LUKS_NAME} || true
}
trap cleanup EXIT

# make partitions

wipefs -a "${SETUP_DISK}"
blkdiscard -f "${SETUP_DISK}" || true
sfdisk --delete "${SETUP_DISK}" || true
echo -e ',200M,c\n,+,\n' | sfdisk "${SETUP_DISK}"

# refreshes partitions under /dev/disk/*
sleep 1
udevadm trigger

# format partitions - boot with FAT32, root with F2FS

mkfs.vfat "${BOOT_PART}"

set +x
echo -n "${SETUP_FDE}" | cryptsetup luksFormat "${ROOT_PART}" --type luks2 --sector-size 4096 --cipher aes-xts-plain64 --key-size 256 --batch-mode
echo -n "${SETUP_FDE}" | cryptsetup luksOpen   "${ROOT_PART}" "${LUKS_NAME}" --allow-discards --persistent --perf-no_read_workqueue --perf-no_write_workqueue --batch-mode
set -x

mkfs.f2fs -b 16384 -w 4096 -O extra_attr,inode_checksum,sb_checksum,packed_ssa -f /dev/mapper/${LUKS_NAME}

# prepare sysroot

rm -rf sysroot
mkdir -p sysroot
mkdir -m 0755 -p sysroot/etc/pacman.d/hooks
mkdir -m 0755 -p sysroot/var/{cache/pacman/pkg,lib/pacman,log}

mount --mkdir "${BOOT_PART}" sysroot/boot

# chroot mounts

touch sysroot/etc/resolv.conf

mount --mkdir --bind /dev  sysroot/dev
mount --mkdir --bind /sys  sysroot/sys
mount --mkdir --bind /proc sysroot/proc
mount --mkdir --bind /run  sysroot/run
mount --bind -o ro $(readlink -m /etc/resolv.conf) sysroot/etc/resolv.conf

function run_in_chroot {
  SYSTEMD_IN_CHROOT=1 chroot -- sysroot "$@"
}

function run_in_chroot_user {
  SYSTEMD_IN_CHROOT=1 chroot --userspec=${SETUP_USER}:users -- sysroot "$@"
}

# disable initramfs generation during download of packages
# filename must match file here: https://gitlab.archlinux.org/archlinux/mkinitcpio/mkinitcpio/-/tree/master/libalpm/hooks

ln -sf /dev/null sysroot/etc/pacman.d/hooks/90-mkinitcpio-install.hook

# initialize pacman keyring

curl -sfLO 'https://github.com/archlinuxarm/PKGBUILDs/raw/refs/heads/master/core/archlinuxarm-keyring/archlinuxarm{.gpg,-trusted,-revoked}'

pacman-key --gpgdir sysroot/etc/pacman.d/gnupg --init
pacman-key --gpgdir sysroot/etc/pacman.d/gnupg --populate --populate-from .

rm archlinuxarm{.gpg,-trusted,-revoked}

# install packages

cat > sysroot/pacman-bootstrap.conf << EOF
[options]
Architecture = aarch64
Color
ParallelDownloads = 5
SigLevel = Never
DisableSandbox

[core]
Server = ${SETUP_MIRROR}/\$arch/\$repo

[extra]
Server = ${SETUP_MIRROR}/\$arch/\$repo

[alarm]
Server = ${SETUP_MIRROR}/\$arch/\$repo

[aur]
Server = ${SETUP_MIRROR}/\$arch/\$repo
EOF

pacman -Sy                                 \
  --noconfirm                              \
  --sysroot "sysroot"                      \
  --config /pacman-bootstrap.conf          \
  archlinuxarm-keyring                     \
  base                                     \
  base-devel                               \
  pacman-contrib                           \
  linux-rpi-16k                            \
  linux-firmware-broadcom                  \
  bash-completion                          \
  foot-terminfo                            \
  rpi5-eeprom                              \
  cryptsetup                               \
  i2c-tools                                \
  plymouth                                 \
  openssh                                  \
  polkit                                   \
  sudo                                     \
  less                                     \
  tmux                                     \
  htop                                     \
  git                                      \
  iwd                                      \
  vim

rm sysroot/etc/resolv.conf.pacnew
rm sysroot/pacman-bootstrap.conf

# pacman mirror & tweaks
echo "Server = ${SETUP_MIRROR}/\$arch/\$repo" > sysroot/etc/pacman.d/mirrorlist 

sed -i 's/^#\(Color\).*$/\1/'           sysroot/etc/pacman.conf
sed -i 's/^#\(VerbosePkgLists\).*$/\1/' sysroot/etc/pacman.conf

# makepkg tweaks
cat > sysroot/etc/makepkg.conf.d/override.conf <<EOF
MAKEFLAGS="-j\`nproc\`"
PKGEXT='.pkg.tar'
EOF

# set hostname
echo "${SETUP_HOSTNAME}" > sysroot/etc/hostname

# set UTF-8 locale
echo 'LANG=C.UTF-8'  > sysroot/etc/locale.conf
echo 'C.UTF-8 UTF-8' > sysroot/etc/locale.gen
run_in_chroot locale-gen

# set time zone
ln -sf /usr/share/zoneinfo/${SETUP_TIMEZONE} sysroot/etc/localtime

# automount /boot
cat > sysroot/etc/fstab << EOF
/dev/mapper/root /     f2fs defaults,noatime,rw 0 1
/dev/mmcblk0p1   /boot vfat defaults,noatime,rw 0 2
EOF

# keep journal log data only in memory
mkdir -m 0755 sysroot/etc/systemd/journald.conf.d
cat > sysroot/etc/systemd/journald.conf.d/override.conf << EOF
[Journal]
Storage=volatile
EOF

# disable DNS fallback servers
mkdir -m 0755 sysroot/etc/systemd/resolved.conf.d
cat > sysroot/etc/systemd/resolved.conf.d/override.conf << EOF
[Resolve]
FallbackDNS=
EOF

# use vim as default editor
echo 'export EDITOR=vim' > sysroot/etc/profile.d/editor.sh

# allow sudo for wheel group
echo '%wheel ALL=(ALL) ALL' > sysroot/etc/sudoers.d/wheel

# ethernet, with DHCP and mDNS
cat > sysroot/etc/systemd/network/eth.network << EOF
[Match]
Name=en*

[Network]
DHCP=yes
MulticastDNS=yes

[Link]
Multicast=yes

[DHCP]
RouteMetric=100
EOF

# wifi settings, with DHCP and mDNS

mkdir -m 0700 sysroot/var/lib/iwd

cat > sysroot/var/lib/iwd/${SETUP_WIFI_SSID}.psk << EOF
[Security]
Passphrase=${SETUP_WIFI_PASS}
EOF

chmod 0600 sysroot/var/lib/iwd/${SETUP_WIFI_SSID}.psk

cat > sysroot/etc/systemd/network/wlan0.network << EOF
[Match]
Name=wlan0

[Network]
DHCP=yes
MulticastDNS=yes

[Link]
Multicast=yes

[DHCP]
RouteMetric=200
EOF

# add non-root use
run_in_chroot useradd -m -g users -G wheel,audio,video,input,disk,input,network,i2c -s /bin/bash -p $(openssl passwd -6 "${SETUP_PASS}") ${SETUP_USER}

# auto login for user
mkdir -p sysroot/etc/systemd/system/getty@tty1.service.d
cat > sysroot/etc/systemd/system/getty@tty1.service.d/override.conf <<EOF
[Service]
ExecStart=
ExecStart=-/sbin/agetty --noreset --noclear --autologin ${SETUP_USER} - \${TERM}
EOF

# disable root account
run_in_chroot passwd -l root

# sd-vconsole hook wants this file
echo 'KEYMAP=us' > sysroot/etc/vconsole.conf

# fixes for wifi issues, uses same options as raspbian
echo 'options brcmfmac roamoff=1 feature_disable=0x282000' > sysroot/etc/modprobe.d/brcmfmac.conf

# kernel commandline
ROOT_UUID=$(blkid -s UUID -o value ${ROOT_PART})
# fsck.mode=skip console=tty1 console=ttyAMA0,115200  vt.global_cursor_default=0
# SYSTEMD_SULOGIN_FORCE=1
echo "root=/dev/mapper/root rd.luks.name=${ROOT_UUID}=root rd.luks.options=discard rw rootwait audit=0 quiet splash logo.nologo loglevel=3 rd.udev.log_level=3 systemd.show_status=false plymouth.ignore-serial-consoles plymouth.boot-log=/dev/null video=HDMI-A-2:1920x1200@60e fsck.repair=yes fsck.mode=skip" > sysroot/boot/cmdline.txt

# aur packages

function aur {
  run_in_chroot_user bash -c "curl -fL https://aur.archlinux.org/cgit/aur.git/snapshot/${1}.tar.gz | tar -C /home/${SETUP_USER} -xzf -"
  run_in_chroot_user bash -c "cd /home/${SETUP_USER}/${1} && makepkg -A"
  pacman --noconfirm --sysroot "sysroot" -U sysroot/home/${SETUP_USER}/${1}/${1}-*.pkg.tar
  rm -rf sysroot/home/${SETUP_USER}/${1}
}

aur yay-bin        # yay aur helper
aur f2fs-tools-git # to get fsck.f2fs that supports 16k page size, normal f2fs-tools does not support it yet
aur paccache-hook  # pacman cache cleanup

# enable/disable startup services

run_in_chroot systemctl enable systemd-networkd  # network
run_in_chroot systemctl enable systemd-resolved  # DNS resolving
run_in_chroot systemctl enable systemd-timesyncd # NTP timesync
run_in_chroot systemctl enable sshd              # openssh
run_in_chroot systemctl enable iwd               # wifi

run_in_chroot systemctl disable systemd-homed
run_in_chroot systemctl disable systemd-userdbd.socket
run_in_chroot systemctl disable systemd-nsresourced.socket

# use systemd-resolved for DNS resolving
umount sysroot/etc/resolv.conf
ln -sf /run/systemd/resolve/stub-resolv.conf sysroot/etc/resolv.conf

# plymouth
curl -sfLo sysroot/usr/share/plymouth/themes/spinner/bgrt-fallback.png https://upload.wikimedia.org/wikipedia/en/thumb/c/cb/Raspberry_Pi_Logo.svg/100px-Raspberry_Pi_Logo.svg.png

# initramfs configuration
# kms is disabled here, because plymouth has some issue with vc4 drm setup, without kms the plymouth will do fallback to /dev/fb0
cat > sysroot/etc/mkinitcpio.conf.d/override.conf << EOF
HOOKS=(base systemd autodetect modconf keyboard keymap plymouth sd-vconsole block sd-encrypt filesystems fsck)
COMPRESSION="lz4"
EOF

# rebuild initramfs
rm sysroot/etc/pacman.d/hooks/90-mkinitcpio-install.hook
run_in_chroot mkinitcpio -k $(ls sysroot/usr/lib/modules/.) -g /boot/initramfs-linux.img -S autodetect

# Raspberry Pi boot configuration
cat > sysroot/boot/config.txt << EOF
kernel=kernel8.img
cmdline=cmdline.txt
initramfs initramfs-linux.img followkernel

dtparam=i2c=on
dtparam=uart0=on
dtparam=pciex1_gen=3
dtparam=ant2

dtoverlay=dwc2,dr_mode=host
dtoverlay=vc4-kms-v3d

usb_max_current_enable=1
disable_fw_kms_setup=1
disable_overscan=1
max_framebuffers=2
disable_splash=1
enable_uart=0
arm_boost=1
EOF

# cleanup of downloaded package file cache
find sysroot/var/cache/pacman/pkg -type f -delete

# done with sysroot mounts
umount sysroot/{boot,dev,sys,proc,run}

# put sysroot in tar archive
rm -f bootstrap-16k/sysroot.tar
tar --acls --xattrs -C sysroot -cpf bootstrap-16k/sysroot.tar .

# create initrd archive
cat > bootstrap-16k/init << EOF
#!/busybox sh
/busybox mkdir /proc
/busybox mount -t devtmpfs devtmpfs /dev
/busybox mount -t proc     proc     /proc
/busybox insmod /lz4_compress.ko.xz
/busybox insmod /lz4hc_compress.ko.xz
/busybox insmod /f2fs.ko.xz
/busybox insmod /virtio_blk.ko.xz
/busybox mkdir /sysroot
/busybox mount /dev/vda /sysroot
/busybox tar -C /sysroot -xpf /sysroot.tar
/busybox umount /sysroot
/busybox poweroff -f
EOF
chmod +x bootstrap-16k/init

cd bootstrap-16k
find . -not -name "*.cpio" | cpio -o -F initrd.cpio --format=newc
cd ..

# boot 16k page arm64 kernel to write sysroot to f2fs root partition
qemu-system-aarch64 -nographic -no-reboot -machine virt -cpu max -m 6G -kernel bootstrap-16k/vmlinuz -initrd bootstrap-16k/initrd.cpio -drive file=/dev/mapper/${LUKS_NAME},format=raw,if=virtio -append "panic=-1 quiet"

# done!

rm -rf sysroot
echo Done!
