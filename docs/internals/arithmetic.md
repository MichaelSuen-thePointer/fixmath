# Internal Arithmetic

Let the raw integers of two values with the same format be `A` and `B`, let the number of fractional bits be `N`, and let the scale factor be `S = 2^N`.

## Addition and subtraction

```text
raw(a + b) = A + B
raw(a - b) = A - B
```

Both operands have the same scale, so no binary-point adjustment or rounding is required. Ignore mode uses unsigned addition and subtraction to define wrapping at the underlying width without relying on undefined signed overflow. The other modes use checked addition and subtraction to detect overflow and map it to saturation boundaries or strict-mode infinities. Strict mode handles `nan` / `inf` combinations before entering the integer path.

## Multiplication

The raw product has `2N` fractional bits and must be scaled back to the target format:

```text
raw(a * b) = round((A * B) / 2^N)
```

A 32-bit underlying type uses a 64-bit intermediate product. The general path for a 64-bit underlying type produces a signed 128-bit product `(high, low)`, shifts the pair right by `N` bits with policy-controlled rounding, and then checks whether the high half is the sign extension of the low half. A mismatch means the result does not fit in the target 64-bit representation.

The 128-bit multiplication backend is selected by platform: native `__int128`, MSVC `_mul128`, or software multiplication that splits operands into 32-bit limbs. All three paths must produce the same high and low 64-bit result.

### 64-bit multiplication fast path

When `N < 62` and both `A` and `B` can be narrowed to `int32_t` without loss, their product is guaranteed to fit in a signed 64-bit intermediate. The implementation performs one 64-bit multiplication followed by division by `2^N` with the selected rounding mode, bypassing the full 64-by-64-to-128 multiplication and high-half overflow check.

The `N < 62` condition also satisfies the sign-bit safety constraint of the power-of-two rounding helper. This branch changes only the intermediate width; it does not change the mathematical formula or rounding result.

## Division

To preserve `N` fractional bits before integer division, the dividend is first scaled up:

```text
raw(a / b) = round((A * 2^N) / B)
```

The general 64-bit path represents `A * 2^N` as a signed 128-bit dividend and performs 128-by-64-bit division, producing a 128-bit quotient and signed remainder. Round-to-even compares `2 * abs(remainder)` with `abs(B)`. At an exact midpoint, it examines the quotient's least-significant bit and increments or decrements according to the quotient sign. The implementation then detects overflow from the sign extension of the quotient's high half and from the finite range boundaries.

A 32-bit underlying type can evaluate the same formula directly with a 64-bit intermediate dividend.

### 64-bit division fast paths

Division has two optimization levels:

1. When `N < 62` and `A` lies within the safe interval from `INT64_MIN / 2^N` through `INT64_MAX / 2^N`, `A * 2^N` cannot overflow 64 bits. The implementation uses ordinary 64-bit multiplication, remainder, and division operations and bypasses 128-bit division.
2. Outside that safe interval, a `Q32:32` format uses `_fm_shl32div`, which exploits the fixed 32-bit shift to assemble the 128-bit dividend directly. It processes the upper 32 bits first, then computes the lower quotient with x64 `divq`, MSVC `_udiv128`, or the software 128-by-64 backend, avoiding the general shift-and-assemble path.

Other 64-bit cases use the general `_fm_div128` implementation. Its backend prefers target instructions or intrinsics and otherwise uses normalized software long division with 32-bit digits. Fast and general paths share the same remainder-based rounding and final range checks.

## Special values and fast-path boundaries

Strict mode handles `nan`, `inf`, division by zero, and other special combinations before entering the integer core. Saturation and Ignore modes also handle division by zero before the division core. Fast paths therefore do not redefine special-value semantics; they only have to remain bit-for-bit equivalent to the general finite-value path.
