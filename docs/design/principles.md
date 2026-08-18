# Design Principles

## Policy-based types

Fixmath makes every choice that affects numeric semantics part of the type:

```cpp
using value_type = fixmath::fixed<fixmath::fixed_policy<
	fixmath::int64_t,
	32,
	fixmath::arithmetic_mode::SaturationMode,
	fixmath::rounding_mode::RoundToEven>>;
```

`fixed_policy` determines the signed underlying integer, number of binary fractional bits, arithmetic mode, and rounding mode. These are compile-time constants, so different policies produce different C++ types. Core operations use `if constexpr` to eliminate unselected branches and require no runtime policy state or lookup.

Binary operations between Fixmath values currently require both operands to have the same `fixed<policy>` type. Small integers can be converted to that fixed-point type through `std::common_type`. Different Q formats do not participate in implicit common-precision deduction because that would hide changes in scaling, range, and rounding.

## Explicit floating point, seamless integer interaction

Converting floating-point values to Fixmath must be an explicit decision by the caller. Both `fixed(float)` and `fixed(double)` are `explicit`, and Fixmath provides no mixed arithmetic or comparison overloads between `fixed` and `float` / `double`. Construction and arithmetic must first make the conversion explicit:

```cpp
const value_type rate{0.125};
const value_type result = amount * value_type{1.5};
```

This boundary prevents an ordinary floating-point expression from silently becoming fixed-point arithmetic during overload resolution. It also makes the rounding, saturation, or special-value mapping caused by conversion to a Q format visible at the call site.

Integers are intended to behave like ordinary scalar operands. Integer types whose integral promotion produces `int32_t` can participate directly in addition, subtraction, multiplication, division, and comparisons with `fixed` through `std::common_type`, without writing `fixed{integer}` at every use site:

```cpp
const value_type next = amount + 1;
const bool enough = 2 * amount >= limit;
```

The integer is still converted to the target policy's Q format before the operation. If it lies outside the type's representable range, construction follows Fixmath's existing boundary semantics. This implicit interaction does not cover 64-bit integers or different `fixed<policy>` types; callers must choose those conversions explicitly.

## Cross-platform consistency first

Given the same policy and raw inputs, Fixmath should produce the same raw result on every supported platform, including overflow direction and midpoint rounding. To achieve that:

- Numeric semantics are defined with integer bit operations and explicit rounding rules, independently of the host floating-point environment's current rounding mode.
- Addition and subtraction overflow use compiler builtins or equivalent unsigned bitwise detection.
- When 64-bit multiplication or division requires a 128-bit intermediate, GCC/Clang platforms use `__int128` or target instructions, MSVC x64 uses intrinsics, and other platforms fall back to software 128-bit implementations.
- Platform-specific implementations may optimize how a result is computed, but must not change the public result.

Cross-platform consistency does not imply identical performance on every platform. Native 128-bit instructions and software fallbacks can have substantially different costs.

## No exceptions

Fixmath numeric operations do not throw C++ exceptions and do not use exceptions as the return channel for overflow, division by zero, or domain errors:

- Saturation mode returns boundary values.
- Strict mode represents non-representable or non-finite results with `nan` / `inf`.
- Ignore mode preserves modular arithmetic semantics; cases without an ordinary numeric result, such as division by zero, return the mode's designated sentinel raw value.

Diagnostics are controlled by `FIXMATH_ERROR` / `FIXMATH_ASSERT`. The default implementation uses the C `assert` facility, which may terminate the process but does not throw an exception. Defining `FIXMATH_USE_ASSERT=0` disables these diagnostics. Callers must not treat assertions as recoverable error handling; they must inspect results according to the selected policy or validate inputs before calling an operation.

## Coding style

The project follows `.clang-format` and `AGENTS.md` at the repository root:

- Use C++20 and keep the library header-only. The specific language and library dependencies are documented in [Required C++20 Features](cpp20-requirements.md). Public declarations belong in `.hpp` files and template implementations in `.inl` files.
- Functions and variables use `snake_case`, types use descriptive names, compile-time constants use `UPPER_SNAKE_CASE`, and internal helpers use the `_fm_` prefix.
- All project-defined macros use the `FIXMATH_` prefix. Do not introduce generic macro names, because macros are not scoped by C++ namespaces and can collide with application code, platform headers, or other libraries.
- Use tabs for indentation, place opening braces on the declaration line, and preserve the MPL 2.0 notice at the top of source files.
- Keep template parameters in internal helpers only when the result type or compile-time policy genuinely controls the implementation.
- Optimizations must first be testable as equivalent to the portable reference path. Undefined behavior is not an acceptable performance technique.
