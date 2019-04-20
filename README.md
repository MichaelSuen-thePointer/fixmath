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

## Discoveries
1. Multiprecision divison algorithm introduced in Knuth's TAOCP Vol.2 is **faster** than shift-subtraction division, about 100ns
2. 128bit mutiplication costs ~400ns on Intel Core i7 8700 and ~150ns on AMD Ryzen R7 2700X
3. 128bit division costs ~400ns on both Intel and AMD

## TODO
- [ ] Test cases on normal/overflow construction
- [ ] Test cases on normal/overflow multiplication/division
- [x] Test cases on overflow addition/subtraction
- [ ] Test cases on comparison
- [ ] Test cases on constants
- [ ] Test cases on converting from/to floating points
- [x] Unary operators
- [ ] **Rethinking rounding behavior**
- [ ] Math function implementation: sin, cos, log, exp...
- [ ] Test cases on math functions
- [ ] **Unsafe version**: Overflow unaware and only 0/0 produces NaN
- [x] Optimize 128bit multiplication/division
- [ ] Python bindings


**still under construction**
