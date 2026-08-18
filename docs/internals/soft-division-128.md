# Software 128-bit Division

## Purpose

Fixed-point division with a 64-bit `raw_t` may need to evaluate

```text
(A * 2^N) / B
```

when the shifted dividend no longer fits in 64 bits. Fixmath represents that dividend as two 64-bit limbs and divides it by one 64-bit limb. The implementation is split into two layers:

```text
_fm_div128       signed 128-by-64 wrapper
    |
    +-- divq     Linux x64 low-limb backend
    +-- _udiv128 optional MSVC x64 backend
    `-- _softudiv128 portable unsigned backend
```

`_fm_div128` handles signs and a quotient wider than 64 bits. `_softudiv128` computes only the low 64 quotient bits after its caller has reduced the upper dividend limb below the divisor.

These functions implement integer division truncated toward zero. They do not apply the Fixmath rounding policy. The fixed-point caller uses the returned remainder later to implement `RoundToEven`, or ignores it for `RoundToZero`.

## Representation and contracts

The signed wrapper receives

```cpp
_fm_div128(int64_t dhi, int64_t dlo, int64_t d, int64_t& rem)
```

where the signed 128-bit dividend is represented by the two's-complement pair `(dhi, dlo)`:

```text
D = signed128(dhi:dlo)
V = d
D = Q * V + R
```

It returns `Q` as `_int128_s { lo, hi }` and stores `R` in `rem`. For a nonzero divisor, the intended signed result satisfies

```text
abs(R) < abs(V)
sign(R) = sign(D), unless R = 0
Q = trunc(D / V)
```

The unsigned core receives

```cpp
_softudiv128(uint64_t u1, uint64_t u0, uint64_t v, uint64_t* r)
```

and computes

```text
U = u1 * 2^64 + u0
q = floor(U / v)
r = U - q * v
```

under three hard preconditions:

- `v != 0`;
- `u1 < v`, so `q < 2^64`;
- `r` points to writable storage.

The public fixed-point division paths reject a zero divisor before calling this layer. `_fm_div128` establishes `u1 < v` through a preliminary division of the high limb.

## Signed wrapper: `_fm_div128`

### 1. Convert to unsigned magnitudes

The wrapper first reinterprets both dividend limbs and the divisor as unsigned values. If the signed dividend is negative, `_fm_neg128` applies two's-complement negation to the complete `(high, low)` pair:

```text
(udhi:udlo) = abs(dhi:dlo)
ud = abs(d)
```

Negating the two unsigned limbs avoids signed overflow, including the most-negative representable values. The helper negates the low limb and complements or negates the high limb according to whether the low-limb negation generated a carry.

The signs are not discarded. The wrapper retains the original `dhi` and `d` to restore the quotient and remainder signs later.

### 2. Extract the high quotient limb

The magnitude division is decomposed algebraically:

```text
udhi = uqhi * ud + urem

(udhi * 2^64 + udlo) / ud
    = uqhi * 2^64 + (urem * 2^64 + udlo) / ud
```

The wrapper therefore computes

```text
uqhi = udhi / ud
urem = udhi % ud
```

when `udhi >= ud`. Otherwise, `uqhi` is zero and `urem` is `udhi`. In either case,

```text
urem < ud
```

which is exactly the precondition required by the low-limb backend.

### 3. Compute the low quotient limb

The remaining division is

```text
uqlo = (urem * 2^64 + udlo) / ud
```

with a quotient known to fit in 64 bits. The selected backend also replaces `urem` with the final unsigned remainder.

Current dispatch is:

- Linux x64 uses the hardware `divq` instruction through inline assembly.
- MSVC x64 may use `_udiv128` when `FIXMATH_HAS_INTRIN_DIV` is enabled by the build.
- All other configurations use `_softudiv128`.

The dispatch changes performance only. All backends share the same unsigned quotient-and-remainder contract.

### 4. Restore signed semantics

If dividend and divisor signs differ, `_fm_neg128` negates the complete quotient pair. If the dividend was negative, the unsigned remainder is negated independently. This produces C++-style truncation toward zero with a remainder carrying the dividend's sign.

The fixed-point caller later checks whether `qhi` is the sign extension of `qlo`. If not, the quotient does not fit in the target 64-bit raw representation and the selected arithmetic policy handles the overflow.

## Unsigned core: `_softudiv128`

The portable core is a specialized base-`2^32` long division. It uses only 64-bit unsigned arithmetic, count-leading-zeros, shifts, multiplication, division, and remainder. Because the quotient is known to fit in 64 bits, it consists of exactly two base-`2^32` digits:

```text
B = 2^32
q = q1 * B + q0
```

The algorithm estimates and corrects `q1`, forms a partial remainder, and then repeats the same process for `q0`.

### 1. Normalize the divisor

Let

```text
s = clz(v)
v_normalized = v << s
```

After normalization, the most-significant bit of `v_normalized` is set. Splitting it into base-`B` digits gives

```text
v_normalized = vn1 * B + vn0
vn1 >= B / 2
```

The large leading digit makes the quotient estimates accurate enough to need at most two downward corrections.

The dividend is shifted left by the same amount without requiring a native 128-bit type:

```text
un64 = (u1 << s) | (u0 >> (64 - s))
un10 = u0 << s
un1  = un10 >> 32
un0  = un10 & (B - 1)
```

When `s == 0`, the implementation uses a separate branch. This is necessary because evaluating `u0 >> 64` would be undefined in C++ even though its mathematical contribution should be zero.

### 2. Estimate and correct the high digit `q1`

The first estimate divides the leading normalized partial dividend by the leading divisor digit:

```text
q1   = un64 / vn1
rhat = un64 - q1 * vn1
```

This estimate considers only `vn1`, so it may be slightly too large when the low divisor digit `vn0` is taken into account. The correction condition is

```text
q1 >= B
    or
