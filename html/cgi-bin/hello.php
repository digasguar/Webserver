#!/usr/bin/env php
<?php
$body = file_get_contents('php://stdin');
echo "Content-Type: text/plain\n";
echo "\n";
echo "REQUEST_METHOD=" . (getenv('REQUEST_METHOD') !== false ? getenv('REQUEST_METHOD') : "?") . "\n";
echo "QUERY_STRING=" . (getenv('QUERY_STRING') !== false ? getenv('QUERY_STRING') : "?") . "\n";
echo "BODY_RECEIVED=" . $body . "\n";
