#!/bin/bash

apt update -y
apt upgrade -y

apt install -y linux-image-extra-virtual
echo "snd_aloop" >> /etc/modules
modprobe snd-aloop

DISTRO=focal
JITSI_HOST=jitsi

# -----------------------------------------------------------------------------
# extra for jibri-jibri-installer
# -----------------------------------------------------------------------------
JIBRI_TMPL=https://raw.githubusercontent.com/emrahcom/emrah-buster-templates/master/machines/eb-jibri-template
PROSODY=/etc/prosody/conf.avail/$JITSI_HOST.cfg.lua
JICOFO_SIP=/etc/jitsi/jicofo/sip-communicator.properties
JITSI_MEET_CONFIG=/etc/jitsi/meet/$JITSI_HOST-config.js


JITSI_MEET_INTERFACE=/usr/share/jitsi-meet/interface_config.js


# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
out <<< "installing jibri packages..."

if [[ "$DISTRO" = "buster" ]]; then
    sed -i "s~ buster main$~ buster main contrib non-free~" \
        /etc/apt/sources.list
    sed -i "s~ buster-updates main$~ buster-updates main contrib non-free~" \
        /etc/apt/sources.list
    sed -i "s~ buster/updates main$~ buster/updates main contrib non-free~" \
        /etc/apt/sources.list
else
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys \
        AA8E81B4331F7F50 112695A0E562B32A 04EE7237B7D453EC 648ACFD622F3D138 \
        DCC9EFBF77E11517

    cat >/etc/apt/sources.list.d/buster.list <<EOF
deb http://deb.debian.org/debian buster main non-free contrib
deb http://security.debian.org/debian-security buster/updates main contrib non-free
EOF
fi

apt-get -y --allow-releaseinfo-change update
apt-get $APT_PROXY_OPTION -y install kmod alsa-utils
apt-get $APT_PROXY_OPTION -y install libnss3-tools
apt-get $APT_PROXY_OPTION -y install va-driver-all vdpau-driver-all
apt-get $APT_PROXY_OPTION -y --install-recommends install ffmpeg
[[ "$DISTRO" = "buster" ]] && \
    apt-get $APT_PROXY_OPTION -y --install-recommends install \
        nvidia-openjdk-8-jre || \
    apt-get $APT_PROXY_OPTION -y --install-recommends install \
        openjdk-8-jdk-headless
apt-get $APT_PROXY_OPTION -y --install-recommends install \
        chromium chromium-driver
apt-get $APT_PROXY_OPTION -y install stunnel x11vnc
apt-get $APT_PROXY_OPTION -y install jibri

apt-get -y purge upower
apt-get -y --purge autoremove

# jibri packages are ok
timeout 8 curl -s "$CHECKMYPORT?proto=udp&port=60000&text=ok-package-jibri" \
    >/dev/null || true


# -----------------------------------------------------------------------------
# configuration
# -----------------------------------------------------------------------------
out <<< "installing jibri configuration..."

# snd_aloop
[[ -z "$(egrep '^snd_aloop' /etc/modules)" ]] && echo snd_aloop >>/etc/modules

# hosts
sed -i "/127.0.2.1/d" /etc/hosts
sed -i "/127.0.0.1/a 127.0.2.1 $JITSI_HOST" /etc/hosts

# chromium
mkdir -p /etc/chromium/policies/managed
wget -O /etc/chromium/policies/managed/eb_policies.json \
    $JIBRI_TMPL/etc/chromium/policies/managed/eb_policies.json

# stunnel
wget -O /etc/stunnel/facebook.conf $JIBRI_TMPL/etc/stunnel/facebook.conf
systemctl restart stunnel4.service

# prosody
cat >> $PROSODY <<EOF
VirtualHost "recorder.$JITSI_HOST"
    modules_enabled = {
        "ping";
    }
    authentication = "internal_plain"
EOF
systemctl restart prosody.service

