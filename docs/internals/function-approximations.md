# Elementary Function Approximation Transforms

This document is the repository record for reviewed approximation results. It captures the compact mathematical transform, selected raw coefficients, and verification evidence after local working specifications and generated artifacts have been discarded. Each function entry uses the same fields so more functions can be added without repeating the minimax algorithm described in [Offline Minimax Approximation Tool](minimax-approximation.md).

## Entry format

- **Core interval:** interval handled by the polynomial.
- **Structure:** symmetry or exact factorization imposed on the basis.
- **Fit variable:** variable presented to the coefficient generator.
- **Fit target:** function approximated in that variable.
- **Polynomial:** generated polynomial and coefficient order.
- **Reconstruction:** conversion from the polynomial result to the function result.
- **Exact properties:** identities preserved independently of coefficient error.
- **Recorded candidates:** reviewed coefficient sets together with their format and verification level.

## `sin`

- **Core interval:** `x in [0, pi/4]`.
- **Structure:** exploit odd symmetry by factoring out `x`.
- **Fit variable:** `z = x^2`.
- **Fit target:** `g(z) = sin(sqrt(z)) / sqrt(z)`, with `g(0) = 1` defined by continuity.
- **Polynomial:** `q(z) = a_0 + a_1 z + ... + a_(m-1) z^(m-1)`; coefficients are generated for consecutive powers of `z`.
- **Reconstruction:** `sin(x) ~= x * q(x^2)`.
- **Exact properties:** the reconstructed polynomial contains only powers `x^1, x^3, ..., x^(2m-1)`, is odd, and returns exactly zero at `x = 0`.

The transform removes the zero of `sin(x)` from the fitted target and encodes the odd-power restriction before Chebyshev and Remez fitting. Coefficients are quantized in ascending `q(z)` power order and emitted in descending Horner order.

### Recorded candidate: Q32.32, five terms

- **Basis:** `x^1, x^3, x^5, x^7, x^9`.
- **Fit objective:** direct absolute-error Remez refinement of the reconstructed odd polynomial over the core interval.
- **Policy:** signed Q32.32 coefficients with round-to-nearest, ties-to-even evaluation.
- **Raw coefficients by ascending power:** `x^1 = 4294967296`, `x^3 = -715827879`, `x^5 = 35791363`, `x^7 = -852064`, `x^9 = 11654`.
- **Raw Horner order for `q(x^2)`:** `[11654, -852064, 35791363, -715827879, 4294967296]`.
- **Measured error:** maximum sampled continuous error `0.0655686 ulp`; maximum exact-evaluator error `1 ulp` over 1,000,001 uniformly spaced raw inputs.
- **Verification level:** `sampled`; this candidate is not yet an exhaustive or interval-bounded whole-domain result.

## `cos`

- **Core interval:** `x in [0, pi/4]`.
- **Structure:** exploit even symmetry and fix the constant term to one.
- **Fit variable:** `z = x^2`.
- **Fit target:** `g(z) = cos(sqrt(z))`.
- **Polynomial:** `q(z) = 1 + a_1 z + ... + a_(m-1) z^(m-1)`; the remaining coefficients use consecutive powers of `z`.
- **Reconstruction:** `cos(x) ~= q(x^2)`.
- **Exact properties:** the reconstructed polynomial contains only powers `x^0, x^2, ..., x^(2m-2)`, is even, and returns exactly one at `x = 0`.

Fixing the constant term before Remez refinement prevents coefficient optimization from trading away the exact identity `cos(0) = 1`. Only the non-constant coefficients are quantized and refined on the integer lattice.

### Recorded candidate: Q32.32, five terms

- **Basis:** `x^0, x^2, x^4, x^6, x^8`, with the `x^0` coefficient fixed to one.
- **Fit objective:** direct absolute-error Remez refinement of the reconstructed even polynomial over the core interval.
- **Policy:** signed Q32.32 coefficients with round-to-nearest, ties-to-even evaluation.
- **Raw coefficients by ascending power:** `x^0 = 4294967296`, `x^2 = -2147483636`, `x^4 = 178956784`, `x^6 = -5964319`, `x^8 = 104756`.
- **Raw Horner order for `q(x^2)`:** `[104756, -5964319, 178956784, -2147483636, 4294967296]`.
- **Measured error:** maximum sampled continuous error `0.2763795 ulp`; maximum exact-evaluator error `1 ulp` over 1,000,001 uniformly spaced raw inputs; `cos(0)` evaluates to raw `4294967296` exactly.
- **Minimum-term check:** the tested four-term basis through `x^6` reached `139 ulp`, so five terms are the minimum sampled candidate among the tested sizes.
- **Verification level:** `sampled`; an independent 319,990-input cross-check and continuous stationary-point search agreed, but this candidate is not yet an exhaustive or interval-bounded whole-domain result.

## `tan`

The planned Q32.32 implementation uses a deliberately weak accuracy contract. Only the two polynomial kernels below are sampled to at most `1 ulp` under their stated Q32.32 evaluators. Argument reduction, the two rational reflection reconstructions, the reciprocal, and the final reciprocal-plus-residual reconstruction do not currently carry a `1 ulp` guarantee. Consequently, this entry must not be read as a whole-domain `tan` accuracy claim.

After periodic and sign reduction, let `a` be nonnegative and lie in `[0, pi/2]`. The first-quadrant implementation is split into four intervals:

