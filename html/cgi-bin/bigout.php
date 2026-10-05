#!/usr/bin/env php
<?php
fwrite(STDOUT, "Content-Type: application/octet-stream\n\n");
fflush(STDOUT);

$chunk = '';
for ($i = 0; $i < 256; $i++) {
    $chunk .= chr(($i * 7 + 3) & 0xff);
}
fwrite(STDOUT, str_repeat($chunk, 5 * 1024 * 4));   // 5 MiB
fflush(STDOUT);
