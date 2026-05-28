# syntax=docker/dockerfile:1

FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    libavcodec-dev \
    libavdevice-dev \
    libavfilter-dev \
    libavformat-dev \
    libavutil-dev \
    libcurl4-openssl-dev \
    libglib2.0-dev \
    libswresample-dev \
    libswscale-dev \
    pkg-config \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY cmake/ /src/cmake/
COPY demo/ /src/demo/
COPY .docker-sdk/ /src/sdk/

RUN cmake -S /src/demo -B /src/demo/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DZOOM_MEETING_SDK_DIR=/src/sdk \
  && cmake --build /src/demo/build --parallel "$(nproc)"

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    QT_QPA_PLATFORM=offscreen \
    JWT_ENDPOINT=https://nodejs.asdc.cc/meeting \
    JWT_ROLE=0 \
    USE_JWT_TOKEN_FROM_WEB_SERVICE=false \
    USE_RECORDING_TOKEN_FROM_WEB_SERVICE=false \
    SEND_VIDEO_RAW_DATA=true \
    SEND_AUDIO_RAW_DATA=true \
    CHAT_DEMO=true \
    EXIT_ON_MEETING_END=true

RUN apt-get update && apt-get install -y --no-install-recommends \
    adduser \
    alsa \
    alsa-oss \
    alsa-utils \
    ca-certificates \
    dbus \
    ffmpeg \
    libasound2 \
    libasound2-plugins \
    libcurl4 \
    libdbus-1-3 \
    libdrm2 \
    libegl-mesa0 \
    libgbm1 \
    libglib2.0-0 \
    libgl1 \
    libgssapi-krb5-2 \
    libx11-xcb1 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-xtest0 \
    libxcb-xfixes0 \
    libxfixes3 \
    openssl \
    pulseaudio \
    pulseaudio-utils \
    tini \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/zoom-loadtest

COPY --from=build /src/demo/bin/ /opt/zoom-loadtest/bin/
COPY media/ /opt/zoom-loadtest/media/
COPY demo/setup-pulseaudio.sh /opt/zoom-loadtest/setup-pulseaudio.sh
COPY docker-entrypoint.sh /opt/zoom-loadtest/docker-entrypoint.sh

RUN chmod +x /opt/zoom-loadtest/setup-pulseaudio.sh /opt/zoom-loadtest/docker-entrypoint.sh

WORKDIR /opt/zoom-loadtest/bin

ENTRYPOINT ["/usr/bin/tini", "--", "/opt/zoom-loadtest/docker-entrypoint.sh"]
