#!/usr/bin/env bash
# Portable synthetic-only CI. No Arduino/device, endpoints or private inputs.
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
dep="${ARDUINOJSON_SOURCE:?Set ARDUINOJSON_SOURCE to clean official locked checkout}"
out="${HOST_OUTPUT:?Set HOST_OUTPUT to a new build directory}"
pin=77771d3c07668e01d8f52acb03910c1110bb373f
test "$(git -C "$dep" rev-parse HEAD)" = "$pin"
test -z "$(git -C "$dep" status --porcelain)"
test ! -e "$out"
mkdir -p "$out"
core="$repo/code/controller/MiGardenController/src/core"
args=(-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I "$core" -I "$dep/src")
for target in core_tests sim; do
    g++ "${args[@]}" "$core"/*.cpp "$repo/code/controller/tests/$target.cpp" -o "$out/$target"
done
g++ "${args[@]}" "$repo/code/controller/MiGardenController/src/commissioning/transaction.cpp" \
    "$repo/code/controller/tests/commissioner_tests.cpp" -o "$out/commissioner_tests"
"$out/core_tests" "$repo/contracts/zone-a/v1/vectors.json"
"$out/commissioner_tests"
python "$repo/code/controller/tests/verify-wire.py" "$out/sim"
