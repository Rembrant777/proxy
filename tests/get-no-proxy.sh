#!/bin/sh
ls
curl --max-time 5 --silent  --output home.html http://localhost:18080/home.html
ls
