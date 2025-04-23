#!/bin/bash

set -eux

# settings

SETUP_DEV=mmcblk0     # !!! BE VERY CAREFUL WHICH DEVICE TO USE HERE !!!

SETUP_MIRROR='https://ca.us.mirror.archlinuxarm.org' # https://archlinuxarm.org/about/mirrors
SETUP_TIMEZONE='America/Los_Angeles'
SETUP_HOSTNAME=piamp

SETUP_WIFI_SSID=YOUR_WIFI_NAME
SETUP_WIFI_PASS=YOUR_WIFI_PASSWORD

SETUP_USER=piamp
SETUP_PASS=piamp

SETUP_PLEXAMP_VER=4.12.1
SETUP_PLEXAMP_NODE=nodejs-lts-iron # whatever version is required for headless Plexamp

# must run as root

if [ "${EUID}" -ne 0 ]; then
  echo "Must run as root!"
  exit 1
fi

# make sure needed packages are installed

pacman -Q curl                    > /dev/null
pacman -Q libarchive              > /dev/null
pacman -Q dosfstools              > /dev/null
pacman -Q f2fs-tools              > /dev/null
pacman -Q qemu-user-static        > /dev/null
pacman -Q qemu-user-static-binfmt > /dev/null

# download Pi Codec Zero playback alsa state

curl -sfLO https://github.com/raspberrypi/Pi-Codec/raw/refs/heads/master/Codec_Zero_Playback_only.state

# download headless Plexamp

if [ ! -f Plexamp-Linux-headless-v${SETUP_PLEXAMP_VER}.tar.bz2 ]; then
  curl -fLO https://plexamp.plex.tv/headless/Plexamp-Linux-headless-v${SETUP_PLEXAMP_VER}.tar.bz2
fi

# partition sdcard

blkdiscard -f /dev/${SETUP_DEV} || true
sfdisk --delete /dev/${SETUP_DEV} || true
echo -e ',200M,c\n,+,\n' | sfdisk /dev/${SETUP_DEV}

# format partitions - boot with FAT32, root with F2FS

mkfs.vfat /dev/${SETUP_DEV}p1
mkfs.f2fs /dev/${SETUP_DEV}p2

# mount partitions

MOUNT_DIR=$(mktemp -d -p .)

mount         /dev/${SETUP_DEV}p2 ${MOUNT_DIR}
mount --mkdir /dev/${SETUP_DEV}p1 ${MOUNT_DIR}/boot

#chmod 0755 ${MOUNT_DIR}
#mount --bind ${MOUNT_DIR} ${MOUNT_DIR}

mkdir -m 0755 -p ${MOUNT_DIR}/etc/pacman.d/hooks
mkdir -m 0755 -p ${MOUNT_DIR}/var/{cache/pacman/pkg,lib/pacman,log}

# chroot mounts

touch ${MOUNT_DIR}/etc/resolv.conf

mount --mkdir --bind /dev  ${MOUNT_DIR}/dev
mount --mkdir --bind /sys  ${MOUNT_DIR}/sys
mount --mkdir --bind /proc ${MOUNT_DIR}/proc
mount --bind -o ro $(readlink -m /etc/resolv.conf) ${MOUNT_DIR}/etc/resolv.conf

function mount_cleanup {
  find ${MOUNT_DIR}/etc/pacman.d/gnupg -type s -delete || true # removing sockets exits gpg-agent daemon
  umount -R ${MOUNT_DIR}
  rm -d ${MOUNT_DIR}
}

trap mount_cleanup EXIT

# disable initramfs generation, Raspberry Pi can boot kernel from kernel8.img file directly
# filename must match file here: https://gitlab.archlinux.org/archlinux/mkinitcpio/mkinitcpio/-/tree/master/libalpm/hooks
# this is here only because "linux-rpi" package depends on "mkinitcpio"

ln -sf /dev/null ${MOUNT_DIR}/etc/pacman.d/hooks/90-mkinitcpio-install.hook

# initialize pacman keyring

curl -sfLO 'https://github.com/archlinuxarm/PKGBUILDs/raw/refs/heads/master/core/archlinuxarm-keyring/archlinuxarm{.gpg,-trusted,-revoked}'

pacman-key --gpgdir ${MOUNT_DIR}/etc/pacman.d/gnupg --init
pacman-key --gpgdir ${MOUNT_DIR}/etc/pacman.d/gnupg --populate --populate-from .

rm archlinuxarm{.gpg,-trusted,-revoked}

# install packages

cat > ${MOUNT_DIR}/pacman-bootstrap.conf << EOF
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
  --sysroot ${MOUNT_DIR}                   \
  --config /pacman-bootstrap.conf          \
  archlinuxarm-keyring                     \
  base                                     \
  linux-rpi                                \
  bash-completion                          \
  sudo                                     \
  less                                     \
  f2fs-tools                               \
  alsa-utils                               \
  iwd                                      \
  openssh                                  \
  vim                                      \
  tmux                                     \
  htop                                     \
  foot-terminfo                            \
  ${SETUP_PLEXAMP_NODE}

rm ${MOUNT_DIR}/etc/resolv.conf.pacnew
rm ${MOUNT_DIR}/pacman-bootstrap.conf

# set pacman mirror

