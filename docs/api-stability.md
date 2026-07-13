# API Stability Policy

## Scope

Headers under `zeta/<module>/` are public API. Headers under
`zeta/<module>/internal/` are implementation details and may change between
minor releases.

The `zeta::zeta` umbrella target and existing public include paths are kept for
compatibility. New code should link the smallest module target it needs.

## Versioning

Zeta follows Semantic Versioning:

- Patch releases fix behavior, documentation, or build issues without changing
  the intended public contract.
- Minor releases add backward-compatible modules and APIs.
- Major releases may remove deprecated APIs or change incompatible contracts.

The project requires C++20. A compiler or standard-library compatibility
break is treated as a major-version concern unless explicitly documented.

## Compatibility rules

- Existing public include paths remain valid throughout a major version.
- Deprecated APIs remain available for at least two minor releases when a
  practical replacement exists.
- Status codes, enum values, serialization-visible layouts, and documented
  cancellation semantics are compatibility-sensitive.
- Header-only implementation changes may increase compile time or template
  instantiation cost; such changes require a benchmark or a written tradeoff.
- `internal/` symbols must not be used as application integration points.

## API change checklist

Every public API change should include:

1. A usage-oriented test.
2. An example or documentation update when the behavior is user-facing.
3. A module-level CMake target check.
4. A fuzz target or benchmark when the change affects parsing, memory safety,
   concurrency, or a hot path.
5. A changelog entry describing compatibility impact.
