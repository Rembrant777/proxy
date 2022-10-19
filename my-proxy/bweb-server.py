#!/usr/bin/python
## this is a no response web server implementation(block-server).
## we create this web server with the aim to not response to expand all the client's thread's lifetime
# so that all the proxy's thread will not release the connection to verify multiple threads will be
# generated on the proxy side to handle multiple requests

# this python server's setup command is python bweb-server.py ${server_port}
import socket
import sys

serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.bind(('', int(sys.argv[1])))
serversocket.listen(5)

while 1:
    channel, details = serversocket.accept()
    while 1:
        continue
