//build

docker build -t msdk-5.15.10-on-centos -f Dockerfile-Centos/Dockerfile .
docker run -it --rm msdk-5.15.10-on-centos

docker build -t msdk-5.15.10-on-ubuntu -f Dockerfile-Ubuntu/Dockerfile .
docker run -it --rm msdk-5.15.10-on-ubuntu

#list all images
docker images -a

#remove images
docker rmi 
