FROM oraclelinux:8
WORKDIR /etc/yum.repos.d/

RUN yum update -y
RUN yum install -y oraclelinux-release-el8 dnf-plugins-core && \
    yum-config-manager --enable ol8_baseos_latest ol8_appstream ol8_UEKR6 ol8_codeready_builder && \
    yum clean all
RUN yum install -y dnf-plugins-core \
    && yum install -y cmake gcc gcc-c++ \
    && yum install -y oracle-epel-release-el8 \
    && yum install -y SDL2-devel mesa-libGL mesa-libGL-devel mesa-dri-drivers \
    && yum install -y libxcb-devel xcb-util-image xcb-util-keysyms \
    && yum install -y openssl-devel \
    && yum install -y libcurl-devel \
    && yum clean all

RUN yum install -y glib2-devel

RUN yum install -y pulseaudio pulseaudio-utils

RUN yum install -y libatomic
RUN dnf install -y https://mirrors.rpmfusion.org/free/el/rpmfusion-free-release-8.noarch.rpm \
    && dnf install -y ffmpeg ffmpeg-devel \
    && dnf clean all
WORKDIR /app
COPY cmake/ /cmake/
COPY sdk/ /sdk/
COPY GetRawVideoAndAudioExample/demo/ /app/demo/

RUN mkdir -p /root/.config \
    && cp /app/demo/zoomus.conf /root/.config/zoomus.conf \
    && cmake -S /app/demo -B /app/demo/build \
    && cmake --build /app/demo/build --parallel 2

ENV LD_LIBRARY_PATH=/app/demo/bin:/app/demo/bin/qt_libs:/app/demo/bin/qt_libs/Qt/lib
WORKDIR /app/demo/bin
CMD ["/bin/bash", "-ec", "mkdir -p /root/.config/pulse; pulseaudio --check || pulseaudio --start --exit-idle-time=-1; pactl load-module module-null-sink sink_name=SpeakerOutput; pactl set-default-sink SpeakerOutput; pactl set-default-source SpeakerOutput.monitor; exec ./GetRawVideoAndAudioDemo"]
