#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TEST_DIR=$(mktemp -d "${TMPDIR:-/tmp}/alteria-tests.XXXXXX")
trap 'rm -rf "$TEST_DIR"' EXIT
CXX=${CXX:-clang++}
for SKETCH in "$ROOT"/*/Firmware/ALTERIA_*; do
  for TEST in "$SKETCH"/tests/*_test.cpp "$SKETCH"/tests/*_regression.cpp; do
    [ -f "$TEST" ] || continue
    "$CXX" -std=c++11 -O1 -fsanitize=address,undefined -I"$SKETCH/tests/stubs" "$TEST" -o "$TEST_DIR/run"
    "$TEST_DIR/run"
  done
done
