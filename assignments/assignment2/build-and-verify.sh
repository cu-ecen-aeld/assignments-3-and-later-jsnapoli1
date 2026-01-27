#!/bin/bash
# Build writer with cross-compiler and verify the executable type
# Output is saved to assignments/assignment2/fileresult.txt

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_FILE="$SCRIPT_DIR/fileresult.txt"

cd "$REPO_ROOT/finder-app"

echo "Cleaning previous build artifacts..."
make clean

echo "Cross-compiling writer for aarch64..."
make CROSS_COMPILE=aarch64-none-linux-gnu-

echo "Verifying executable type..."
file writer | tee "$OUTPUT_FILE"

echo ""
echo "Result saved to: $OUTPUT_FILE"
