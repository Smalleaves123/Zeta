#!/bin/bash
set -euo pipefail

: "${CXX:=clang++}"
: "${CXXFLAGS:=}"
: "${LIB_FUZZING_ENGINE:=}"

if [[ -z "${OUT:-}" ]]; then
  echo "OUT is required" >&2
  exit 1
fi

if [[ -z "${WORK:-}" ]]; then
  WORK="$(mktemp -d)"
fi

cmake -S . -B "${WORK}/build" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
  -DZETA_BUILD_TESTS=OFF \
  -DZETA_BUILD_FUZZERS=ON \
  -DZETA_FUZZING_ENGINE="${LIB_FUZZING_ENGINE}"

cmake --build "${WORK}/build" --parallel

targets=(
  strings_split_fuzz
  strings_utils_fuzz
  strings_join_fuzz
  strings_format_fuzz
  strings_str_cat_fuzz
  strings_numbers_fuzz
  hash_hash_fuzz
  numeric_int128_fuzz
  status_status_fuzz
  status_statusor_fuzz
  container_inlined_vector_fuzz
  container_btree_map_fuzz
  container_node_hash_map_fuzz
  crc32c_fuzz
  random_random_fuzz
)

for target in "${targets[@]}"; do
  cp "${WORK}/build/fuzz/${target}" "${OUT}/${target}"
  if [[ -d "fuzz/corpus/${target}" ]]; then
    zip -qr "${OUT}/${target}_seed_corpus.zip" "fuzz/corpus/${target}"
  fi
done
