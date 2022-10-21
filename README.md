# What's this proxy ?
* this is a toy objected proxy implementation based on c language 
* supports LRU and LFU cache policy that cache requested results to proxy local areas and organized web-objects in key,value pairs
* supports multi-thread process && handle different client's connection requests

# How to compile this project ?
* when you in your mac labtop download gcc and compile this project can found out there are lots of linux internal errors 
* so, we use docker ubuntu images to setup the development environment, but, you have to install your gcc, make compile tools by yourself with commands shown as below 
```shell 
apt-get install gcc
apt-get install make 
```
* when you pull your docker ubuntu image from remote repo(docker.hub) successfully, then you execute the script [ubuntu-proxy.sh](https://github.com/nanachi1027/proxy/blob/main/ubuntu-proxy.sh) to make a mapping from your development directory to ubuntu os's `/opt` directory.

* when your successfully setup the docker image use command below to login the docker env go `/opt`
```
docker exec -it ${container-id} bash 
```

* execute the shell `build.sh` this script will automatically compile both the proxy and tiny into binary executable files 
```
sh build.sh 

part of the output info: 
cache.c:46:35: note: expected 'LFUCache *' but argument is of type 'LRUCache *'
   46 | static void freeLFUList(LFUCache *cache) {
      |                         ~~~~~~~~~~^~~~~
gcc -g -Wall -c csapp.c
gcc -g -Wall proxy.o cache.o csapp.o sbuf.o -o proxy -lpthread
```

* then you got a executable proxy refer to the markdown docs under directory `tests/*.md` you can test this proxy's cache, concurrency and basic proxy features

# contribution && commit codes 
* any modification can be taken into consideration have fun~ 
