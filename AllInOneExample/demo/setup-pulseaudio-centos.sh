#!/bin/bash


# enable dbus


#mkdir -p /var/run/dbus
#dbus-uuidgen > /var/lib/dbus/machine-id
#dbus-daemon --config-file=/usr/share/dbus-1/system.conf --print-address &





pulseaudio -D --exit-idle-time=-1  &


# add root

 usermod -aG pulse-access root
 usermod -aG audio root


# Create a virtual speaker output

pactl load-module module-null-sink sink_name=SpeakerOutput
pactl set-default-sink SpeakerOutput
pactl set-default-source SpeakerOutput.monitor

mkdir ~/.config

echo -e "[General]\nsystem.audio.type=default" > ~/.config/zoomus.conf

