# Docker Notes

The per-example Dockerfiles support Ubuntu, Ubuntu Desktop, CentOS Stream 9, and Oracle Linux 8. CentOS 8 is no longer supported.

Docker builds must use the repository root as the build context. The SDK remains local under `sdk/`; its contents are ignored by git and copied into the build image. The public repository contains only `sdk/.gitkeep`, so a real SDK installation is required for a successful build.

## Build

Run these commands from the repository root:

```bash
docker build \
  -t msdk-demo-ubuntu \
  -f AllInOneExample/Dockerfile-Ubuntu/Dockerfile .
```

Replace `AllInOneExample` and the Dockerfile directory with the example and platform you want to build. Before building, place the extracted SDK contents in `sdk/`. The required files may be directly under `sdk/` or inside one extracted SDK subdirectory. The SDK must contain `h/`, `qt_libs/`, `json/`, `images/`, `cpthost`, `libmeetingsdk.so`, `libcml.so`, and `libmpg123.so`.

## Run

The image expects a runtime configuration file. Mount the ignored local config rather than copying it into the image:

```bash
docker run --rm -it \
  -v "$PWD/AllInOneExample/demo/config.json:/app/demo/bin/config.json:ro" \
  msdk-demo-ubuntu
```

Ubuntu and Ubuntu Desktop images start the generated `run.sh` launcher, which initializes the container audio setup before starting the demo. Audio-capable containers may need additional host PulseAudio or device configuration.

## Examples

```bash
# Ubuntu
docker build -t msdk-demo-ubuntu \
  -f ChatExample/Dockerfile-Ubuntu/Dockerfile .

# Ubuntu Desktop
docker build -t msdk-demo-ubuntu-desktop \
  -f ChatExample/Dockerfile-UbuntuDesktop/Dockerfile .

# CentOS Stream 9
docker build -t msdk-demo-centos9 \
  -f ChatExample/Dockerfile-Centos9/Dockerfile .

# Oracle Linux 8
docker build -t msdk-demo-oraclelinux8 \
  -f ChatExample/Dockerfile-OracleLinux8/Dockerfile .
```

The RTMS example additionally requires its Boost, OpenSSL, FFmpeg, and WebSocket++ dependencies, which are included in its retained Docker variants.
