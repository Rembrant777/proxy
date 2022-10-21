Basic Test Case -- test your proxy can work correctly

step1: setup your tiny in normal mode 
```shell 
./tiny 18080 
```
step2: setup your proxy(not enable cache) 
```shell 
./proxy 18999
```

step3: execute commands shown below you will get return value on client side 
```shell
get server side file directly from server 
curl --max-time 5 --silent  --output home.html http://localhost:18080/home.html

get server side file via proxy 
curl --max-time 5 --silent --proxy http://localhost:18999 --output home.html http://localhost:18080/home.html 
```
