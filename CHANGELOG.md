# Changelog

All notable changes to Zeta are documented here.

## [0.7.0] - 2026-07-13

### Added

- Structured `LogField` records and `LogMessage::WithField()` key/value
  logging.
- `JsonLogFormatter` for JSON-lines output with string escaping.
- `ZETA_LOG_IF` for conditional logging without constructing disabled records.
- Structured record support in the default and rotating file sinks while
  preserving the legacy sink overload.

## [0.6.0] - 2026-07-13

### Added

- Portable compiler, platform, architecture, attribute, optimization, and
  thread-safety annotation helpers in `zeta/base/`.
- `ZETA_DEPRECATED`, `ZETA_MUST_USE_RESULT`, `ZETA_NO_UNIQUE_ADDRESS`,
  `ZETA_ASSUME`, `ZETA_PREFETCH`, and `ZETA_RESTRICT` portability macros.

## [0.5.0] - 2026-07-13

### Added

- ASCII character helpers and case conversion in `zeta/strings/ascii.h`.
- Wildcard matching in `zeta/strings/match.h`.
- C-style escaping and `StatusOr`-based unescaping in
  `zeta/strings/escaping.h`.
- Positional `$0` ... `$9` substitution in `zeta/strings/substitute.h`.

## [0.4.0] - 2026-07-13

### Added

- `zeta::crc`: portable CRC32C computation, streaming extension, tests, and
  fuzz coverage.

## [0.3.0] - 2026-07-13

### Added

- `zeta::algorithm`: container-friendly wrappers for common standard
  algorithms, including search, predicates, sorting, transforms, and numeric
  accumulation.

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
