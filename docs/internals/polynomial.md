# Fixed-point Polynomial Evaluation

This derivation assumes that the input and every coefficient use the same Fixmath format. Let

```text
S = RATIO = 2^N
X = raw(x) = Sx
A_i = raw(a_i) = Sa_i
P(x) = a_n x^n + ... + a_1 x + a_0
Y = raw(P(x)) = S P(x)
```

The distinction between a polynomial Horner chain and a sum of independent products is important. A single final division by `S` is valid for the latter, but not in general for the former.

## Direct raw-form derivation

Substituting `x = X / S` and `a_i = A_i / S` gives

```text
Y = S * sum(i = 0..n, a_i x^i)
  = sum(i = 0..n, A_i X^i / S^i)
```

The denominator depends on the degree of each term. Moving all terms to one common denominator gives

```text
Y = (A_n X^n
   + A_(n-1) X^(n-1) S
   + ...
   + A_1 X S^(n-1)
   + A_0 S^n) / S^n
```

Therefore, delaying all normalization is mathematically possible, but the final divisor is `S^n`, not `S`.

For a quadratic polynomial,

```text
P(x) = a_2 x^2 + a_1 x + a_0

Y = (A_2 X^2 + A_1 X S + A_0 S^2) / S^2
```

Dividing that numerator only by `S` would leave one extra factor of `S` and produce a result in the wrong scale.

## Same-scale Horner evaluation

Horner's method is

```text
P(x) = (...((a_n x + a_(n-1))x + a_(n-2))x + ...)x + a_0
```

If every intermediate `H` remains in the normal raw scale `S`, one fused stage is

```text
H_next = round((H * X + A_i * S) / S)
```

`H * X` has scale `S^2`, so the coefficient must be promoted from `A_i` to `A_i * S` before addition. The final division restores the stage result to scale `S`, making it suitable as the input to the next multiplication.

Consequently, a degree-`n` Horner polynomial needs one `/S` normalization per multiplication stage. Combining multiplication and addition into a fixed-point FMA avoids an unnecessary intermediate rounding between them, but it does not remove the normalization required between consecutive Horner stages.

For the quadratic example:

```text
H_1 = round((A_2 * X + A_1 * S) / S)
Y   = round((H_1 * X + A_0 * S) / S)
```

## Fully deferred Horner numerator

Normalization and rounding can instead be deferred until the end by allowing the accumulator scale to grow:

```text
T_1 = A_n X + A_(n-1) S
T_2 = T_1 X + A_(n-2) S^2
...
T_n = T_(n-1) X + A_0 S^n
Y   = round(T_n / S^n)
```

This performs only one final rounding, which can be more accurate than rounding after every Horner stage. Its practical cost is the rapidly growing accumulator: before the final division, the leading term contains `A_n X^n`, requiring approximately `(n + 1)` raw widths for unrestricted operands. A 128-bit intermediate is therefore not sufficient for an unrestricted high-degree polynomial over 64-bit raw values.

The powers `S`, `S^2`, and so on used to align later coefficients must also be representable in the chosen accumulator. Range analysis can make the deferred form viable for bounded inputs and coefficients, but it cannot be assumed safe from the fixed type alone.

## When one final `/S` is valid

A single final division by `S` is valid for a fused sum of products whose factors are all already represented at scale `S`:

```text
Q = c + sum(j = 1..m, a_j b_j)

raw(Q) = round((C * S + sum(j = 1..m, A_j B_j)) / S)
```

Every product `A_j B_j` has the same scale `S^2`, so they can share one wide accumulator and one final `/S`. This is a fixed-point dot product or multi-add operation.

A polynomial can use this form only if each power has already been normalized to the ordinary raw scale:

```text
B_i = raw(x^i) approximately S x^i
Y   = round((A_0 S + A_1 B_1 + ... + A_n B_n) / S)
```

The final accumulation then needs only one `/S`, but generating `B_2`, ..., `B_n` still requires normalization after power multiplications, an independently scaled power algorithm, or a sufficiently wide deferred calculation. The scaling work has been moved; it has not disappeared.

## Implementation consequences

- A same-scale fixed-point FMA should compute `round((A * B + C * S) / S)` with one range check and one rounding step. In Fixmath, the final normalization can reuse `_fm_div2n_round<policy, fixed::FRACTION_BITS>` instead of introducing separate polynomial rounding logic.
- For a 32-bit `raw_t`, `A * B + C * S` can be held in a signed 64-bit intermediate and passed to the single-value `_fm_div2n_round` overload. For a 64-bit `raw_t`, the multiply-add must remain in a signed 128-bit `(high, low)` accumulator and use the corresponding two-word overload.
- After `_fm_div2n_round` restores scale `S`, the result can become `H` for the next Horner stage. The helper must therefore be called once per multiplication stage, not only after the entire polynomial.
- A chained dot product may accumulate multiple `A_i * B_i` terms and divide by `S` once if the wide accumulator is proven not to overflow.
- A general same-scale Horner polynomial cannot divide by `S` only once at the end. It must either normalize each stage or retain a growing numerator and finally divide by `S^n`.
- Round-to-even must inspect the remainder only at the chosen normalization points. Deferring normalization changes the result relative to stage-by-stage rounding because it performs one rounding instead of `n` roundings.
- Strict-mode `nan` and `inf` raw patterns must be handled before entering an integer multiply-accumulate core; they cannot be treated as ordinary coefficients.
