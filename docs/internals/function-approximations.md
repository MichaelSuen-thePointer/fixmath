# Elementary Function Approximation Transforms

This document records the compact mathematical transform used before generating polynomial coefficients. Each function entry uses the same fields so more functions can be added without repeating the minimax algorithm described in [Offline Minimax Approximation Tool](minimax-approximation.md).

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
