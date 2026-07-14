# Zeta Module Map

This document is the maintenance map for the public CMake targets. Public
headers stay under `zeta/<module>/`; implementation-only headers stay under the
module's `internal/` directory.

## Dependency layers

The module registration order in `zeta/CMakeLists.txt` follows these layers:

| Layer | Modules | Rule |
| --- | --- | --- |
| Foundation | `algorithm`, `base`, `bits`, `cleanup`, `meta`, `numeric`, `synchronization` | No Zeta module dependencies |
| Core values | `memory`, `status`, `time`, `strings`, `types` | Reusable value and platform abstractions |
| Data and adapters | `container`, `crc`, `hash`, `random`, `functional` | Depends only on foundation/core modules |
| Infrastructure | `debugging`, `flags`, `futures`, `log`, `metrics`, `utility` | Application-facing facilities and compatibility surface |

Key edges:

```text
base ───────────────► debugging, types, utility
bits ───────────────► container, hash, random
status ─────────────► futures, strings
memory ─────────────► functional
time ───────────────► log, metrics
strings ────────────► flags
```

Each module exposes an interface target named `zeta::<module>`. The umbrella
`zeta::zeta` target links every module and remains available for compatibility;
new consumers should prefer the smallest module target that covers their
includes.

## Ownership rules

- Put a new public API in the narrowest domain module that owns its concept.
- Put reusable implementation machinery in `<module>/internal/`.
- Add a module-local test under `tests/<module>/` and register it through the
  shared `zeta_test()` helper.
- Add a package-consumer include when the new API changes the installed public
  surface.
- Keep `utility` compatibility-only; do not add unrelated new APIs there.

The CMake helpers in `cmake/ZetaModule.cmake` and `cmake/ZetaTest.cmake`
validate module/test target references during configuration, so dependency
ordering errors fail early instead of becoming linker surprises.
