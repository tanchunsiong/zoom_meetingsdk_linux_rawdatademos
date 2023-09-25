# Use the CentOS 9 base image
FROM quay.io/centos/centos:stream9

# Install required packages
RUN yum install -y dnf-plugins-core \
    && dnf install -y cmake gcc gcc-c++ \
    && dnf config-manager --set-enabled crb \
    && dnf install -y epel-release epel-next-release \
    && yum install -y gtk3-devel gtkmm30 gtkmm30-devel mesa-libGL mesa-libGL-devel mesa-dri-drivers \
    && yum install -y libxcb-devel xcb-util-image xcb-util-keysyms \
    && yum install -y openssl-devel \
    && yum install -y libcurl-devel \
    && yum clean all

 # install pulse audio related
 #RUN yum install -y alsa-utils

 #RUN yum install -y libasound2 libasound2-plugins alsa alsa-utils alsa-oss

 RUN yum install -y  pulseaudio pulseaudio-utils

# install ffmpeg related
RUN yum install -y ffmpeg


# Set the working directory
WORKDIR /app

# Copy your application files to the container
#COPY demo/bin /app/demo/bin
COPY demo/ /app/demo/

# Execute additional commands
RUN cd /app/demo && rm -rf bin && rm -rf build && cmake -B build && cd build && make

# Set the working directory to the binary folder
WORKDIR /app/demo/bin

# Run the meetingSDKDemo binary when the container starts
#CMD ["/bin/bash"]
CMD ["./meetingSDKDemo"]
