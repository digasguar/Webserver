#!/bin/bash
echo "run" >> /tmp/webserv_slowlog.txt
sleep 1
printf 'Content-Type: text/plain\n\n'
echo "done"
