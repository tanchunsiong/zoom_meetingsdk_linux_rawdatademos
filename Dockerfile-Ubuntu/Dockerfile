# Use the official Ubuntu image as the base image
FROM ubuntu:latest

# Install necessary dependencies
RUN apt-get update && \
    apt-get install -y build-essential cmake

RUN apt-get install -y pkgconf

RUN apt-get install -y gtkmm-3.0



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
    libglib2.0-0 \
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

# Set the working directory
WORKDIR /app

# Copy your application files to the container

COPY demo/ /app/demo/

# Execute additional commands
RUN cd /app/demo && rm -rf bin && rm -rf build && cmake -B build && cd build && make

# Set the working directory to the binary folder
WORKDIR /app/demo/bin

# Run the meetingSDKDemo binary when the container starts

#CMD ["/bin/bash"]
CMD ["./meetingSDKDemo"]
