#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TEST_DIR=$(mktemp -d "${TMPDIR:-/tmp}/alteria-tests.XXXXXX")
trap 'rm -rf "$TEST_DIR"' 0 HUP INT TERM
CXX=${CXX:-clang++}
for SUITE in ALTERIA_ALLROUNDER ALTERIA_DRUM_LAB; do
  cd "$ROOT/firmware/$SUITE"
  if [ "$SUITE" = ALTERIA_ALLROUNDER ]; then CASES="allrounder_test demo_regression calibration_test"; else CASES="drum_lab_test calibration_test"; fi
  for CASE in $CASES; do
    "$CXX" -std=c++11 -fsanitize=address,undefined -Itests/stubs "tests/$CASE.cpp" -o "$TEST_DIR/$SUITE-$CASE"
    "$TEST_DIR/$SUITE-$CASE"
  done
done
