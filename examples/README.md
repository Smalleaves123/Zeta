# Examples

These example programs are intended for end users of Zeta.

## Build from the repository

```bash
cmake -S . -B build/examples -DZETA_BUILD_EXAMPLES=ON -DZETA_BUILD_TESTS=OFF
cmake --build build/examples
```

## Run

```bash
./build/examples/examples/zeta_example_quickstart
./build/examples/examples/zeta_example_status_time
./build/examples/examples/zeta_example_time_control
./build/examples/examples/zeta_example_time_retry
./build/examples/examples/zeta_example_log_random
./build/examples/examples/zeta_example_strings_pipeline
./build/examples/examples/zeta_example_memory_views
./build/examples/examples/zeta_example_ordered_index
./build/examples/examples/zeta_example_hashing
```

## What each example shows

- `quickstart.cpp`: `flat_hash_map`, `InlinedVector`, `StrCat`, `StrJoin`, `StrSplit`
- `status_time.cpp`: `Result`, `Status`, integer parsing, local/UTC timestamps
- `time_control.cpp`: `Stopwatch`, `Deadline`, `PeriodicTimer`, `ExponentialBackoff`
- `time_retry.cpp`: retry budget with attempts, deadline, and exponential backoff
- `log_random.cpp`: structured logging, `BitGen`, `Uniform`, `Bernoulli`
- `strings_pipeline.cpp`: split, trim, normalize, join, formatted output
- `memory_views.cpp`: `Span` plus `function_ref` for zero-allocation callbacks
- `ordered_index.cpp`: ordered lookup with `btree_map` and UTC timestamps
- `hashing.cpp`: payload hashing and range hashing over parsed fields
