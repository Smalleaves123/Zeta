# Changelog

All notable changes to Zeta are documented here.

## [0.2.0] - 2026-07-13

### Added

- `zeta::metrics`: thread-safe `Counter`, `Gauge`, `Histogram`, and
  `ScopedTimer` primitives.
- Cooperative Future cancellation with `CancellationSource`/
  `CancellationToken`, `Promise::Cancel()`, and `Future::GetFor()`.
- Portable fuzz smoke mode, libFuzzer engine selection, and CI fuzz coverage.

### Changed

- Future fan-in combinators now use continuations instead of detached helper
  threads.
- Tests link individual module targets to validate transitive dependencies.
- `Duration` division by zero follows the documented non-throwing behavior.

## [0.1.0]

Initial modular C++20 utility library release.
