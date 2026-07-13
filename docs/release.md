# Release Workflow

1. Update `project(zeta VERSION ...)` in the top-level `CMakeLists.txt`.
2. Add a dated entry to `CHANGELOG.md`.
3. Run the default, warnings-as-errors, sanitizer, package smoke, examples,
   and portable fuzz builds.
4. Confirm the installed package exposes every documented module target.
5. Create an annotated tag matching the version, for example `v0.2.0`.
6. Push the commit and tag, then publish release notes from the changelog.

The public release gate is:

```bash
cmake --preset ci-release
cmake --build --preset ci-release
ctest --preset ci-release --output-on-failure
cmake --install build/ci-release --prefix stage
```

Release candidates should also run the package consumer, examples, fuzz smoke
targets, and at least one sanitizer preset.