echo "Server = ${SETUP_MIRROR}/\$arch/\$repo" > ${MOUNT_DIR}/etc/pacman.d/mirrorlist 

# set hostname

echo "${SETUP_HOSTNAME}" > ${MOUNT_DIR}/etc/hostname

# set UTF-8 locale

echo 'LANG=C.UTF-8'  > ${MOUNT_DIR}/etc/locale.conf
echo 'C.UTF-8 UTF-8' > ${MOUNT_DIR}/etc/locale.gen
chroot ${MOUNT_DIR} locale-gen

# set time zone

ln -sf /usr/share/zoneinfo/${SETUP_TIMEZONE} ${MOUNT_DIR}/etc/localtime

# use systemd-resolved for DNS resolving

umount ${MOUNT_DIR}/etc/resolv.conf
ln -sf /run/systemd/resolve/stub-resolv.conf ${MOUNT_DIR}/etc/resolv.conf

# keep journal log data only in memory

mkdir -m 0755 ${MOUNT_DIR}/etc/systemd/journald.conf.d
cat > ${MOUNT_DIR}/etc/systemd/journald.conf.d/storage.conf << EOF
[Journal]
Storage=volatile
EOF

# disable DNS fallback servers

mkdir -m 0755 ${MOUNT_DIR}/etc/systemd/resolved.conf.d
cat > ${MOUNT_DIR}/etc/systemd/resolved.conf.d/nofallbackdns.conf << EOF
[Resolve]
FallbackDNS=
EOF

# use vim as default editor

echo 'export EDITOR=vim' > ${MOUNT_DIR}/etc/profile.d/editor.sh

# allow sudo for wheel group

echo '%wheel ALL=(ALL) ALL' > ${MOUNT_DIR}/etc/sudoers.d/wheel

# wifi settings, with DHCP and mDNS enabled

mkdir -m 0700 ${MOUNT_DIR}/var/lib/iwd

cat > ${MOUNT_DIR}/var/lib/iwd/${SETUP_WIFI_SSID}.psk << EOF
[Security]
Passphrase=${SETUP_WIFI_PASS}
EOF

chmod 0600 ${MOUNT_DIR}/var/lib/iwd/${SETUP_WIFI_SSID}.psk

cat > ${MOUNT_DIR}/etc/systemd/network/wlan0.network << EOF
[Match]
Name=wlan0

[Network]
DHCP=yes
MulticastDNS=yes

[Link]
Multicast=yes
EOF

# enable/disable startup services

chroot ${MOUNT_DIR} systemctl enable iwd
chroot ${MOUNT_DIR} systemctl enable sshd
chroot ${MOUNT_DIR} systemctl enable systemd-networkd
chroot ${MOUNT_DIR} systemctl enable systemd-resolved
chroot ${MOUNT_DIR} systemctl enable systemd-timesyncd

chroot ${MOUNT_DIR} systemctl disable getty@tty1
chroot ${MOUNT_DIR} systemctl disable systemd-homed
chroot ${MOUNT_DIR} systemctl disable systemd-userdbd.socket
chroot ${MOUNT_DIR} systemctl disable systemd-nsresourced.socket

# alsa state used by alsa-restore.service on startup

mv Codec_Zero_Playback_only.state ${MOUNT_DIR}/var/lib/alsa/asound.state

# add non-root user

chroot ${MOUNT_DIR} useradd -m -g users -G wheel,audio,video,input,disk,input,network -s /bin/bash -p $(openssl passwd -6 "${SETUP_PASS}") ${SETUP_USER}

# disable root account

chroot ${MOUNT_DIR} passwd -l root

# kernel boot commandline, with workaround for wifi on Pi Zero 2 W
# https://bugs.launchpad.net/raspbian/+bug/1929746

# console=serial0,115200
echo 'root=/dev/mmcblk0p2 rw rootwait audit=0 fsck.repair=yes brcmfmac.feature_disable=0x82000' > ${MOUNT_DIR}/boot/cmdline.txt

# Raspberry Pi config - enable Codec Zero DAC, disable BT & hdmi, reduce GPU memory usage, disable serial UART

cat > ${MOUNT_DIR}/boot/config.txt << EOF
dtoverlay=
dtoverlay=rpi-codeczero
dtoverlay=disable-bt
#dtoverlay=vc4-kms-v3d,nohdmi
camera_auto_detect=0
display_auto_detect=0
max_framebuffers=0
enable_uart=0
arm_boost=1
gpu_mem=16
EOF

# unpack Plexamp and add service

bsdtar xf Plexamp-Linux-headless-v${SETUP_PLEXAMP_VER}.tar.bz2 -C ${MOUNT_DIR}/opt

cat > ${MOUNT_DIR}/etc/systemd/system/plexamp.service << EOF
[Unit]
Description=Plexamp
After=network-online.target sound.target
Requires=network-online.target sound.target

[Service]
Type=simple
User=piamp
WorkingDirectory=/opt/plexamp
ExecStart=/usr/bin/node /opt/plexamp/js/index.js
Restart=on-failure
RestartSec=1

[Install]
WantedBy=multi-user.target
EOF

# cleanup downloaded package files

find ${MOUNT_DIR}/var/cache/pacman/pkg -type f -delete

# done!
