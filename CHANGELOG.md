# Changelog

## Unreleased

- Support fractional-bit counts through `F == N - 1` for the supported signed raw types.
- Remove the public `fixed::RATIO` constant because `2^(N-1)` is not representable by an N-bit signed raw type.
