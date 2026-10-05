#!/usr/bin/env php
<?php
parse_str(getenv('QUERY_STRING') ?: '', $qs);

// Like Python's parse_qs: blank/missing values fall back to the default
function param($qs, $key, $default = "0") {
    return (isset($qs[$key]) && is_string($qs[$key]) && $qs[$key] !== '') ? $qs[$key] : $default;
}

// Python-style float repr: 7.0 -> "7.0", 0.1+0.2 -> "0.30000000000000004"
function pyfloat($x) {
    return var_export((float)$x, true);
}

$a = param($qs, 'a');
$b = param($qs, 'b');

if (is_numeric($a) && is_numeric($b)) {
    $a = (float)$a;
    $b = (float)$b;
    $result = $a + $b;
    $body = "<html><body><h1>" . pyfloat($a) . " + " . pyfloat($b) . " = " . pyfloat($result) . "</h1></body></html>";
} else {
    $body = "<html><body><h1>Invalid numbers</h1></body></html>";
}

echo "Content-Type: text/html\n";
echo "Content-Length: " . strlen($body) . "\n";
echo "\n";
echo $body;
