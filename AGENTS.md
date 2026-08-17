# Repository Guidelines

## Project Structure & Module Organization

Fixmath is a header-only C++20 fixed-point arithmetic library. Public headers live in `include/fixmath/`; `fixed.hpp` is the main entry point, `fixed_impl.inl` and `fixed_math.inl` contain core operations, and the remaining `.inl` files provide traits, bit utilities, and platform-independent 128-bit helpers. Tests are consolidated in `tests/unit_tests.cpp`. The `tests/CMakeLists.txt` project downloads GoogleTest during configuration and builds the `FIXMATH_unittests` executable. GitHub Actions configuration is under `.github/workflows/`.

## Build, Test, and Development Commands

Run commands from the repository root:

```sh
cmake -S tests -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The first command configures the test project and fetches GoogleTest, so network access is required on a clean build. The second compiles the C++20 tests. The final command runs all discovered GoogleTest cases and prints failure details. Use a separate build directory (for example, `build-release`) rather than adding generated files to the source tree.

## Coding Style & Naming Conventions

Match the surrounding C++ style: four spaces for indentation, a maximum line length of 120 characters, braces on the
same line as declarations, and `snake_case` for functions, variables, and implementation headers. Types use descriptive
names such as `fixed_policy`; compile-time constants use `UPPER_SNAKE_CASE`. Internal helpers currently use the `_fm_`
prefix. Keep public declarations in `.hpp` files and template implementations in `.inl` files. Preserve the MPL 2.0
notice at the top of source files. No automatic formatter or linter is configured, so minimize unrelated formatting
changes.

Avoid unnecessary templates in internal helpers, but retain template parameters when the result type or compile-time
policy directly controls the implementation. Float/double bit-manipulation helpers should use separate overloads with
the relevant IEEE-754 constants written directly rather than generalized format traits.

## Testing Guidelines

Add coverage to `tests/unit_tests.cpp` using GoogleTest. Follow the existing `TEST(FIXMATH, CASE_NAME)` naming pattern and use `EXPECT_*` assertions. Include boundary cases for overflow, saturation, rounding, signed values, and raw representations. Randomized tests must remain reproducible enough to diagnose failures; prefer fixed seeds for new cases. Run the full CTest command before submitting changes.

## AI-Assisted Contributions

AI tools may assist with documentation, tests, and code. Treat generated output as untrusted: review it for correctness and licensing, disclose material AI assistance, and run the same checks required for hand-written changes.

## Commit & Pull Request Guidelines

Recent commits use short, imperative summaries such as `add sqrt` or `fix a bug ...`. Keep the subject focused on one behavioral change and explain non-obvious arithmetic or portability decisions in the body. Pull requests should describe the affected policy or operation, list test commands and platforms used, and link relevant issues. Include expected-versus-actual numeric examples for bug fixes; screenshots are generally unnecessary for this library.

Commits initiated by an Agent or AI must append `by <specific agent name>` to the commit subject, for example
`fix floating point rounding by Codex`. Use the actual agent name rather than a generic `AI` label.
