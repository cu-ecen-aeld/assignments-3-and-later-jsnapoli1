#!/usr/bin/env bash
set -euo pipefail

output_file="cross-compile.txt"

mkdir -p "$(dirname "$output_file")"

aarch64-none-linux-gnu-gcc -print-sysroot -v 2>&1 | tee -a "$output_file"
