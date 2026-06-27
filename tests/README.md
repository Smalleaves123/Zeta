# Tests

Tests in Zeta are part verification and part documentation.

## Goal

Each test file should help a reader understand:

- Which public header to include.
- Which API to call.
- What behavior to expect.

## Style

- Prefer testing public headers.
- Keep tests scenario-driven and behavior-focused.
- Use names that describe usage, not implementation trivia.
- Keep examples small enough that a reader can skim them as a usage guide.

## Boundary

Implementation detail belongs in `zeta/<module>/internal/` and usually should
not be used from normal tests. When an internal helper needs coverage, keep that
test focused and clearly labeled as internal behavior.

## Examples

- `tests/base/base_test.cpp` shows the foundational helpers that should stay
  close to the public surface of the library.
- `tests/status/status_builder_test.cpp` shows how to attach context to
  `Status` values and how propagation macros behave.
- `tests/time/time_stopwatch_test.cpp` shows how to measure elapsed time and
  work with deadlines.
- `tests/time/time_backoff_test.cpp` shows how retry delays grow and reset.
