#!/usr/bin/env php
<?php
// Intended behaviour of infinite.py: spin forever (the original is actually a
// SyntaxError: missing colon after while(1), so Python exits with status 1).
$b = null;
while (1) {
    $a = $b;
}
