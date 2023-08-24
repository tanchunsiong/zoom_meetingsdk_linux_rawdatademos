#run these in /release_demo director

#build Centos 8

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile8 .
docker run -it --rm msdk-5.15.10-on-centos

#build Centos 9 (doesnt work, dynamic fetching of token needs openssl 1.1.1 centos 9 is on 3.0.7)

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile9 .
docker run -it --rm msdk-5.15.10-on-centos

#build Ubuntu

docker build -t msdk-5.15.10-on-ubuntu -f Dockerfile-Ubuntu/Dockerfile .
docker run -it --rm msdk-5.15.10-on-ubuntu



#list all images
docker images -a

#remove images
docker rmi 
