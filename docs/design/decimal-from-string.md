# Decimal `from_string` Format

## Status and scope

This document defines the accepted source-text grammar planned for `from_string`. It is a design contract for a future API, not a description of functionality currently available in Fixmath.

The planned parser returns `bool` to report success or failure and writes the parsed value through a reference output parameter. The exact public function signature and the state of the output parameter after failure remain separate decisions.

## Normative grammar

The input format is:

```text
value       = finite / "nan" / infinity
finite      = [ sign ] integer [ fraction ] [ exponent ]
infinity    = [ sign ] "inf"
sign        = "+" / "-"
integer     = 1*DIGIT
fraction    = "." 1*DIGIT
exponent    = ( "e" / "E" ) [ sign ] 1*DIGIT
DIGIT       = %x30-39
```

In equivalent compact notation:

```text
(?:[+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?|nan|[+-]?inf)
```

The notation above describes a complete input, not a substring search. Every input character must belong to the matched decimal representation.

## Required syntax

- Only the ASCII characters shown in the grammar are accepted.
- A finite value's leading sign is optional and, when present, applies to the complete finite value.
- The integer part is mandatory and contains at least one digit. A value between minus one and one must therefore include `0` before the decimal point.
- The decimal point and fractional digits form one optional unit. If the decimal point is present, at least one fractional digit is mandatory.
- The exponent forms one optional unit beginning with `e` or `E`. Its sign is independently optional, but at least one exponent digit is mandatory.
- The exact spelling `nan` is accepted without a sign.
- The spellings `inf`, `+inf`, and `-inf` are accepted.
- The grammar does not include whitespace, digit separators, locale-specific decimal separators, hexadecimal input, or other special-value spellings.

Leading zeroes are syntactically valid in both the significand and exponent. This grammar defines only whether the source text is well formed; it does not imply that every well-formed value is representable by every `fixed<policy>` type.

## Special values and arithmetic modes

The parser recognizes `nan`, `inf`, `+inf`, and `-inf` for every arithmetic mode. A recognized special value is a successful parse:

- `nan` writes `fixed::nan()`;
- `inf` and `+inf` write `fixed::inf()`;
- `-inf` writes `-fixed::inf()`.

This rule also applies to `Ignore` and `SaturationMode`. In those modes, the factories' raw patterns are not classified as special values by `is_nan()` or `is_inf()`; parsing nevertheless recognizes the source spelling and writes the corresponding factory representation. `StrictMode` retains its existing classified `nan` and positive or negative infinity semantics.

## Examples

Valid complete inputs include:

```text
0
+0
-0
123
00123
0.123
-12.3400
1e2
1e+2
1e-2
-12.34e+005
1E2
-12.34E-5
nan
inf
+inf
-inf
```

Invalid complete inputs include:

```text
.123       # missing integer part
-.123      # missing integer part
1.         # decimal point without fractional digits
1.e2       # decimal point without fractional digits
e2         # missing significand
1e         # missing exponent digits
1e+        # exponent sign without exponent digits
 1         # leading whitespace is not in the grammar
1          # followed by trailing whitespace: not a complete match
1,25       # locale-specific decimal separator is not in the grammar
+nan       # nan does not accept a sign
-nan       # nan does not accept a sign
Infinity   # alternative special-value spelling is not in the grammar
```

## Planned parsing stages

### 1. Special-value fast path

Before scanning a finite decimal, compare the complete input against `nan`, `inf`, `+inf`, and `-inf`. On a match, write the corresponding value described above and return success.

### 2. Finite syntax scan

Scan the input once with a grammar-aware state machine. Validate the complete finite grammar while recording:

- the leading sign;
- the position of the first nonzero significand digit, if any;
- the decimal-point position, if present;
- the `e` or `E` position, if present;
- the exponent sign;
- the number of significand digits before the decimal point.

The search for the first nonzero digit covers only the significand, not the exponent. The scan rejects misplaced or repeated signs, decimal points, and exponent markers, as well as missing mandatory digits.

### 3. Exponent parsing

