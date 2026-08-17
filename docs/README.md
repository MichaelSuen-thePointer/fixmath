# Fixmath Documentation

This directory documents Fixmath in layers: design constraints, data model, algorithm implementation, and ecosystem integration. The code remains the final authority on behavior; items marked as future plans are not part of the current API.

## Design

- [Design principles](design/principles.md): policy-based design, cross-platform consistency, exception-free behavior, and coding style.

## Concepts

- [Q M:N format and special values](concepts/q-format.md): raw integers, scaling, representable ranges, and `inf` / `nan` in strict mode.
- [Arithmetic and rounding modes](concepts/modes-and-rounding.md): `Ignore`, `SaturationMode`, `StrictMode`, `RoundToZero`, and `RoundToEven`.

## Internals

- [Basic arithmetic](internals/arithmetic.md): integer implementations of addition, subtraction, multiplication, and division, including overflow handling and fast paths.
- [Offline minimax approximation tool](internals/minimax-approximation.md): planned local coefficient generator, its dependencies, Chebyshev/Remez pipeline, raw-coefficient optimization, artifacts, and verification.
- [Polynomial evaluation](internals/polynomial.md): raw-coefficient Horner evaluation, fused multiply-add scaling, and when normalization can be deferred.
- [`sqrt`](internals/sqrt.md): digit-by-digit integer square root, scaling, and rounding.

## Integration

- [`std` customization points and compatibility plans](integration/std-customization.md): current standard-permitted customizations, ADL usage, and future opt-in non-standard `std` math overloads.
