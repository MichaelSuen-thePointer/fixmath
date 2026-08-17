# fixmath

[![UnitTests](https://github.com/MichaelSuen-thePointer/fixmath/actions/workflows/cmake.yml/badge.svg)](https://github.com/MichaelSuen-thePointer/fixmath/actions/workflows/cmake.yml)

Fixmath is an experimental, header-only C++20 fixed-point arithmetic library. It provides configurable Q-format precision, arithmetic behavior, and rounding, with basic arithmetic for all supported types and math-function support currently focused on Q32.32 and Q16.16 values.

> **Project status:** The API and test coverage are still evolving. Review behavior carefully before using the library in production or safety-critical code.

## Features

- Selectable signed storage type and number of fractional bits.
- Ignore, strict, and saturating arithmetic modes.
- Round-to-zero and round-to-even policies.
- Integer, floating-point, and raw-representation conversions.
- Fixed-point arithmetic, comparisons, numeric limits, and square root support.
- Portable helpers for platforms without native 128-bit arithmetic.

## Requirements

- A C++20 compiler with concepts and `std::bit_cast` support.
- CMake 3.10 or newer to build the tests.
- Network access during the first test configuration, because CMake fetches GoogleTest.

CI currently exercises Debug builds on Ubuntu and Windows.

## Usage

Add `include/` to your compiler's include path, then include the main header:

```cpp
#include <fixmath/fixed.hpp>

using Q32_32 = fixmath::fixed<fixmath::fixed_policy<
    fixmath::int64_t,
    32,
    fixmath::arithmetic_mode::SaturationMode,
    fixmath::rounding_mode::RoundToEven>>;

int main() {
    const Q32_32 price{19.95};
    const Q32_32 quantity{3};
    const Q32_32 total = price * quantity;
    return total > Q32_32{0} ? 0 : 1;
}
```

Because the library is header-only, no separate library target needs to be linked.

## Build and Test

From the repository root:

```sh
cmake -S tests -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

See [AGENTS.md](AGENTS.md) for repository layout, coding conventions, and contribution guidance.

## AI-Assisted Development

This repository uses AI-assisted development. AI tools may help draft documentation, tests, and code changes. See the [AI-Assisted Contributions](AGENTS.md#ai-assisted-contributions) guidelines for the review and validation requirements applied to generated work.

## License

Fixmath is available under the [Mozilla Public License 2.0](LICENSE).
