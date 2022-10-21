#!/bin/sh

 curl --max-time 5 --silent --proxy http://localhost:18999 --output home.html http://localhost:18080/home.html
