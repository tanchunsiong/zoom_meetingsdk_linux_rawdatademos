FROM dorowu/ubuntu-desktop-lxde-vnc:focal

ENV DEBIAN_FRONTEND=noninteractive
RUN sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 4EB27DB2A3B88B8B
RUN apt-get update && \
    apt-get install -y build-essential cmake

RUN apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
    libx11-xcb1 \
    libxcb-xfixes0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-randr0 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-xtest0 \
    libdbus-1-3 \
    libglib2.0-dev \
    libgbm1 \
    libxfixes3 \
    libgl1 \
    libdrm2 \
    libgssapi-krb5-2 \
    openssl \
    ca-certificates \
    pkg-config \
    libegl-mesa0 \
    libsdl2-dev \
    g++-multilib
RUN apt-get install -y libcurl4-openssl-dev
RUN apt-get install -y libasound2 libasound2-plugins alsa alsa-utils alsa-oss
RUN apt-get install -y  pulseaudio pulseaudio-utils
RUN apt-get install -y libavformat-dev libavfilter-dev libavdevice-dev ffmpeg
WORKDIR /app
COPY cmake/ /cmake/
COPY sdk/ /sdk/
COPY SendRawShareandAudioExample/demo/ /app/demo/

RUN mkdir -p /root/.config \
    && cp /app/demo/zoomus.conf /root/.config/zoomus.conf \
    && cmake -S /app/demo -B /app/demo/build \
    && cmake --build /app/demo/build --parallel 2

ENV LD_LIBRARY_PATH=/app/demo/bin:/app/demo/bin/qt_libs:/app/demo/bin/qt_libs/Qt/lib
WORKDIR /app/demo/bin
CMD ["/bin/bash", "-ec", "mkdir -p /root/.config/pulse; pulseaudio --check || pulseaudio --start --exit-idle-time=-1; pactl load-module module-null-sink sink_name=SpeakerOutput; pactl set-default-sink SpeakerOutput; pactl set-default-source SpeakerOutput.monitor; exec ./SendRawShareandAudioDemo"]
