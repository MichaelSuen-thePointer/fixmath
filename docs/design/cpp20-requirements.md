# Required C++20 Features

Fixmath requires C++20 as part of its numeric and type-system contract, not only as a build-system baseline. The implementation currently depends on the following C++20 library and language features.

## `std::bit_cast`

Floating-point conversion helpers use `std::bit_cast` to transfer the object representation between `float` and `uint32_t`, and between `double` and `uint64_t`, without pointer aliasing or union-punning:

```cpp
const uint32_t bits = std::bit_cast<uint32_t>(value);
return std::bit_cast<double>(raw);
```

The extracted sign, exponent, and significand are then processed with integer arithmetic. This supports deterministic conversion and permits the helpers to remain `constexpr` when the standard's constant-evaluation requirements are satisfied. `std::bit_cast` guarantees representation-preserving transfer between suitable same-size types; it does not by itself guarantee that a platform uses the IEEE-754 binary32 and binary64 encodings expected by these helpers.

## C++20 signed left shift

C++20 defines `E1 << E2` as the unique result congruent to `E1 * 2^E2` modulo `2^W`, where `W` is the width of the result type. This applies to a signed negative left operand and to a mathematical product outside the signed range. The shift count must still be nonnegative and less than `W`.

Fixmath relies on this rule when a signed high limb is shifted left to transfer bits into the low limb during a two-word shift. The operation has the required modulo bit pattern without depending on signed multiplication overflow. See [Power-of-two Division and Rounding](../internals/div2n-rounding.md#c20-signed-left-shift-semantics) for the complete derivation and the applicable shift-count invariant.

This is a C++20-specific portability assumption. Code intended to compile as C++17 or earlier would need to convert the left operand to the corresponding unsigned type before shifting.

## Three-way comparison

Fixmath implements `operator<=>` as the central comparison operation. Ordinary policies return `std::strong_ordering`; strict policies return `std::partial_ordering` so that comparisons involving `nan` can produce `std::partial_ordering::unordered`.

The compiler rewrites `<`, `<=`, `>`, and `>=` expressions from the three-way comparison overload. Fixmath defines equality operators explicitly and uses `<=>` inside them. The same comparison model is also used for supported mixed `fixed` and integer operands after conversion to their constrained common fixed-point type.

## Concepts and constraints

The public templates use C++20 concepts and `requires` clauses to state their valid domains:

- `FixedPolicy` admits only Fixmath policy types as `fixed` template arguments.
- `Fixed` identifies Fixmath fixed-point types.
- `PromotesToInt32` limits seamless integer interaction to types whose integral promotion produces `int32_t`.
- `FixedImplicitBinaryOperable` enables mixed operators and `std::common_type` only for one `fixed` operand and one supported integer operand.

These constraints are part of overload selection, not merely diagnostic checks inside function bodies. In particular, they prevent the integer interoperability layer from accidentally accepting floating-point values, 64-bit integers, or unrelated user-defined types.

## Compiler requirement

A supported compiler and standard library must therefore provide all of the following in C++20 mode:

- `<bit>` and `std::bit_cast`;
- the C++20 `[expr.shift]` signed left-shift semantics;
- `<compare>`, comparison category types, and `operator<=>` rewriting;
- concepts, constrained template parameters, and `requires` clauses.

Partial C++20 modes that lack any of these facilities are not supported even if they accept other parts of the headers.

## References

- [C++20 working draft N4861: `std::bit_cast` `[bit.cast]`](https://timsong-cpp.github.io/cppwp/n4861/bit.cast)
- [C++20 working draft N4861: shift operators `[expr.shift]`](https://timsong-cpp.github.io/cppwp/n4861/expr.shift)
- [C++20 working draft N4861: three-way comparison `[expr.spaceship]`](https://timsong-cpp.github.io/cppwp/n4861/expr.spaceship)
- [C++20 working draft N4861: constraints and concepts `[temp.constr]`](https://timsong-cpp.github.io/cppwp/n4861/temp.constr)
