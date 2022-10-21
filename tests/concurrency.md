concurrency tests whether your proxy can handle multiple threads and assign threads to process the cleints' requests 
step1: setup tiny in block mode with command this mode requests will be accetped by server side but not return directly just loop and sleep 
```shell 
./tiny 18080 block 
```

step2: setup your proxy(not enable cache is recommended) in another terminal 
```shell 
./proxy 18999
```

step3: send request to server multiple times  in new terminal 
```shell 
curl --max-time 5 --silent --proxy http://localhost:18999 http://localhost:18080/home.html
curl --max-time 5 --silent --proxy http://localhost:18999 http://localhost:18080/home.html
curl --max-time 5 --silent --proxy http://localhost:18999 http://localhost:18080/home.html
curl --max-time 5 --silent --proxy http://localhost:18999 http://localhost:18080/home.html
curl --max-time 5 --silent --proxy http://localhost:18999 http://localhost:18080/home.html
```

result: you will see from the proxy side threads from the thread pool print logs like 
```txt
Pthread_create with index 0
Pthread_create thread tid 140168781829696
Pthread_create with index 1
Pthread_create thread tid 140168773436992
Pthread_create with index 2
Pthread_create thread tid 140168765044288


proxy#runnable thread id 140168781829696 receive connect fd 4 from client
proxy#runnable thread id 140168773436992 receive connect fd 5 from client
proxy#runnable thread id 140168765044288 receive connect fd 7 from client
```

Because we only create 3 threads in thread pool so contine with curl command will not get any log info. 
