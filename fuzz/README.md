# Fuzzing

This directory contains libFuzzer-style harnesses for high-surface Zeta APIs.

## Build

```bash
cmake -S . -B build-fuzz -DZETA_BUILD_FUZZERS=ON -DZETA_BUILD_TESTS=OFF
cmake --build build-fuzz
```

If your toolchain does not ship libFuzzer by default, override
`ZETA_FUZZING_ENGINE`.

Examples:

```bash
cmake -S . -B build-fuzz \
  -DZETA_BUILD_FUZZERS=ON \
  -DZETA_BUILD_TESTS=OFF \
  -DZETA_FUZZING_ENGINE='-fsanitize=fuzzer,address,undefined'
```

## Run

Single target:

```bash
./build-fuzz/fuzz/strings_split_fuzz
```

With a seed corpus:

```bash
./build-fuzz/fuzz/strings_split_fuzz fuzz/corpus/strings_split_fuzz
```

Persist new interesting inputs:

```bash
mkdir -p fuzz/artifacts/strings_split_fuzz
./build-fuzz/fuzz/strings_split_fuzz \
  fuzz/corpus/strings_split_fuzz \
  -artifact_prefix=fuzz/artifacts/strings_split_fuzz/
```

## Layout

- `*.cpp`: harness source files
- `corpus/<target>/`: optional seed inputs per target
- `artifacts/<target>/`: crashers and minimized reproducers
- `oss_fuzz_build.sh`: OSS-Fuzz style build entrypoint
- `project.yaml`: OSS-Fuzz project metadata skeleton

## Current targets

- `strings_split_fuzz`
- `strings_utils_fuzz`
- `strings_join_fuzz`
- `strings_format_fuzz`
- `strings_str_cat_fuzz`
- `strings_numbers_fuzz`
- `hash_hash_fuzz`
- `numeric_int128_fuzz`
- `status_status_fuzz`
- `status_statusor_fuzz`
- `container_inlined_vector_fuzz`
- `container_btree_map_fuzz`
- `random_random_fuzz`

## OSS-Fuzz direction

The current layout is already close to OSS-Fuzz expectations:

- one translation unit per target
- deterministic harnesses
- no dependence on external state
- corpus isolated by target

If this project later adds OSS-Fuzz integration, the missing pieces are mainly:

- replacing placeholder repo/contact fields in `project.yaml`
- wiring `fuzz/oss_fuzz_build.sh` into the OSS-Fuzz project wrapper
- growing the per-target corpora beyond the initial smoke seeds
