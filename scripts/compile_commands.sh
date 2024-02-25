#!/bin/sh

echo $PWD

make --always-make --dry-run \
 | grep -wE 'cc' \
 | grep -w '\-c' \
 | jq -nR --arg wd "$PWD" '[inputs|{"directory": $wd, "command":., "file": match(" [^ ]+$").string[1:]}]' \
> compile_commands.json

