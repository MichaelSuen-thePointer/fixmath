# Pi constants

`fixed::two_pi()`, `fixed::pi()`, `fixed::half_pi()`, and `fixed::quarter_pi()` are constructed entirely from integers. The implementation does not convert a `float` or `double`, so the result is independent of the host floating-point implementation and the selected fixed-point rounding mode.

## Offline Q0.63 generation

For each mathematical value `x`, split it into its nonnegative integer and fractional parts:

```text
integer = floor(x)
fraction = x - integer
```

The source constant stores 63 binary fractional bits:

```text
fractional_q63 = floor(fraction * 2^63)
```

The following Python script reproduces the constants with 100 decimal digits of working precision. `mpmath` is an offline verification dependency only; it is not required to build or use Fixmath.

```python
import mpmath as mp

mp.mp.dps = 100

values = {
    "two_pi": 2 * mp.pi,
    "pi": mp.pi,
    "half_pi": mp.pi / 2,
    "quarter_pi": mp.pi / 4,
}

for name, value in values.items():
    integer = mp.floor(value)
    fractional_q63 = mp.floor((value - integer) * mp.mpf(2) ** 63)
    print(name, int(integer), int(fractional_q63), hex(int(fractional_q63)))
```

The output used by the implementation is:

| Function | Integer part | Q0.63 fractional decimal | Q0.63 fractional hexadecimal |
| --- | ---: | ---: | ---: |
| `two_pi()` | 6 | 2611923443488327891 | `0x243f6a8885a308d3` |
| `pi()` | 3 | 1305961721744163945 | `0x121fb54442d18469` |
| `half_pi()` | 1 | 5264666879299469876 | `0x490fdaa22168c234` |
| `quarter_pi()` | 0 | 7244019458077122842 | `0x6487ed5110b4611a` |

## Conversion to the target Q format

For a `fixed` type with `FRACTION_BITS = F`, where the policy guarantees `1 <= F <= 63`, the raw value is:

```text
raw = (integer << F) | (fractional_q63 >> (63 - F))
```

The right shift discards the low `63 - F` source bits. This is exactly downward truncation because, for integer `F <= 63`:

```text
floor(floor(fraction * 2^63) / 2^(63 - F))
    = floor(fraction * 2^F)
```

Consequently, the result is the largest representable fixed-point value no greater than the mathematical constant. No `RoundToZero` or `RoundToEven` policy operation participates in construction.

## Availability constraints

`INTEGER_BITS` includes the sign bit. Each function is constrained with the minimum number of signed integer bits required by its positive value:

| Function | Value interval | Required `INTEGER_BITS` |
| --- | --- | ---: |
| `two_pi()` | `[4, 8)` | 4 |
| `pi()` | `[2, 4)` | 3 |
| `half_pi()` | `[1, 2)` | 2 |
| `quarter_pi()` | `[0, 1)` | 1 |

If the type does not meet the requirement, the member function's C++20 `requires` constraint is false and the function is removed from the viable overload set.
