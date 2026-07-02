# Docker notes

Run these commands from an individual sample's `demo/` directory.

The image tags below are intentionally version-agnostic. Rename them if you want a
more specific local tag.

## CentOS 8

```bash
docker build -t msdk-demo-centos8 -f ../Dockerfile-Centos8/Dockerfile .
docker run -it --rm msdk-demo-centos8
```

## CentOS 9

```bash
docker build -t msdk-demo-centos9 -f ../Dockerfile-Centos9/Dockerfile .
docker run -it --rm msdk-demo-centos9
```

CentOS 9 may still require extra OpenSSL compatibility work depending on the demo and
runtime path you use for token-fetching code.

## Ubuntu

```bash
docker build -t msdk-demo-ubuntu -f ../Dockerfile-Ubuntu/Dockerfile .
docker run -it --rm msdk-demo-ubuntu
docker run --cpus=2.0 --memory=4G -it --rm msdk-demo-ubuntu
```

## Ubuntu Desktop

```bash
docker build -t msdk-demo-ubuntu-desktop -f ../Dockerfile-UbuntuDesktop/Dockerfile .
docker run -it --rm msdk-demo-ubuntu-desktop
```

## Oracle Linux 8

```bash
docker build -t msdk-demo-oraclelinux8 -f ../Dockerfile-OracleLinux8/Dockerfile .
docker run -it --rm msdk-demo-oraclelinux8
```

## Useful commands

```bash
docker images -a
docker ps -a
docker rmi <image-id>
```
