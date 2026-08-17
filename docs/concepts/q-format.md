# Q M:N Format and Special Values

## Representation formula

Fixmath stores values in a signed two's-complement integer named `raw`. If the underlying integer has `B` total bits and the policy specifies `N` fractional bits, then:

```text
M = B - N
value = raw / 2^N
epsilon = 2^-N
```

This documentation uses the `Q M:N` notation, where `M` **includes the sign bit** and `N` is the number of fractional bits. For example, a 64-bit underlying integer with 32 fractional bits forms `Q32:32`. In ordinary modes, its mathematical range is `[-2^31, 2^31 - 2^-32]`.

The corresponding code constants are `ALL_BITS = B`, `INTEGER_BITS = M`, `FRACTION_BITS = N`, and `URATIO = 2^N`. A policy currently requires a signed underlying integer no wider than 64 bits and requires `1 <= N < B`.

`from_raw(r)` adopts the raw representation directly without scaling. Construction from an integer or floating-point value performs the conversion to `raw`. This makes `from_raw` useful for serialization, protocols, and exact tests, but callers must ensure that the bit pattern has valid semantics under the selected mode.

## Range in ordinary modes

In `Ignore` and `SaturationMode`, no raw bit pattern is interpreted as a genuinely non-finite value:

| Raw value | Mathematical value |
| --- | --- |
| `INT_MIN` | `-2^(M-1)` |
| `0` | `0` |
| `INT_MAX` | `2^(M-1) - 2^-N` |

The class still provides `nan()` and `inf()` factories so generic implementations can share an interface, but `is_nan()` and `is_inf()` always return `false` in these two modes. Their raw bit patterns remain ordinary finite or saturation values and must not be treated as IEEE special values.

## Special values in strict mode

`StrictMode` reserves three patterns at the ends of the raw integer range:

| Raw value | Meaning |
| --- | --- |
| `INT_MIN` | `nan` |
| `INT_MIN + 1` (that is, `-INT_MAX`) | `-inf` |
| `INT_MAX` | `+inf` |

The finite raw range in strict mode is therefore `[INT_MIN + 2, INT_MAX - 1]`, reduced at both ends compared with the ordinary Q format. `max_fix()` and `min_fix()` return the finite boundaries, `inf()` returns positive infinity, unary minus applied to `inf()` produces negative infinity, and `nan()` returns the non-number value.

Strict-mode comparisons use `std::partial_ordering`: any comparison involving `nan` is unordered. Conversion to `float` or `double` maps special values to the corresponding IEEE-754 NaN or signed infinity.

Borrowing these integer encodings does not make Fixmath an IEEE-754 implementation. It has no negative zero, subnormal values, signaling NaN, or NaN payload.
