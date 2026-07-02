# Zeta — A Modern C++20 Utility Library

Zeta is a header-only C++20 library inspired by [Abseil](https://github.com/abseil/abseil-cpp), focusing on **efficiency-critical primitives** that outperform or complement their standard-library counterparts. Every component is designed for real production use — not demos.

## Quick Start

```cpp
#include <zeta/container/flat_hash_map.h>
#include <zeta/strings/str_cat.h>
#include <zeta/container/inlined_vector.h>

int main() {
    // Swiss Table hash map (2-3x faster than std::unordered_map)
    zeta::flat_hash_map<std::string, int> m;
    m["hello"] = 42;
    m.try_emplace("world", 99);

    // Single-allocation string concatenation
    std::string msg = zeta::StrCat("pi = ", 3.14, ", answer = ", 42);

    // Small-buffer-optimized vector (no heap for ≤ N elements)
    zeta::InlinedVector<int, 8> v = {1, 2, 3};
    v.push_back(4); // still on the stack
}
```

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                    # 7 test suites, 100% pass
```

### Examples

```bash
cmake -S . -B build/examples -DZETA_BUILD_EXAMPLES=ON -DZETA_BUILD_TESTS=OFF
cmake --build build/examples
./build/examples/examples/zeta_example_quickstart
./build/examples/examples/zeta_example_strings_pipeline
```

More example programs: [examples/README.md](./examples/README.md).

### Fuzzing

```bash
cmake -S . -B build-fuzz -DZETA_BUILD_FUZZERS=ON -DZETA_BUILD_TESTS=OFF
cmake --build build-fuzz
./build-fuzz/fuzz/strings_split_fuzz
./build-fuzz/fuzz/strings_utils_fuzz
./build-fuzz/fuzz/strings_join_fuzz
./build-fuzz/fuzz/strings_format_fuzz
./build-fuzz/fuzz/hash_hash_fuzz
./build-fuzz/fuzz/strings_str_cat_fuzz
./build-fuzz/fuzz/numeric_int128_fuzz
./build-fuzz/fuzz/container_inlined_vector_fuzz
./build-fuzz/fuzz/strings_numbers_fuzz
./build-fuzz/fuzz/status_status_fuzz
./build-fuzz/fuzz/status_statusor_fuzz
./build-fuzz/fuzz/container_btree_map_fuzz
./build-fuzz/fuzz/random_random_fuzz
```

Override `ZETA_FUZZING_ENGINE` if your local toolchain needs different
sanitizer or libFuzzer flags.

More detailed local workflow and corpus layout: [fuzz/README.md](./fuzz/README.md).
OSS-Fuzz bootstrap files live in `fuzz/project.yaml` and `fuzz/oss_fuzz_build.sh`.

**Requirements:** C++20, CMake 3.20+. The library is header-only — just add `-I<path-to-zeta>` to your build.

### CMake Targets

Zeta keeps `zeta::zeta` as a compatibility umbrella target, but the preferred
integration style is Abseil-like module linking:

```cmake
find_package(zeta REQUIRED CONFIG)

target_link_libraries(my_app PRIVATE
    zeta::status
    zeta::strings
    zeta::time
)
```

Available module targets currently include:

- `zeta::bits`
- `zeta::base`
- `zeta::cleanup`
- `zeta::container`
- `zeta::flags`
- `zeta::hash`
- `zeta::log`
- `zeta::memory`
- `zeta::meta`
- `zeta::numeric`
- `zeta::random`
- `zeta::status`
- `zeta::strings`
- `zeta::synchronization`
- `zeta::time`
- `zeta::utility`

**SIMD backends** are auto-detected:
| Platform | Backend | Probing speed |
|----------|---------|--------------|
| ARM64 (Apple Silicon) | NEON | 16 slots / instruction |
| x86-64 | SSE2 | 16 slots / instruction |
| Other | Scalar | loop fallback |

---

## Project Structure

```
cpp-/
├── CMakeLists.txt                    # Top-level build
├── zeta/                             # Library root (equivalent to absl/)
│   ├── base/                         # Foundational helpers
│   ├── status/                       # Error model and propagation helpers
│   ├── strings/                      # Text building, splitting, formatting
│   │   └── internal/                 # Non-public string implementation detail
│   ├── time/                         # Duration / clock / timestamp primitives
│   ├── container/                    # Hash and ordered containers
│   │   └── internal/                 # Container internals, not API-stable
│   ├── memory/                       # Views and callable adapters
│   ├── synchronization/              # Mutex / once / notification
│   ├── hash/                         # Hash framework
│   ├── random/                       # PRNG and distributions
│   ├── numeric/                      # Numeric primitives
│   ├── flags/                        # Command-line flag parsing
│   ├── log/                          # Lightweight logging
│   ├── bits/                         # Low-level bit utilities
│   ├── cleanup/                      # Scope guard utilities
│   ├── meta/                         # Traits and compile-time helpers
│   └── utility/                      # Compatibility bucket; shrinking over time
├── tests/                            # Module-oriented tests and usage docs
├── examples/                         # End-user examples
├── benchmarks/                       # Microbenchmarks
└── fuzz/                             # Fuzz targets and corpora
```

Tests are intentionally written to read like usage notes for the public API.
See [tests/README.md](./tests/README.md) for the conventions we follow.

### Structure Direction

The project is intentionally converging on an Abseil-style layout:

- Public headers live under `zeta/<module>/...`
- Module internals live under `zeta/<module>/internal/...`
- CMake exposes one target per module
- `zeta::zeta` remains for compatibility, not as the preferred long-term entry

Modules such as `bits` and `utility` currently remain public for compatibility,
but new APIs should prefer landing in clearer domain modules like `base`,
`strings`, `status`, `time`, `memory`, or `container`.

---

## Module Reference

### 1. `zeta/memory/function_ref.h`

A lightweight, non-owning, type-erased callable reference — analogous to `std::string_view` but for callables.

| Feature | `std::function` | `zeta::function_ref` |
|---------|:---:|:---:|
| Heap allocation | May allocate | **Never** |
| Size (typical) | 32+ bytes | **16 bytes** (2 pointers) |
| Owns the callable | Yes | No (view only) |
| noexcept variant | No | **Yes** |

```cpp
#include <zeta/memory/function_ref.h>

void for_each(int* data, size_t n, zeta::function_ref<void(int&)> fn) {
    for (size_t i = 0; i < n; ++i) fn(data[i]);
}

int main() {
    int sum = 0;
    for_each(arr, n, [&sum](int& x) { sum += x; });        // lambda
    for_each(arr, n, [factor = 2](int& x) { x *= factor; }); // capture lambda

    // noexcept variant
    zeta::function_ref<int(int) noexcept> ref = [](int x) noexcept { return x * 2; };
}
```

---

### 2. `zeta/memory/span.h`

`zeta::Span<T>` is a thin alias over `std::span<T>` for non-owning views over contiguous data.

```cpp
#include <zeta/memory/span.h>

std::vector<int> v = {1, 2, 3};
zeta::Span<int> s(v);
auto tail = s.subspan(1);  // view of {2, 3}
```

---

### 3. `zeta/strings/str_cat.h` — `StrCat` / `StrAppend`

Efficient variadic concatenation. Unlike `a + b + c` (which creates intermediate temporaries), `StrCat` pre-computes the total length and performs **one allocation**.

```cpp
#include <zeta/strings/str_cat.h>

std::string s = zeta::StrCat("x = ", 10, ", y = ", 3.14, ", ok = ", true);
// s == "x = 10, y = 3.14, ok = true"

// Append to existing string (single reallocation)
std::string buf = "log: ";
zeta::StrAppend(buf, "event=", event_id, ", ts=", timestamp);
```

Supported types: `const char*`, `std::string`, `std::string_view`, `char`, `bool`, all integral types, `float`, `double`.

---

### 4. `zeta/strings/str_split.h` — `StrSplit`

Zero-copy split yielding `std::string_view` pieces. No heap allocations, no substring copies.

```cpp
#include <zeta/strings/str_split.h>

// By single character
for (auto part : zeta::StrSplit("a,b,c", ','))
    std::cout << part << "\n"; // "a", "b", "c"

// By multi-character string
for (auto part : zeta::StrSplit("key=>value=>flag", "=>"))
    std::cout << part << "\n"; // "key", "value", "flag"

// By any character in a set
for (auto part : zeta::StrSplit("x, y; z", ByAnyChar{",; "}, SkipEmpty))
    std::cout << part << "\n"; // "x", "y", "z"
```

**Options (compile-time, zero-cost):**
| Option | Effect |
|--------|--------|
| `SkipEmpty` | Omit empty pieces between consecutive delimiters |
| `MaxSplits_t{N}` | Stop after producing `N` pieces (remainder in last piece) |

```cpp
// Split into at most 3 pieces
for (auto p : StrSplit("a:b:c:d:e", ':', MaxSplits_t{3}))
    // produces: "a", "b", "c:d:e"
```

---

### 5. `zeta/strings/str_join.h` — `StrJoin`

Join elements of a range with a delimiter. Pre-computes total length for a single allocation.

```cpp
#include <zeta/strings/str_join.h>

std::vector<std::string> v = {"a", "b", "c"};
std::string s = zeta::StrJoin(v, ", ");   // "a, b, c"

std::vector<int> nums = {1, 2, 3, 4};
std::string csv = zeta::StrJoin(nums, ","); // "1,2,3,4"

// Initializer list
std::string dash = zeta::StrJoin({"x", "y", "z"}, "-"); // "x-y-z"
```

---

### 6. `zeta/strings/str_utils.h` — Predicates, Stripping, Replacement

```cpp
#include <zeta/strings/str_utils.h>

// ── Predicates ──
bool ok = zeta::StartsWith("hello world", "hello");      // true
bool ok = zeta::EndsWith("file.txt", ".txt");             // true
bool ok = zeta::StrContains("hello world", "lo wo");      // true
bool ok = zeta::StartsWithIgnoreCase("Hello", "hello");   // true

// ── Stripping (returns string_view, zero-copy) ──
auto sv = zeta::StripPrefix("--verbose", "--");           // "verbose"
auto sv = zeta::StripSuffix("data.tar.gz", ".gz");        // "data.tar"
auto sv = zeta::StripAsciiWhitespace("  hello  ");        // "hello"

// ── Case conversion ──
std::string lo = zeta::AsciiStrToLower("Hello World!");   // "hello world!"
std::string hi = zeta::AsciiStrToUpper("Hello World!");   // "HELLO WORLD!"

// ── Replacement ──
std::string s = zeta::StrReplaceFirst("a,b,c", ",", "|");  // "a|b,c"
std::string s = zeta::StrReplaceAll("a,b,c", ",", "|");    // "a|b|c"
```

---

### 7. `zeta/container/flat_hash_map.h` & `flat_hash_set.h`

Swiss Table hash containers. Typically **2–3× faster** than `std::unordered_map` with ~30% less memory.

**Why it's faster:**
- **Flat storage** — all slots in a contiguous array (cache-friendly)
- **1-byte metadata per slot** — 16 slots probed in a single SIMD instruction
- **Open addressing** — no pointer-chasing linked lists

```cpp
#include <zeta/container/flat_hash_map.h>
#include <zeta/container/flat_hash_set.h>

// ── Map ──────────────────────────────────────────────
zeta::flat_hash_map<std::string, int> m;
m["alpha"] = 1;
m["beta"]  = 2;

// try_emplace — key + value args, no value constructed if key exists
auto [it, ok] = m.try_emplace("gamma", 3);

// insert_or_assign — overwrite if key exists
m.insert_or_assign("alpha", 100);  // now m["alpha"] == 100

for (const auto& [k, v] : m)
    std::cout << k << " = " << v << "\n";

// const-correct lookup
const auto& cm = m;
auto cit = cm.find("alpha");  // returns const_iterator

// ── Set ──────────────────────────────────────────────
zeta::flat_hash_set<int> s = {1, 2, 3, 4, 5};
if (s.contains(3)) { /* ... */ }
s.erase(2);

// ── Heterogeneous lookup (no temporary std::string!) ─
struct TransparentHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};
struct TransparentEq {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

zeta::flat_hash_map<std::string, int, TransparentHash, TransparentEq> hm;
hm["hello"] = 42;
auto it = hm.find("hello"sv);  // no std::string constructed!
```

### 8. `zeta/status/result.h`

`zeta::Result<T>` is an alias for `zeta::StatusOr<T>` when a result-oriented name is clearer in application code.

```cpp
#include <zeta/status/result.h>

zeta::Result<int> ParseInt(std::string_view s);
```

**Full API surface:**
| Method | Description |
|--------|------------|
| `find(key)` | Heterogeneous lookup, const + non-const |
| `contains(key)` | Fast membership test, `const` |
| `count(key)` | Returns 0 or 1 |
| `at(key)` | Bounds-checked access, const + non-const |
| `operator[]` | Insert-or-access (map only) |
| `insert(pair)` | Insert if key not present |
| `emplace(args...)` | In-place construction |
| `try_emplace(key, args...)` | Key + value args, no value constructed if duplicate |
| `insert_or_assign(key, value)` | Overwrite if key exists (map only) |
| `erase(key)` | Heterogeneous erase |
| `erase(iterator)` | Erase by position |
| `clear()` | Remove all elements |
| `swap(other)` | O(1) pointer swap |
| `reserve(n)` / `rehash(n)` | Pre-allocate buckets |
| `==` / `!=` | Value equality comparison |

---

### 7. `zeta/container/inlined_vector.h` — `InlinedVector<T, N>`

A `std::vector` with **small-buffer optimization**. Up to `N` elements are stored **inside the object** (on the stack), avoiding heap allocation entirely. When the size exceeds `N`, elements are transparently moved to the heap.

```cpp
#include <zeta/container/inlined_vector.h>

// Up to 8 elements stored inline — no heap allocation
zeta::InlinedVector<int, 8> v = {1, 2, 3};

v.push_back(4);          // still inline
v.push_back(5);          // still inline
// ... up to 8

v.push_back(9);          // 9th element → moves to heap (transparent)
// v now has heap-backed storage with capacity ≥ 16

v.shrink_to_fit();       // if size ≤ 8, moves back to inline storage

// Full std::vector API:
v.emplace_back(10);
v.insert(v.begin(), 0);
v.erase(v.begin() + 2);
v.resize(3);
v.reserve(100);
```

**When to use:** Replace `std::vector<T>` with `InlinedVector<T, N>` when most instances hold ≤ N elements. Common choices:
| N | Typical use |
|---|------------|
| 4–8 | Function arguments, small collections |
| 16–32 | Token lists, path components, word buffers |
| 64–128 | Per-request buffers, line readers |

---

## Design Principles

1. **Modern C++ first.** C++20 throughout — concepts, `if constexpr`, `constexpr` where possible, `[[no_unique_address]]`, `std::to_chars`, fold expressions.

2. **Zero-cost abstractions.** Template flags (e.g. `SkipEmpty`, `MaxSplits`) are compile-time, not runtime booleans. SIMD backend selection uses `if constexpr`.

3. **Const-correctness.** `const_iterator` is a distinct type. `find`/`contains`/`count` have proper `const` overloads. Heterogeneous lookup works on `const` tables.

4. **Heterogeneous by default.** Any lookup/erase/count method templates on the key type, constrained with transparent hash/equal detection.

5. **Production reliability.** 7 test suites, 100% pass rate. Move-only types supported. Exception-safe insert paths. Proper iterator invalidation semantics.

---

## Performance (Micro-benchmarks)

Measured on Apple M1, clang-16, Release build. Results are indicative — always benchmark with your own workload.

| Benchmark | `std::unordered_map` | `zeta::flat_hash_map` | Speedup |
|-----------|:---:|:---:|:---:|
| Insert 1M int→int | 48 ms | 21 ms | **2.3×** |
| Lookup 1M (hit) | 22 ms | 9 ms | **2.4×** |
| Erase 500k | 18 ms | 8 ms | **2.3×** |
| Memory (1M pairs) | 48 MB | 32 MB | **33% less** |

The benchmark suite also includes `time/retry` control-flow coverage so retry/backoff policies can be measured alongside container hot paths.

| Benchmark | `std::vector` | `zeta::InlinedVector<T,8>` | Benefit |
|-----------|:---:|:---:|:---:|
| Create 1M (size ≤ 3) | 1M allocs | **0 allocs** | No heap |
| Push 1000× (avg size 5) | 7-8 allocs/iter | **0 allocs/iter** | No heap |

---

## License & Contributing

MIT License. Contributions welcome — open an issue or PR with benchmarks and tests.
