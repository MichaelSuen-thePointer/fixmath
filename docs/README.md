# Fixmath Documentation

This directory documents Fixmath in layers: design constraints, data model, algorithm implementation, and ecosystem integration. The code remains the final authority on behavior; items marked as future plans are not part of the current API.

## Design

- [Design principles](design/principles.md): policy-based design, cross-platform consistency, exception-free behavior, and coding style.
- [Required C++20 features](design/cpp20-requirements.md): `std::bit_cast`, signed left-shift semantics, three-way comparison, and concepts.

## Concepts

- [Q M:N format and special values](concepts/q-format.md): raw integers, scaling, representable ranges, and `inf` / `nan` in strict mode.
- [Arithmetic and rounding modes](concepts/modes-and-rounding.md): `Ignore`, `SaturationMode`, `StrictMode`, `RoundToZero`, and `RoundToEven`.

## Internals

- [Basic arithmetic](internals/arithmetic.md): integer implementations of addition, subtraction, multiplication, and division, including overflow handling and fast paths.
- [Software 128-bit division](internals/soft-division-128.md): signed wrapper, normalized 128-by-64 unsigned division, quotient-digit correction, and platform dispatch.
- [Power-of-two division and rounding](internals/div2n-rounding.md): `_fm_div2n_round`, signed arithmetic shifts, discarded-bit remainders, and ties-to-even correction.
- [Offline minimax approximation tool](internals/minimax-approximation.md): local coefficient generator design and first implementation, including its dependencies, Chebyshev/Remez pipeline, raw-coefficient optimization, artifacts, and verification.
- [Elementary function approximation transforms](internals/function-approximations.md): concise, reusable records of the variable transforms, polynomial structures, reconstruction formulas, and exact identities used for coefficient generation.
- [Polynomial evaluation](internals/polynomial.md): raw-coefficient Horner evaluation, fused multiply-add scaling, and when normalization can be deferred.
- [Pi constants](internals/pi-constants.md): offline Q0.63 generation, target-format truncation, and availability constraints.
- [`sqrt`](internals/sqrt.md): digit-by-digit integer square root, scaling, and rounding.

## Integration

- [`std` customization points and compatibility plans](integration/std-customization.md): current standard-permitted customizations, ADL usage, and future opt-in non-standard `std` math overloads.