q1 * vn0 > B * rhat + un1
```

While either condition holds, the implementation decrements `q1` and adds `vn1` back to `rhat`. Normalization guarantees that no more than two decrements are required.

The first half of the condition ensures `q1` is a valid base-`B` digit. The second verifies that multiplying by the complete divisor does not exceed the three-digit dividend prefix. Short-circuit evaluation also prevents `B * rhat + un1` from being evaluated in the case where the initial estimate is `B`.

After correction, the partial remainder for the lower digit is

```text
un21 = un64 * B + un1 - q1 * v_normalized
```

The expression is evaluated with unsigned 64-bit arithmetic. Intermediate wraparound is defined modulo `2^64`; after the corrected quotient digit is subtracted, the retained low limb is the required partial remainder.

### 3. Estimate and correct the low digit `q0`

The second digit repeats the same calculation using `un21` and the last dividend digit `un0`:

```text
q0   = un21 / vn1
rhat = un21 - q0 * vn1
```

It applies the corresponding correction test

```text
q0 >= B
    or
q0 * vn0 > B * rhat + un0
```

and decrements `q0` as required. Once corrected, both `q1` and `q0` lie in `[0, B)`, so combining them cannot overflow 64 bits:

```text
q = q1 * B + q0
```

### 4. Recover the remainder

The normalized residual after subtracting the complete low quotient digit is

```text
r_normalized = un21 * B + un0 - q0 * v_normalized
```

Because both dividend and divisor were shifted by `s`, the original remainder is recovered with

```text
r = r_normalized >> s
```

For valid inputs, the final result satisfies `r < v`. The function writes this value through the pointer and returns the combined quotient.

## Why two correction iterations are enough

Normalization makes the leading divisor digit at least half the radix: `vn1 >= B / 2`. Estimating a quotient digit from the leading divisor digit can then overestimate the true digit by at most two. Each loop iteration reduces the estimate by one while restoring the corresponding leading-divisor contribution to `rhat`, so the loop terminates after at most two corrections.

This bounded correction is why the routine is substantially faster than bit-at-a-time long division: it produces 32 quotient bits per estimation step and needs exactly two estimation phases for a 64-bit quotient.

## Relationship to fixed-point rounding

For fixed-point division, `_fm_div128` returns the truncated wide quotient and the signed remainder. `operator/` then computes

```text
2 * abs(remainder) compared with abs(divisor)
```

to implement nearest, ties-to-even rounding. At an exact midpoint, it examines the low quotient bit. Any rounding carry is added to the complete `(qhi, qlo)` pair before range checks.

Keeping rounding outside the software division routine is intentional: the same quotient-and-remainder primitive supports both rounding modes and keeps platform backends bit-for-bit comparable.

## Testing expectations

Tests for this layer should cover:

- `u1 == 0`, `u1 == v - 1`, and normalized divisors with `s == 0`;
- divisors with many leading zeros, exercising large normalization shifts;
- cases requiring zero, one, and two estimate corrections for both quotient digits;
- positive and negative dividends and divisors, including signed minima;
- zero and nonzero remainders with the dividend's sign;
- quotients that fit 64 bits and full quotients requiring a nonzero high limb;
- equivalence among hardware, intrinsic, and software backends where available;
- the invariant `D == Q * V + R` and `abs(R) < abs(V)` using an independent wide-integer oracle.

Division by zero is tested at the fixed-point operator boundary, not as a valid input to `_softudiv128`.
