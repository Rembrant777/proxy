# proxy
c language implements proxy that supports LRU and LFU cache policy 


## how to setup the compiling environment?
1> first you need to install docker on your desktop
<br/>
2> login remote docker hub and execute following command on you desktop to retrieve the latest ubuntu linux image on your docker image 
```
docker pull ubuntu:latest 
```
3> setup the ubuntu image to container with the commands given below and the (aimer20221018 is my own customized tag)
```
docker run -itd  --name myubuntuaimer20221018  -v /Users/apple/YR/homework/my-proxy/proxy:/opt ubuntu:aimer20221018
```
4> in the above folder we set mapping from our own host to docker namespace so that you can compile your c codes with ubuntu's inner installed gcc/g++ compiler 

5> ok here comes to how to compile the project before compile your c codes into binary executable file you need to install 
* 5.1 gcc on your docker container of ubuntu by apt-get install gcc 
* 5.2 apt-get install make 
* 5.3 then you execute the compile command ./build.sh to compile the c codes under proxy and go to the tiny foler also execute the ./build.sh script to compile the tiny server c codes 
* 5.4 then run the test cases to test proxy normal function, proxy works in parallel, proxy provides lru and lfu policies  
