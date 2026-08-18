# Power-of-two Division and Rounding

## Purpose

Fixed-point multiplication produces twice as many fractional bits as the target format. Returning to the original scale requires division by

```text
RATIO = 2^N
```

Fixmath implements this operation with the `_fm_div2n_round` overloads. They divide a one-word or two-word integer by a power of two and apply the selected policy:

- `RoundToZero`: truncate the exact quotient toward zero;
- `RoundToEven`: choose the nearest integer, resolving an exact midpoint toward the even result.

The power-of-two divisor allows the implementation to use shifts and discarded-bit tests instead of a general integer division in the rounding path.

## Why a bare signed right shift is not division toward zero

For a negative signed integer, an arithmetic right shift rounds downward, while C++ signed division rounds toward zero. The smallest example is

```text
-1 >> 1 = -1
-1 / 2  =  0
```

The difference is fundamental:

```text
arithmetic shift: floor(a / 2^N)
signed division:  trunc(a / 2^N)
```

Therefore, `_fm_div2n_round` does not use a bare arithmetic shift for `RoundToZero`. The one-word overload uses `/`, and the signed two-word overload first adds a negative-value bias before shifting.

Surprisingly, an arithmetic right shift is exactly the useful starting point for `RoundToEven`. The reason is that nearest rounding is naturally expressed from the lower adjacent integer, which is the floor quotient.

## Floor quotient and discarded-bit remainder

Let

```text
d = 2^N
q = floor(a / d)
r = a - q * d
```

Euclidean division by a positive `d` gives

```text
a = q * d + r
0 <= r < d
```

and therefore

```text
a / d = q + r / d
```

Under the project's C++20 requirement, an arithmetic right shift of a negative signed integer computes `q`. Because `d` is a power of two, `r` is exactly the low `N` bits of the two's-complement representation of `a`:

```text
q = a >> N
r = unsigned(a) & (d - 1)
```

This remains true when `a` is negative. Casting to the corresponding unsigned type preserves the bit pattern, and reduction modulo `2^N` produces the same nonnegative Euclidean remainder.

The implementation extracts these bits without depending on a full-width mask expression:

```cpp
const UT frac = UT(a) << (bits - N) >> (bits - N);
```

After extraction, `frac` lies in `[0, d)` and represents the distance from the floor result `q` toward the next integer `q + 1`.

## Nearest, ties-to-even from the floor result

Let

```text
half = d / 2
```

Only three cases are needed:

```text
r < half: return q
r > half: return q + 1
r = half: return q if q is even, otherwise q + 1
```

This rule does not depend on the sign of `a`. `q` is always the lower adjacent integer, and `q + 1` is always the upper adjacent integer. For a negative exact quotient, moving from `q` to `q + 1` moves toward zero; for a positive quotient, it moves away from zero. Both are simply movements toward the nearer integer.

The one-word implementation is therefore

```text
frac = low N bits of a
q = a >> N

if frac > half:
    q += 1
else if frac == half and q is odd:
    q += 1
```

The same single positive correction handles positive values, negative values, and midpoint ties.

## The `-1 >> 1` example

For `a = -1`, `N = 1`, and `d = 2`:

```text
q = -1 >> 1 = -1
r = low 1 bit of -1 = 1
half = 1
```

The exact quotient is `-0.5`. This is a midpoint, and `q = -1` is odd, so the helper adds one:

```text
rounded = q + 1 = 0
```

Thus the arithmetic shift by itself would be wrong for truncation toward zero, but the shift plus ties-to-even correction gives the correct rounded result.

The neighboring negative midpoint demonstrates the other tie direction:

```text
a = -3, d = 2
q = -2
r = 1
```

The exact quotient is `-1.5`. Since the floor candidate `-2` is already even, no increment is applied and the result remains `-2`.

## More signed examples

For `N = 2`, `d = 4`, and `half = 2`:

| `a` | Exact `a / 4` | Shifted `q` | Low bits `r` | Rounded result |
| ---: | ---: | ---: | ---: | ---: |
| `7` | `1.75` | `1` | `3` | `2` |
| `6` | `1.5` | `1` | `2` | `2` |
| `5` | `1.25` | `1` | `1` | `1` |
| `-5` | `-1.25` | `-2` | `3` | `-1` |
| `-6` | `-1.5` | `-2` | `2` | `-2` |
| `-7` | `-1.75` | `-2` | `1` | `-2` |

Notice that the negative remainders are not the signed `%` remainders used by truncating division. They are nonnegative distances measured from the floor quotient.

## One-word overloads

The compile-time overload

```cpp
_fm_div2n_round<policy, N>(a)
```

and the runtime-count overload

```cpp
_fm_div2n_round<policy>(a, n)
```

share the same structure.

For `RoundToEven` they:

1. cast to the corresponding unsigned type;
2. extract the low `N` discarded bits;
3. arithmetic-shift the signed value, or logical-shift an unsigned value;
4. add one when the discarded fraction is above half;
5. at half, add one only when the shifted quotient is odd.

