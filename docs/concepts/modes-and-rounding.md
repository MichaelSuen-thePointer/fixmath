# Arithmetic and Rounding Modes

## Arithmetic modes

The arithmetic mode determines the result semantics for overflow, non-finite values, and invalid inputs.

### `arithmetic_mode::Ignore`

Addition and subtraction operate through the corresponding unsigned type, producing modulo-underlying-width results. Multiplication and division likewise retain only the target width without saturation checks. This is the lowest-overhead mode, and callers are responsible for proving that unwanted wraparound cannot occur.

Division by zero has no useful modular result. The implementation triggers the configurable diagnostic and returns the raw pattern produced by `fixed::nan()`. Because `is_nan()` always returns `false` in Ignore mode, this is only a sentinel value, not a classifiable NaN.

### `arithmetic_mode::SaturationMode`

Finite results outside the representable range are clamped to `min_sat()` or `max_sat()`. Division by zero also triggers the configurable diagnostic: positive and negative numerators saturate to the respective boundaries, while `0 / 0` returns the raw boundary pattern associated with `fixed::nan()`. Taking the square root of a negative value returns the lower saturation boundary.

This mode has no genuine `inf` / `nan` classification. Boundary bit patterns remain finite values.

### `arithmetic_mode::StrictMode`

Strict mode reserves `nan`, `+inf`, and `-inf` and propagates them using rules similar to common floating-point behavior. For example, `inf + (-inf)`, `0 * inf`, `0 / 0`, and the square root of a negative value produce `nan`; dividing a finite nonzero value by zero produces a signed `inf`. Finite arithmetic overflow produces the infinity in the corresponding direction.

Strict mode still does not throw exceptions. Some invalid inputs also trigger configurable assertion diagnostics, so special values define numeric semantics but are not a guarantee that every debug build will continue executing.

## Rounding modes

### `rounding_mode::RoundToZero`

Bits beyond the target precision are discarded and the result is truncated toward zero. For multiplication and division, this matches the truncation direction of signed C++ integer division. For nonnegative `sqrt`, it is equivalent to selecting the lower adjacent representable value.

### `rounding_mode::RoundToEven`

The nearest representable value is selected. A result exactly halfway between two values selects the one whose raw integer has an even least-significant bit. This is also known as bankers' rounding. Multiplication examines the discarded fractional bits, division compares twice the absolute remainder against the absolute divisor, and `sqrt` generates one more bit to determine distance and midpoint status.

Round-to-even avoids a fixed directional bias at midpoints, at the cost of additional remainder or low-bit calculations.

## Operations that round

- Addition and subtraction between values with the same format preserve scaling and require no rounding.
- Multiplication rounds when reducing a result from `2N` fractional bits to `N` bits.
- Division first scales the dividend by `2^N` and rounds when the quotient is not exact.
- `sqrt` rounds the exact root back to the same Q format.
- Floating-point construction follows the policy's rounding mode. Integer construction is an exact scaling operation unless the input lies outside the representable range.

The rounding policy does not define range handling. Any overflow after rounding is still handled by the arithmetic mode.