Parse the exponent magnitude as decimal digits. The signed exponent is restricted to the complete `int32_t` range. Accumulate its magnitude in an unsigned type and check the sign-specific limit before every multiply-add:

```text
digit = ch - '0'
limit = negative exponent ? 2147483648 : 2147483647

if magnitude > limit / 10:
    fail
if magnitude == limit / 10 and digit > limit % 10:
    fail

magnitude = magnitude * 10 + digit
```

This accepts `-2147483648`, accepts `+2147483647`, and rejects exponents outside that range without overflowing the accumulator.

### 4. Zero fast path

After the syntax and exponent have been validated, an absent first nonzero significand digit means the finite value is zero regardless of the exponent. Write zero and return success. A leading minus sign does not create a distinct raw negative-zero value.

### 5. Logical decimal-point adjustment

For a nonzero significand, combine the scanned decimal-point position, the number of leading significand zeroes, and the signed exponent to obtain the logical decimal-point position relative to the first nonzero digit. This is an integer position, not a pointer that must remain inside the input.

The calculation must not overflow when a very long input or an extreme valid exponent places the logical decimal point far outside the significand. The implementation may use checked or saturating position arithmetic because only subsequent range comparisons require the out-of-range direction.

### 6. Integer overflow fast path

Use the logical count of integer digits for an early overflow decision. If it exceeds the maximum possible decimal digit count of the sign- and policy-specific finite boundary, write positive or negative infinity and return success. Equal digit counts require an exact comparison and cannot use this fast path alone.

This result rule applies to every arithmetic mode: numeric overflow detected by `from_string` writes `fixed::inf()` or `-fixed::inf()`, rather than reporting a syntax failure.

### 7. Integer-part accumulation

Accumulate the integer part with a pre-multiply boundary check analogous to exponent parsing. The limit is derived from the target `fixed<policy>` raw boundary for the parsed sign, not from `int32_t`: the underlying type may be 64 bits, positive and negative limits may be asymmetric, and `StrictMode` reserves raw patterns for special values.

An integer part exactly at its permitted boundary is not necessarily a final success. Fraction conversion and rounding may add raw units or carry into the integer part, so the final boundary decision remains pending until the fractional part has been processed.

### 8. Fractional-part conversion

Convert the fractional part with an exact `10^(F+1)` scaled integer, where `F` is the target type's number of fractional bits:

- Set `S = 10^(F+1)`.
- Read the first `F+1` logical decimal fractional positions into an integer `D`, padding missing positions with zero. Decimal digits beyond those positions are reduced to a `tail_nonzero` sticky flag.
- For each binary fractional position `i` from 1 through `F`, compare the remainder against `S / 2^i`. If it is at least that weight, subtract the weight and set bit `i` of the fractional raw value.
- Every weight is an integer because `10^(F+1)` contains a factor of `2^(F+1)`. Consequently, the comparisons and subtractions are exact integer operations.
- After extracting `F` bits, compare the remainder with the half-ULP weight `S / 2^(F+1) = 5^(F+1)`.
  - `RoundToZero` does not increment the raw magnitude.
  - `RoundToEven` increments when the remainder is greater than the half-ULP weight. When they are equal, it increments if `tail_nonzero` is set, or, for an exact tie, if the current raw magnitude is odd.
- If rounding carries out of the fractional field, carry one into the integer part and apply the normal finite-range overflow handling.

This is specifically the exact multiply-by-`10^(F+1)` design, not an approximate decimal-prefix or floating-point-intermediary design. For the current maximum `F = 63`, `S = 10^64` requires 213 value bits, so a 256-bit unsigned intermediate is sufficient.

For Q32, for example, `S = 10^33`; the bit weights are `10^33 / 2^i`, and the half-ULP weight is `10^33 / 2^33 = 5^33`.

## Deferred decisions

The following must still be specified before implementation begins:

- exact public function signature, the output value after failure, and whether partial parsing is supported;
- how syntax errors and the first unconsumed character are reported;
- underflow behavior for `Ignore`, `SaturationMode`, and `StrictMode`;
- resource limits or an algorithmic guarantee for very long significands and exponents;
- whether the function is required to be usable during constant evaluation.
