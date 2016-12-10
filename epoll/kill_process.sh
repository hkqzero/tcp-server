#!/bin/sh
netstat -pan|grep 9999 | awk '{print $7}' | awk -F"/" '{print $1}' | xargs kill -9