For `RoundToZero` they use

```text
a / 2^N
```

instead. The compiler may lower the constant power-of-two division to shifts and bias operations, but the source-level `/` preserves the required signed truncation semantics.

## Two-word signed overload

The signed wide overload treats `(rhi, rlo)` as one two's-complement 128-bit value. For `RoundToEven`, it extracts the low `N` bits from `rlo` and performs a cross-limb arithmetic right shift:

```text
new_lo = (unsigned(rlo) >> N) | (unsigned(rhi) << (64 - N))
new_hi = rhi >> N
```

The logical shift of `rlo` avoids copying its sign bit into the middle of the combined value. Bits shifted out of `rhi` enter the top of `new_lo`, while the arithmetic shift of `rhi` preserves the sign extension of the complete 128-bit integer.

## C++20 signed left-shift semantics

C++20 `[expr.shift]` defines

```text
E1 << E2
```

as the unique result congruent to `E1 * 2^E2` modulo `2^W`, where `W` is the width of the promoted left operand's type. Vacated bits are zero-filled. This definition applies even when `E1` has a signed type, is negative, or the mathematical product is outside the signed range.

The shift operation is undefined only when the right operand is negative or is greater than or equal to the width of the promoted left operand. This does not make general signed overflow defined: spelling the same calculation as signed multiplication may still overflow. The modulo rule is specific to `<<`.

Consequently, the current high-to-low transfer

```text
rhi << (64 - N)
```

is defined under the project's C++20 requirement even when `rhi` is negative. The two-word overload requires `0 < N < 64`, so its shift count `64 - N` is always between 1 and 63. When the shifted signed result participates in the bitwise OR with the unsigned low-limb expression, conversion to `uint64_t` preserves the same modulo-`2^64` bit pattern.

An explicit `uint64_t(rhi)` cast before the left shift would express the bit-transfer intent and retain the same C++20 result, but it is not required to avoid undefined behavior. This differs from C++17 and earlier wording, where left-shifting a negative signed value was undefined.

The resulting pair is the 128-bit floor quotient. The discarded low bits are again its nonnegative Euclidean remainder modulo `2^N`. When rounding requires the upper adjacent integer, `_fm_inc128` increments the complete `(high, low)` pair so a carry out of `rlo` reaches `rhi`.

## Two-word truncation toward zero

The signed two-word `RoundToZero` branch cannot use the floor shift directly. Before shifting a negative value, it adds

```text
2^N - 1
```

to the complete 128-bit value. Nonnegative values receive a zero bias. The identity is

```text
trunc(a / 2^N) = floor((a + 2^N - 1) / 2^N), for a < 0
```

The code constructs the bias from the sign of `rhi`, adds it to `rlo`, propagates the carry into `rhi`, and then performs the same cross-limb arithmetic shift. This converts the floor behavior of the shift into truncation toward zero.

For example:

```text
a = -1, N = 1
bias = 1
floor((-1 + 1) / 2) = 0
```

The unsigned two-word overload needs no bias because unsigned floor division and truncation are identical.

## Tie parity and negative integers

At a midpoint, the helper tests the low bit of the floor quotient:

```text
q & 1
```

This parity test works for negative two's-complement integers exactly as it does for positive integers: even values have a zero low bit and odd values have a one low bit. Adding one to an odd negative floor value selects the other, even midpoint neighbor.

Examples:

```text
-1 is odd  -> midpoint -0.5 rounds to  0
-2 is even -> midpoint -1.5 rounds to -2
-3 is odd  -> midpoint -2.5 rounds to -2
```

## Overflow and range handling

`_fm_div2n_round` performs scaling and rounding, not public arithmetic-mode handling. Its callers select intermediate widths and later check whether the rounded result fits the target raw representation.

The template constraints prevent shift counts from touching unsupported sign-bit boundaries, and the two-word overload requires `0 < N < 64`. When round-to-even increments the largest representable intermediate, the carry remains visible in the returned high limb so the caller can detect overflow rather than silently losing it.

## Testing expectations

Tests should cover:

- positive and negative exact multiples of `2^N`;
- discarded fractions immediately below and above half;
- positive and negative midpoint cases with both even and odd floor quotients;
- `-1` for several shift counts;
- the most-negative supported intermediate values;
- carry from the low limb into the high limb after rounding;
- equivalence between one-word and two-word overloads where both can represent the same input;
- comparison with an independent exact rational oracle for both rounding policies.

The central invariant for `RoundToEven` is that the shift produces the floor candidate and the discarded low bits measure the distance to the upper candidate. Testing this invariant directly is more diagnostic than checking only final multiplication results.

## References

- [C++20 working draft N4861: shift operators `[expr.shift]`](https://timsong-cpp.github.io/cppwp/n4861/expr.shift)
- [Current C++ working draft: shift operators `[expr.shift]`](https://eel.is/c++draft/expr.shift)
