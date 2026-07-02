#!/bin/bash


# enable dbus
mkdir -p /var/run/dbus /var/lib/dbus
dbus-uuidgen --ensure=/var/lib/dbus/machine-id
dbus-daemon --config-file=/usr/share/dbus-1/system.conf --print-address --fork || true

# add root
adduser root pulse-access || true
adduser root audio || true


# Cleanup to be "stateless" on startup, otherwise pulseaudio daemon can't start

rm -rf /var/run/pulse /var/lib/pulse /root/.config/pulse
mkdir -p /root/.config/pulse
cp -r /etc/pulse/* /root/.config/pulse/


pulseaudio --check || pulseaudio -D --exit-idle-time=-1


# Create a virtual speaker output

pactl load-module module-null-sink sink_name=SpeakerOutput || true



pactl set-default-sink SpeakerOutput

pactl set-default-source SpeakerOutput.monitor


mkdir -p ~/.config

echo -e "[General]\nsystem.audio.type=default" > ~/.config/zoomus.conf