| Interval | Reduced argument | Planned reconstruction |
| --- | --- | --- |
| `a in [0, 0.46]` | `u = a` | `T(u)` |
| `a in (0.46, pi/4]` | `u = pi/4 - a` | `(1 - T(u)) / (1 + T(u))` |
| `a in (pi/4, pi/2 - 0.46)` | `u = a - pi/4` | `(1 + T(u)) / (1 - T(u))` |
| `a in [pi/2 - 0.46, pi/2)` | `d = pi/2 - a` | `1/d + C(d)` |

Here `T` is the direct tangent kernel and `C` is the cotangent residual kernel. The exact pole at `a = pi/2` is handled separately according to the arithmetic policy. Returning exactly one for `a = pi/4` is permitted as an exact special case. For Q32.32, the intended downward-safe raw boundaries are `0.46 -> 1975684956`, `pi/4 -> 3373259426`, `pi/2 - 0.46 -> 4770833896`, and `pi/2 -> 6746518852`.

The direct tangent and cotangent-residual intervals are symmetric under `a -> pi/2 - a`: both kernels receive an argument in `[0, 0.46]`. The two middle reflection intervals are likewise symmetric around `pi/4` and both map to `u in [0, pi/4 - 0.46]`. They evaluate only one tangent polynomial. The reflection path should retain one guard bit from the final `u * q(u^2)` product before evaluating the rational identity. This guard bit improved the sampled reflection result, but the weak contract intentionally does not promote that observation to a guaranteed final error bound.

The fourth interval separates the cotangent pole:

```text
cot(d) = 1/d + C(d)
C(d) = cot(d) - 1/d ~= d * q_c(d^2)
```

`C` is smooth at zero, with `C(0) = 0` and `q_c(0) = -1/3`. The planned path performs one reciprocal, one residual polynomial, and one final addition; it does not evaluate the tangent kernel and then take its reciprocal. Ordinary Q32.32 reciprocal and final-add rounding, as well as loss in a Q32.32-only `pi/2 - a` reduction, remain outside the kernel guarantee.

### Tangent kernel candidate: Q32.32, six terms

- **Core interval:** `u in [0, 0.46]`.
- **Structure:** odd factorization `T(u) = u * q_t(u^2)`.
- **Basis:** `u^1, u^3, u^5, u^7, u^9, u^11`.
- **Fit target:** `tan(sqrt(z)) / sqrt(z)`, with value one at `z = 0`.
- **Policy:** signed Q32.32 coefficients with round-to-nearest, ties-to-even stage-by-stage evaluation.
- **Raw coefficients by ascending power:** `u^1 = 4294967295`, `u^3 = 1431656075`, `u^5 = 572645510`, `u^7 = 232123842`, `u^9 = 90993159`, `u^11 = 49715989`.
- **Raw Horner order for `q_t(u^2)`:** `[49715989, 90993159, 232123842, 572645510, 1431656075, 4294967295]`.
- **Measured kernel error:** maximum sampled continuous error `0.41500238 ulp`; maximum exact-evaluator error `1 ulp` over 1,000,001 uniformly spaced raw inputs plus extrema neighborhoods.
- **Intermediate range:** largest observed signed numerator width `66 bits`; no evaluator overflow was observed.
- **Selection note:** a five-term kernel cannot cover a self-contained `pi/4` reflection split with the sampled target. At the minimum symmetric boundary `pi/8`, its direct kernel reached `4 ulp`, so six terms are retained as the minimum sampled candidate for this plan.

### Cotangent residual kernel candidate: Q32.32, four terms

- **Core interval:** `d in [0, 0.46]`.
- **Structure:** odd factorization `C(d) = d * q_c(d^2)`.
- **Basis:** `d^1, d^3, d^5, d^7`.
- **Fit target:** `(cot(sqrt(z)) - 1/sqrt(z)) / sqrt(z)`, with value `-1/3` at `z = 0` by continuity.
- **Policy:** signed Q32.32 coefficients with round-to-nearest, ties-to-even stage-by-stage evaluation.
- **Raw coefficients by ascending power:** `d^1 = -1431655764`, `d^3 = -95443945`, `d^5 = -9084519`, `d^7 = -949077`.
- **Raw Horner order for `q_c(d^2)`:** `[-949077, -9084519, -95443945, -1431655764]`.
- **Measured kernel error:** maximum sampled continuous error `0.673196427674 ulp`; maximum exact-evaluator error `1 ulp` over 1,000,001 uniformly spaced raw inputs plus extrema neighborhoods.
- **Intermediate range:** largest observed signed numerator width `64 bits`; no evaluator overflow was observed.
- **Minimum-term check:** the tested three-term basis through `d^5` reached `125 ulp`, so four terms are the minimum sampled candidate among the tested sizes.

### Accuracy boundary

The recorded `1 ulp` figures apply only to `T(u)` and `C(d)` as standalone fixed-point kernels. No stronger claim is currently made for:

- either `pi/4` rational reflection formula;
- `1/d + C(d)` after separately rounded reciprocal and addition;
- inputs whose mathematical tangent is outside the finite Q32.32 range;
- `pi/2` argument reduction performed without an additional tail;
- periodic whole-domain reduction; or
- policies and rounding modes other than the recorded Q32.32 RoundToEven evaluator.

All figures in this entry are sampled numerical evidence, not exhaustive or interval-certified proofs.
