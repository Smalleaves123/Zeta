# Zeta Architecture Direction

Zeta is converging on an Abseil-style project shape rather than a Folly-style
monolithic infrastructure library.

## Principles

- Public APIs live in `zeta/<module>/...`.
- Non-stable implementation detail lives in `zeta/<module>/internal/...`.
- CMake exposes small module targets such as `zeta::status` and
  `zeta::strings`.
- `zeta::zeta` stays available as a compatibility umbrella target.
- New APIs should land in a domain module, not in a generic catch-all header.
- Tests should read like usage examples for the public API, not just assertions.
- Future cancellation is cooperative; executors are borrowed and must outlive
  all continuations scheduled through `Via()`.

## Stable Module Families

### Stable Public Modules

- `algorithm`
- `base`
- `status`
- `strings`
- `time`
- `memory`
- `functional`
- `debugging`
- `metrics`
- `container`
- `crc`
- `hash`
- `random`
- `types`
- `futures`

### Low-Level Public Modules

- `bits`
- `cleanup`
- `meta`
- `numeric`
- `synchronization`
- `flags`
- `log`

### Compatibility / Transitional Modules

- `utility`

## Current Compatibility Posture

The repository still exposes `bits` and `utility` as public modules. This is a
compatibility choice, not the preferred long-term shape.

When expanding the library:

- Prefer `time` over adding time helpers to `utility`
- Prefer `strings` over adding text helpers to `utility`
- Prefer `memory` or `container` over adding storage helpers to `utility`
- Prefer `functional` for callback composition and visitor helpers rather than
  adding unrelated adapters to `memory`
- Prefer `internal/` over polluting top-level public module directories with
  implementation details
- Prefer adding request-flow time helpers to `time` rather than scattering
  deadline / stopwatch logic across service code
- Prefer `base` for foundational helpers that are used across modules rather
  than piling them into `utility`

Logging belongs in `log`: application-facing records may attach structured
fields, while formatting and sink selection remain replaceable at the module
boundary. New observability integrations should consume `LogRecordView` rather
than depend on formatter internals.

Configuration belongs in `flags`: parsing, environment precedence, help
generation, and validation should remain in one startup-facing module rather
than being reimplemented by individual services.

Calendar values belong in `time`: use `CivilDate` and `CivilTime` for dates that
do not yet represent an instant, and require an explicit `TimeZoneSpec` when
converting them to Unix time.

The `base` module now owns the cross-platform compiler and platform detection
surface, declaration attributes, optimization hints, and Clang thread-safety
annotations. These headers should remain dependency-light so every other
module can use them without introducing coupling.

## Near-Term Refactor Path

1. Keep all existing include paths valid.
2. Add and validate per-module CMake targets.
3. Move new implementation helpers into module-local `internal/` directories.
4. Gradually drain generic buckets such as `utility` into explicit modules.
5. Expand examples, tests, fuzzing, and benchmarks per module rather than
   through the umbrella target.

## Test Style

Zeta tests are expected to do two jobs:

- Verify behavior.
- Show readers how the public API is intended to be used.

That means test files should prefer:

- Public headers instead of internal headers.
- Small scenario-driven test cases over dense implementation probes.
- Names that describe usage or behavior, not just low-level mechanics.
- Explicit examples for edge cases that matter to callers.

Internal headers may still have focused unit coverage when the implementation
needs it, but the default test style should stay close to documentation.
