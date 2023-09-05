#run these in /release_demo directory

#build Centos 8

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile8-demo .
docker run -it --rm msdk-5.15.10-on-centos8

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile8-demo-compact .
docker run -it --rm msdk-5.15.10-on-centos8-compact

#build Centos 9 (doesnt work, dynamic fetching of token needs openssl 1.1.1 centos 9 is on 3.0.7)

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile9-demo .
docker run -it --rm msdk-5.15.10-on-centos9

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile9-demo-compact .
docker run -it --rm msdk-5.15.10-on-centos9-compact


#build Ubuntu

docker build -t msdk-5.15.10-on-ubuntu -f Dockerfile-Ubuntu/Dockerfile-demo .
docker run -it --rm msdk-5.15.10-on-ubuntu

docker build -t msdk-5.15.10-on-ubuntu -f Dockerfile-Ubuntu/Dockerfile-demo-compact .
docker run -it --rm msdk-5.15.10-on-ubuntu-compact



#list all images
docker images -a

#remove images
docker rmi 
