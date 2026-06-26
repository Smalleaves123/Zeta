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

## Stable Module Families

### Stable Public Modules

- `status`
- `strings`
- `time`
- `memory`
- `container`
- `hash`
- `random`

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
- Prefer `internal/` over polluting top-level public module directories with
  implementation details
- Prefer adding request-flow time helpers to `time` rather than scattering
  deadline / stopwatch logic across service code

## Near-Term Refactor Path

1. Keep all existing include paths valid.
2. Add and validate per-module CMake targets.
3. Move new implementation helpers into module-local `internal/` directories.
4. Gradually drain generic buckets such as `utility` into explicit modules.
5. Expand examples, tests, fuzzing, and benchmarks per module rather than
   through the umbrella target.
