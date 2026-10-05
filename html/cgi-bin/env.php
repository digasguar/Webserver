#!/usr/bin/env php
<?php
echo "Content-Type: text/plain\n\n";
$env = getenv();
ksort($env);
foreach ($env as $k => $v) {
    echo "$k=$v\n";
}
echo "CWD=" . getcwd() . "\n";
