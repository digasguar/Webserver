#!/usr/bin/env php
<?php
file_put_contents("/tmp/webserv_slowlog.txt", "run\n", FILE_APPEND);
sleep(1);
echo "Content-Type: text/plain\n\n";
echo "done\n";
