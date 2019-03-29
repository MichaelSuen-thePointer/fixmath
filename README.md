# Fix32
A 64bit fixed-point number with 32bit integer, 32bit fraction part

Named Fix**32**, but its 64 bit wide.

## Features
1. Representation range \[`-(2^63-2)/2^32`, `(2^63-2)/2^32`\], and it's symmetric
1. `+-infinity` and `NaN` support
1. Overflow aware, calculation/comparison on `+-infinity` and `NaN` behaves just likes IEEE754 standard

## Details
1. Underlying representation is `int64_t`
1. Hand written soft 128bit multiplication, division and modulo (need platform specific optimization)
1. `INT64_MAX` is used internally to represent `+infinity`, `INT64_MIN+1` represents `-infinity` and `INT64_MIN` represents `NaN`

## TODO
1. Test cases on normal/overflow construction
1. Test cases on normal/overflow multiplication/division
1. Test cases on overflow addition/subtraction
1. Test cases on comparison
1. Test cases on converting from/to floating points
1. Unary operators
1. **Rethinking rounding behavior**
1. Math function implementation: sin, cos, log, exp...
1. Test cases on math functions
1. **Unsafe version**: Overflow unaware and only 0/0 produces NaN
1. Optimize 128bit multiplication/division
1. Python bindings


**still under construction**
