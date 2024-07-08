#run these in /demo directory

# build Centos 8
docker build -t msdk-6.1.0-on-centos8-compact -f Dockerfile-Centos8/Dockerfile .
docker run -it --rm msdk-6.1.0-on-centos8-compact

# build Centos 9 (doesnt work well, dynamic fetching of token needs openssl 1.1.1 centos 9 is on 3.0.7)
docker build -t msdk-6.1.0-on-centos9-compact -f Dockerfile-Centos9/Dockerfile .
docker run -it --rm msdk-6.1.0-on-centos9-compact

# build Ubuntu
docker build -t msdk-6.1.0-on-ubuntu-compact -f Dockerfile-Ubuntu/Dockerfile .
docker run -it --rm msdk-6.1.0-on-ubuntu-compact
docker run --cpus=2.0 --memory=4G -it --rm msdk-6.1.0-on-ubuntu-compact

# build UbuntuDesktop
docker build -t msdk-6.1.0-on-ubuntudesktop-compact -f Dockerfile-UbuntuDesktop/Dockerfile .
docker run -it --rm msdk-6.1.0-on-ubuntudesktop-compact


# list all images
docker images -a
docker ps -a

# remove images
docker rmi 


