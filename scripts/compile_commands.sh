#!/bin/sh

make --always-make --dry-run \
 | grep -wE 'cc|gcc|clang' \
 | jq -nR --arg wd "$PWD" '[inputs|{"directory": $wd, "command":., "file": match(" [^ ]+$").string[1:]}]' \
> compile_commands.json

