#!/usr/bin/env bash
set -euo pipefail
nr_root="$(cd "$(dirname "$0")" && pwd)"
nr_build="$nr_root/build"
mkdir -p "$nr_build" "$nr_root/results"
nr_compiler="${CXX:-c++}"
nr_flags=(-std=c++17 -O2 -Wall -Wextra -Wpedantic)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  nr_flags+=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"$nr_compiler" "${nr_flags[@]}" "$nr_root/examples/nr_core.cpp" "$nr_root/examples/nr_tests.cpp" -o "$nr_build/nr_tests"
"$nr_compiler" "${nr_flags[@]}" "$nr_root/examples/nr_core.cpp" "$nr_root/examples/nr_examples.cpp" -o "$nr_build/nr_examples"
"$nr_build/nr_tests" > "$nr_root/results/verification.txt"
cat "$nr_root/results/verification.txt"
"$nr_build/nr_examples" --out "$nr_root/results"
