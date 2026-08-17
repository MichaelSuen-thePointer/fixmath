# Internal `sqrt`

## Scaling target

If the input raw value is `A` and the format has `N` fractional bits, its mathematical value is `A / 2^N`. The result uses the same Q format, so the desired raw result is:

```text
R = sqrt(A / 2^N) * 2^N = sqrt(A * 2^N)
```

The implementation does not materialize `A * 2^N` as a potentially overflowing wide integer. Instead, it consumes the input bits in pairs and continues with zero pairs to generate exactly the required number of root bits.

## Digit-by-digit integer square root

The algorithm is a binary restoring square root:

1. Starting at the most significant end, shift two bits of the radicand into `remainder` at a time.
2. Shift `root` left by one and form the trial subtrahend `(root << 1) | 1`.
3. If the trial value is no greater than the remainder, set the current root bit and subtract the trial value. Otherwise, leave the bit clear.
4. Repeat until all integer and target fractional root bits have been generated.

A count-leading-zeros operation skips insignificant leading zero pairs. When `N` is odd, the input is first shifted left by one so the algorithm can continue processing pairs without changing the final scaling relationship. The main loop consumes the original significant bits, and a second loop shifts zero pairs into the remainder to generate the additional `N / 2` root bits.

The entire calculation uses only the underlying unsigned integer, shifts, comparisons, and subtraction. It does not depend on floating-point `sqrt`, so the result is independent of the platform floating-point implementation and environment.

## Rounding

`RoundToZero` returns the generated root directly. For a nonnegative input, this is downward truncation.

`RoundToEven` performs one additional digit-by-digit step to obtain an extra root bit and remainder:

- If the extra bit is zero, keep the result.
- If the extra bit is one and further remainder remains, increment the result.
- At an exact midpoint, increment only when the current result is odd.

This matches the nearest, ties-to-even semantics used by multiplication and division.

## Invalid values

- Strict mode: `sqrt(nan) = nan`, `sqrt(+inf) = +inf`, and a negative input triggers the configurable diagnostic and returns `nan`.
- Saturation mode: a negative input triggers the configurable diagnostic and returns `min_sat()`.
- Ignore mode: negative inputs have no domain guard. Their raw patterns enter the algorithm as unsigned values and produce a deterministic bit-level result, but that result does not represent a real square root. Callers must reject negative values before calling the function.
- Zero returns zero directly.