# prosody register
PASSWD1=$(openssl rand -hex 20)
PASSWD2=$(openssl rand -hex 20)

prosodyctl unregister jibri auth.$JITSI_HOST || true
prosodyctl register jibri auth.$JITSI_HOST $PASSWD1
prosodyctl unregister recorder recorder.$JITSI_HOST || true
prosodyctl register recorder recorder.$JITSI_HOST $PASSWD2

# jicofo
cat >> $JICOFO_SIP <<EOF
org.jitsi.jicofo.jibri.BREWERY=JibriBrewery@internal.auth.$JITSI_HOST
org.jitsi.jicofo.jibri.PENDING_TIMEOUT=90
EOF
systemctl restart jicofo.service

# jitsi-meet config
sed -i 's~//\s*fileRecordingsEnabled.*~fileRecordingsEnabled: true,~' \
    $JITSI_MEET_CONFIG
sed -i 's~//\s*fileRecordingsServiceSharingEnabled.*~fileRecordingsServiceSharingEnabled: true,~' \
    $JITSI_MEET_CONFIG
sed -i 's~//\s*liveStreamingEnabled:.*~liveStreamingEnabled: true,~' \
    $JITSI_MEET_CONFIG
sed -i "/liveStreamingEnabled:/a \\\n    hiddenDomain: 'recorder.$JITSI_HOST'," \
    $JITSI_MEET_CONFIG

# icewm
mkdir -p /home/jibri/.icewm
cat > /home/jibri/.icewm/startup <<EOF
#!/bin/bash

chromium https://$JITSI_HOST &
(sleep 20 && pkill --oldest --signal TERM chromium) &

#x11vnc -display :0 -nolookup -once -loop -usepw -shared -noxdamage -nodpms &
EOF
chmod 755 /home/jibri/.icewm/startup

# jibri
usermod -aG adm,audio,video,plugdev jibri
mkdir -p /usr/local/eb/recordings
chown jibri:jibri /usr/local/eb/recordings -R

wget -O /etc/jitsi/jibri/jibri.conf $JIBRI_TMPL/etc/jitsi/jibri/jibri.conf
sed -i "s/___JITSI_HOST___/$JITSI_HOST/" /etc/jitsi/jibri/jibri.conf
sed -i "s/___PASSWD1___/$PASSWD1/" /etc/jitsi/jibri/jibri.conf
sed -i "s/___PASSWD2___/$PASSWD2/" /etc/jitsi/jibri/jibri.conf
sed -i "s/\"--kiosk\",/\"--kiosk\",\n\t    \"--ignore-certificate-errors\",/" /etc/jitsi/jibri/jibri.conf

wget -O /usr/local/bin/finalize_recording.sh \
    $JIBRI_TMPL/usr/local/bin/finalize_recording.sh
chmod 755 /usr/local/bin/finalize_recording.sh

wget -O /usr/local/bin/ffmpeg $JIBRI_TMPL/usr/local/bin/ffmpeg
chmod 755 /usr/local/bin/ffmpeg

mkdir -p /etc/systemd/system/jibri.service.d
wget -O /etc/systemd/system/jibri.service.d/override.conf \
    $JIBRI_TMPL/etc/systemd/system/jibri.service.d/override.$DISTRO.conf

systemctl daemon-reload
systemctl enable jibri.service
systemctl start jibri.service

# jibri vnc
mkdir -p /home/jibri/.vnc
x11vnc -storepasswd jibri /home/jibri/.vnc/passwd
chown jibri:jibri /home/jibri/.vnc -R

# jibri configuration is ok
timeout 8 curl -s "$CHECKMYPORT?proto=udp&port=60000&text=ok-conf-jibri" \
    >/dev/null || true


# -----------------------------------------------------------------------------
# completed
# -----------------------------------------------------------------------------
# jibri completed
timeout 8 curl -s "$CHECKMYPORT?proto=udp&port=60000&text=ok-jibri" \
    >/dev/null || true
