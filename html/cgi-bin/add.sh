#!/bin/bash

urldecode() {
    local s=${1//+/ }
    printf '%b' "${s//%/\\x}"
}

# First non-blank value for key $1 (URL-decoded), default "0" (like parse_qs)
param() {
    local pairs pair k v
    IFS='&' read -ra pairs <<< "$QUERY_STRING"
    for pair in "${pairs[@]}"; do
        k=${pair%%=*}
        v=""
        [[ $pair == *=* ]] && v=${pair#*=}
        if [[ $(urldecode "$k") == "$1" && -n $v ]]; then
            urldecode "$v"
            return
        fi
    done
    printf '0'
}

a=$(param a)
b=$(param b)

num_re='^[[:space:]]*[+-]?([0-9]+\.?[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?[[:space:]]*$'

if [[ $a =~ $num_re && $b =~ $num_re ]]; then
    a=${a//[[:space:]]/}
    b=${b//[[:space:]]/}
    # awk does the float math and prints Python-style (7.0, 0.30000000000000004)
    read -r fa fb fr < <(awk -v a="$a" -v b="$b" '
        function fmt(x,   p, s) {
            for (p = 15; p <= 17; p++) {
                s = sprintf("%." p "g", x)
                if (s + 0 == x) break
            }
            if (s !~ /[.eEn]/) s = s ".0"
            return s
        }
        BEGIN { x = a + 0; y = b + 0; printf "%s %s %s\n", fmt(x), fmt(y), fmt(x + y) }')
    body="<html><body><h1>$fa + $fb = $fr</h1></body></html>"
else
    body="<html><body><h1>Invalid numbers</h1></body></html>"
fi

printf 'Content-Type: text/html\nContent-Length: %d\n\n%s' "${#body}" "$body"
