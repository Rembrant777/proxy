Cache Test Case 
Cache Test needs to cache visited web object and cache the data organizaed in key & value pairs. 
In this proxy implement we set the key as the request's path name that is request_t#path
And the value the file content, we set the length VALUE_SIZE large enough so we don't support cache append this operation.

step1: setup tiny as server side in normal mode 
```shell 
./tiny 18080
```

step2: setup proxy with lru cache policy 
```
./proxy 18999 lru
```

step3: send proxy curl request multiple times 
>> in the first request we can see that the data is retrieved from server side to client side write in client side path file home.html; we add print cache inner method to show data is added to the cache.
>> in the second request we can see that data request's path can be found inside the cache and data is loaded to the proxy's local buffer and send to the client via the client's fd(file descriptor) through the log 

```shell 
sh get-proxy.sh 
```

expected log info shown below:

```txt
get key content home.html
#get get data from lru cache(len=2)
#getFromLRUCache by given key home.html len 2
#getValueFromLRUHashMap recv key home.html
#getValueFromLRUHashMap got item home.html
#getValueFromLRUHashMap got item key home.html and query key home.html
#getValueFromLRUHashMap got item home.html
#forward_request cache key home.html already exists in cache get from cache directly
#get key content home.html
#get get data from lru cache(len=2)
#getFromLRUCache by given key home.html len 2
#getValueFromLRUHashMap recv key home.html
#getValueFromLRUHashMap got item home.html
#getValueFromLRUHashMap got item key home.html and query key home.html
#getValueFromLRUHashMap got item home.html
#forward_request begin read via server fd -2
#forward_request match cache value read from proxy local cache to client side
#get key content home.html
#get get data from lru cache(len=2)
#getFromLRUCache by given key home.html len 2
#getValueFromLRUHashMap recv key home.html
#getValueFromLRUHashMap got item home.html
#getValueFromLRUHashMap got item key home.html and query key home.html
#getValueFromLRUHashMap got item home.html
#forward_request read from cache len 210
content
HTTP/1.0 200 OK
Server: Tiny Web Server
Content-length: 120
Content-type: text/html

<html>
<head><title>test</title></head>
<body>
<img align="middle" src="godzilla.gif">
Dave O'Hallaron
</body>
</html>

#forward_request free request entity here#runnable==> forward finish sleep for 10 seconds and close connection to client
```





