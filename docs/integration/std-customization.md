# `std` Customization Points and Compatibility Plans

## Current standard customizations

Fixmath currently provides two standard-library customizations for its user-defined type in `namespace std`.

### `std::numeric_limits`

`std::numeric_limits<fixmath::fixed<policy>>` describes the policy's smallest positive step, finite boundaries, rounding style, and special-value capabilities. Only strict mode declares `has_infinity` and `has_quiet_NaN`; Ignore mode declares `is_modulo`.

### `std::common_type`

When one operand is `fixed` and the other is a small integer type whose integral promotion produces `int32_t`, `std::common_type` selects the `fixed` type. This allows mixed operations such as `fixed + int` and `int * fixed` to reuse the same-format implementation without automatically mixing two different policies.

These are customization locations that the standard reserves for user-defined types. Future customizations must remain constrained to Fixmath types and must not change the behavior of expressions involving only standard-library types.

## Math functions and ADL

The current `sqrt(fixed)` overload is defined in the `fixmath` namespace. Generic code should use the ADL-friendly pattern:

```cpp
using std::sqrt;
auto result = sqrt(value);
```

When `value` is a Fixmath type, argument-dependent lookup finds `fixmath::sqrt`. Built-in floating-point values continue to use `std::sqrt`. Future functions such as `sin` and `cos` should first provide the same formal `fixmath::name(fixed)` interface.

## Future opt-in `std` math overloads

The project plans to add a **disabled-by-default, explicitly macro-enabled** compatibility layer that forwards calls such as `std::sin(fixed)` to the corresponding Fixmath implementation. The proposed umbrella macro is `FIXMATH_ENABLE_UNSAFE_STD_MATH_OVERLOADS`; its exact name and initial function set may still change before implementation.

The purpose of this layer is to minimize source changes when migrating third-party libraries that hard-code qualified calls such as `std::sin(float)`. After replacing their scalar type, existing qualified calls could continue to compile without editing every call site to use ADL.

This limitation must be explicit: the C++ standard generally does not permit users to add new function overloads to `namespace std`, even when an argument is a user-defined type. Enabling the macro may result in undefined behavior, implementation differences, overload ambiguity, or conflicts after a standard-library upgrade. Therefore:

- Default builds must not add math function overloads to `std`.
- This capability must remain separate from the standard-permitted `numeric_limits` and `common_type` customizations and must be labeled as a non-standard compatibility option in headers and release notes.
- Forwarding overloads must not introduce a second set of math semantics. Results, special values, and rounding must match the formal functions such as `fixmath::sin`.
- Users should prefer converting code they control to the ADL pattern. This option is intended only for third-party code that cannot be changed economically, and each target compiler and standard-library combination must be tested.
- The macro must be defined consistently before including any Fixmath header in every translation unit, avoiding different declaration sets that could cause ODR violations or inconsistent overload resolution.

This section records a future compatibility direction. The current code does not provide `std::sin(fixed)` or the proposed macro.
