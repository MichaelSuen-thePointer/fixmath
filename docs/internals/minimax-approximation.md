# Offline Minimax Approximation Tool

## Implementation status

The first implementation is available under `tools/approx/`. It supports checked-in JSON specifications for the factored basis `x * q(x^2)`, a shared signed Q format, round-to-even or round-to-zero Horner evaluation, deterministic neighborhood quantization, and sampled verification. Run the Q32.32 sine example with:

```text
python -m pip install -r tools/approx/requirements.txt
python tools/approx/generate.py --spec tools/approx/specs/sin_q32_32.json --output build/approx/sin_q32_32
```

The current verifier honestly labels its result `sampled`; interval-bounded certification and Sollya cross-checking remain later work. A successful run means the configured samples, Remez extrema neighborhoods, overflow checks, and target all passed, not that every raw input was exhaustively proved.

## Using the current tool

The current implementation generates and verifies one explicitly specified polynomial size per run. It does not yet search a degree range or automatically select the minimum coefficient count. Chebyshev fitting records a quick error estimate for the requested size, but the user must manually try adjacent sizes because Remez refinement, Q-format coefficient quantization, and stage-by-stage evaluator rounding can change the final result.

For the supported factored form

```text
p(x) = x * q(x^2)
q(z) = a_0 + a_1 z + ... + a_(m-1) z^(m-1)
```

`m` coefficients produce the odd powers `x^1` through `x^(2m-1)`. Set both fields consistently in the JSON specification:

```json
"basis": {
  "kind": "factored",
  "variable": "x_squared",
  "powers": [0, 1, 2, 3, 4],
  "reconstruction": "x_times_polynomial"
},
"degree": 9
```

This example requests five coefficients and therefore a degree-nine polynomial. Use a separate specification name and output directory for each candidate so their artifacts remain easy to compare:

```text
python tools/approx/generate.py --spec tools/approx/specs/sin_q32_32_m5.json --output build/approx/sin_q32_32_m5
python tools/approx/generate.py --spec tools/approx/specs/sin_q32_32_m6.json --output build/approx/sin_q32_32_m6
```

Select the minimum count with the following manual loop:

1. Start with a plausible coefficient count and run the complete generator.
2. If Remez does not converge, an intermediate overflows, or `maximum_implemented_error_ulp` exceeds the requested target, increase the count and try again.
3. If the candidate passes, reduce the count by one and regenerate instead of accepting the larger polynomial immediately.
4. Accept `m` as the minimum sampled candidate only after `m` passes the final exact-evaluator check and `m - 1` fails it.
5. Compare `manifest.json`, not only the Chebyshev estimate. Review the real Remez coefficients, emitted raw coefficients, fixed-coefficient extrema, maximum implemented error, worst raw input, intermediate width, and verification level.

A zero exit status means the configured target passed at the manifest's recorded verification level. At present that level is `sampled`, so the result may be described as the minimum sampled candidate, not as a globally certified minimum. Interval-bounded or exhaustive verification is still required before making a whole-domain proof.

## Purpose and scope

This document specifies the planned offline tool that will generate, optimize, verify, and emit fixed-point polynomial coefficients for Fixmath elementary functions. It is both the design basis for implementing that tool and the explanation of how generated coefficient tables are produced.

The tool is a development-time utility, not part of the header-only runtime library. Generated headers contain reviewed raw integer constants and deterministic evaluators; users of Fixmath do not need Python or any approximation package.

For a target function `f`, interval `[a, b]`, degree `n`, and polynomial `p`, the real absolute minimax problem is

```text
minimize ||f - p||_infinity

where ||g||_infinity = max(x in [a, b], abs(g(x)))
```

The proposed generation pipeline is:

```text
range reduction
    -> Chebyshev interpolant
    -> real-coefficient Remez refinement
    -> fixed-point coefficient refinement
    -> exact-model verification
    -> emitted raw coefficients
```

Chebyshev interpolation provides a fast, stable, near-minimax starting point. Remez exchange then reduces the real maximum error. A fixed-coefficient refinement is still required because rounding the real minimax coefficients to Fixmath raw values changes the polynomial. Finally, verification must include the rounding performed by the actual fixed-point evaluator.

The tool is responsible for candidate generation and reproducible evidence. It does not silently claim a proof when only numerical sampling has been performed; every output identifies its verification level.

## Local library selection

### Required dependency: Python and `mpmath`

The initial implementation should use Python 3 with `mpmath` as its only mandatory third-party dependency. `mpmath` is a pure Python arbitrary-precision arithmetic library with a BSD license. It provides the facilities needed by the generator without bringing NumPy, SciPy, GMP, or MPFR into the required path:

- arbitrary-precision elementary functions for the reference function;
- `chebyfit` for the fast near-minimax initial polynomial;
- arbitrary-precision matrices and linear-system solvers for Remez iterations;
- root finding, differentiation, and polynomial evaluation for candidate analysis.

`mpmath` does not provide the complete required tool. The Remez exchange loop, raw-coefficient optimization, exact Fixmath evaluator, artifact emission, and acceptance logic remain project code so that their behavior exactly matches Fixmath policies.

### Optional verification dependency: `python-flint`

`python-flint` may be enabled as an optional verification backend. It exposes FLINT and Arb arbitrary-precision ball arithmetic and distributes Windows x64 wheels, making it suitable for interval subdivision and conservative function bounds without becoming a mandatory generator dependency.

Arb support does not by itself implement a global minimax proof. The tool must still perform subdivision, bound the error expression on every subinterval, and combine those bounds into a certificate.

### Optional external reference: Sollya

Sollya is the reference cross-check tool because it already provides high-precision `remez`, fixed-coefficient `fpminimax`, and infinity-norm facilities. It should remain optional because a native build depends on GMP, MPFR, MPFI, fplll, and libxml2 and is substantially heavier than the proposed Python tool, particularly for Windows development.

Sollya may be used from WSL, a container, or an optional CI job to compare coefficients and error bounds. Normal coefficient generation must not require it.

### Alternatives not selected

- LolRemez is a compact local Remez command-line program with a Visual Studio solution and a header-only big-float implementation, but it does not optimize Fixmath raw coefficients or model `_fm_div2n_round`.
- Boost's historical minimax tool requires a patched NTL build and likewise lacks fixed-point coefficient refinement.
- Chebfun, MATLAB-based workflows, and general NumPy/SciPy approximation stacks are larger than necessary for this offline task.

If project code cannot converge for a difficult target, Sollya is the fallback generator. Replacing the lightweight path with Sollya should be an explicit per-function decision, not a hidden dependency change.

## Proposed tool structure

The implementation should keep numeric algorithms separate from command-line and output code. The following layout is illustrative; filenames may change without changing the architecture:

```text
tools/approx/
|-- generate.py             command-line entry point
|-- fixmath_approx/
|   |-- specification.py    input schema and validation
|   |-- chebyshev.py        initial degree estimate and coefficients
|   |-- remez.py            high-precision exchange algorithm
|   |-- quantize.py         raw coefficient search
|   |-- fixed_eval.py       exact Fixmath integer evaluator
|   |-- verify.py           sampled, exhaustive, and interval checks
|   `-- emit.py             manifest and C++ output
`-- tests/
```

The numeric modules must be importable independently of the CLI so deterministic unit tests can exercise each phase. Generated files must never depend on Python objects or serialized implementation details.

## Approximation specification

Every generation run starts from an explicit specification containing at least:

| Field | Meaning |
| --- | --- |
| Function | Named reference function or approved expression |
| Public domain | Inputs accepted by the future Fixmath API |
| Reduced interval | Core interval approximated by the polynomial |
| Range reduction | Identities and constants used before evaluation |
| Basis | Full, odd, even, factored, or explicit monomial list |
| Degree | Fixed degree or search range |
| Error objective | Absolute, relative, or weighted infinity norm |
| Raw format | Underlying width and `FRACTION_BITS` |
| Arithmetic mode | Ignore, saturation, or strict behavior in scope |
| Rounding mode | Round-to-zero or round-to-even |
| Working precision | Generator precision in bits or decimal digits |
| Accuracy target | Maximum accepted raw-unit, absolute, relative, or ULP error |

There must be no environment-dependent defaults for fields that affect emitted coefficients. Convenience defaults may exist in the CLI, but the resolved values are always written to the output manifest.

## 1. Reduce and normalize the interval

First use identities, symmetry, and periodicity to reduce the public input domain to the smallest practical core interval. Then map `[a, b]` to `[-1, 1]`:

```text
t = (2x - (a + b)) / (b - a)
x = (a + b) / 2 + (b - a)t / 2
g(t) = f(x(t))
```

The approximation is generated for `g(t)`. Keeping `t` near `[-1, 1]` improves coefficient conditioning and bounds Horner intermediates. The runtime range-reduction constants and their rounding errors are part of the final implementation error budget.

Exploit known structure before selecting a degree:

- Approximate an odd function with odd monomials only and an even function with even monomials only.
- Preserve exact values such as `sin(0) = 0` by factoring the polynomial, for example `sin(x) = x * q(x^2)`.
- For relative error near a zero of `f`, approximate a regular residual such as `sin(x) / x` instead of dividing by a vanishing target during optimization.

These restrictions reduce both coefficient count and evaluation cost. They must be supplied to minimax generation as the actual basis, rather than applied by deleting coefficients afterward.

## 2. Generate a Chebyshev initial fit

The initial tool should call `mpmath.chebyfit(g, [-1, 1], n + 1, error=True)` at the configured working precision. It returns a degree-`n` near-minimax polynomial in descending monomial order together with a numerical maximum-error estimate. This is sufficient for degree estimation and a starting coefficient vector without implementing a separate transform in the first version.

Conceptually, the fit samples `g` at Chebyshev-distributed points, such as the second-kind grid

```text
t_j = cos(j * pi / n),  j = 0..n
```

up to an inconsequential reversal of point order. Such samples define an interpolant that may also be represented in the Chebyshev basis

```text
p_cheb(t) = sum(k = 0..n, c_k T_k(t))
```

and converted by a discrete cosine transform. Chebyshev interpolants are generally close to the best uniform approximation and are much cheaper to generate than a minimax polynomial, which makes them suitable for degree estimation and initialization. They are not themselves a proof of minimax optimality, and the error returned by `chebyfit` is recorded as an estimate rather than a certificate.

The generator should increase `n` until the measured error of `p_cheb` has adequate margin below the requested target. A small margin is insufficient because later coefficient quantization and fixed-point evaluation add error.

The initial `chebyfit` result may contain powers excluded by the final odd, even, or factored basis. It is only a seed: the Remez phase must solve directly in the constrained basis that will ultimately be emitted. Do not obtain a constrained polynomial merely by deleting coefficients from the initial fit.

## 3. Refine with the Remez exchange algorithm

For a full polynomial of degree `n`, begin with `n + 2` ordered reference points. The extrema of `T_(n+1)`, mapped to `[a, b]`, are a natural initial reference set:

```text
t_j = cos(j * pi / (n + 1)),  j = 0..n+1
```

If the Chebyshev interpolant already provides reliable alternating error extrema, those extrema can instead seed the reference set.

At each Remez iteration, solve for the coefficients and an error amplitude `E` such that

```text
p(t_j) - g(t_j) = (-1)^j E
```

Then locate the extrema of the new error function `e(t) = p(t) - g(t)` over the full interval, select an ordered set with alternating signs, and exchange it into the reference set. Iterate until the extreme magnitudes agree within the generator precision and the reference set no longer changes materially.

For a basis with `m` free coefficients, the target alternation count is `m + 1`, not necessarily `n + 2`. Under the usual continuity and Haar/Chebyshev-system conditions, equal-magnitude alternating extrema characterize the best uniform approximation. Sollya's `remez` command implements this weighted infinity-norm problem and supports custom monomial lists or function bases.

Choose the error objective explicitly:

```text
absolute: max abs(p(x) - f(x))
relative: max abs((p(x) - f(x)) / f(x))
weighted: max abs(w(x) * (p(x) - f(x)))
```

Absolute error maps directly to raw-value error. Relative error is often more useful over a wide dynamic range but is undefined at zeros unless the problem is transformed or split into intervals.

## 4. Refine coefficients on the Fixmath lattice

The real minimax coefficients normally are not exactly representable as Fixmath raw integers. With a shared coefficient scale `S = 2^N`, the emitted coefficient is

```text
A_i = integer
a_i_fixed = A_i / S
```

Simply rounding every real coefficient independently to the nearest `A_i / S` can substantially increase the maximum error and destroy equioscillation. The real minimax polynomial should therefore be treated as the center of a constrained search over integer coefficient vectors.

Recommended options, in increasing implementation effort, are:

1. Search a bounded neighborhood around each rounded raw coefficient and retain the vector with the smallest measured infinity norm.
2. Use a lattice-basis-reduction or mixed-integer method to optimize all quantized coefficients together.
3. Use Sollya `fpminimax` in `fixed` mode as the initial offline generator. It accepts a fixed number of fractional bits for each coefficient and internally starts from a real minimax polynomial. Its method is heuristic but specifically targets good finite-precision `L-infinity` coefficients.

The search must enforce the actual basis, shared or per-coefficient scale, exact constrained coefficients, and raw integer range. The first Fixmath implementation should prefer one shared Q format for all coefficients because it matches the existing raw-coefficient Horner model. Per-coefficient formats are a possible later optimization but require explicit rescaling at evaluation time.

After quantization, rerun the error-extrema search. Real equioscillation is no longer a necessary certificate of optimality on the coefficient lattice, but loss of the expected alternating shape is a useful diagnostic that naive coefficient rounding has degraded the result.

## 5. Model the implemented evaluator

The polynomial-only real error is not the final library error. For a generated function `F_impl`, separate at least these components:

```text
E_total <= E_range_reduction
         + E_polynomial
         + E_coefficient_quantization
         + E_fixed_evaluation
         + E_reconstruction
```

This sum is a conservative decomposition; a tighter analysis may track correlations rather than summing independent maxima.

The model used during final coefficient selection should match the intended runtime evaluator:

- Coefficients are the exact emitted raw integers.
- Horner stages use the same intermediate widths as the C++ implementation.
- Each same-scale multiply-add is normalized with `_fm_div2n_round<policy, fixed::FRACTION_BITS>`.
- `RoundToZero` and `RoundToEven` are evaluated separately when both policies are supported.
- Intermediate overflow, saturation, and strict-mode special values are included, not assumed away.

Optimizing only `||f - p||_infinity` may select coefficients that are inferior after stage-by-stage rounding. A later generator may optimize the exact discrete evaluator directly around the fixed-coefficient solution, but the acceptance test must always measure that evaluator.

## 6. Verification required for an accuracy guarantee

Remez convergence and coefficient refinement produce candidates; they do not by themselves guarantee the accuracy of the shipped C++ path. Acceptance requires all of the following:

1. Compute a high-precision or certified bound for the real approximation error over the complete reduced interval. Sollya distinguishes quick numerical estimates such as `dirtyinfnorm`, which may underestimate error, from interval-based norm checks intended for validation.
2. Verify all raw input values exhaustively when the reduced discrete domain is small enough. Compare the exact integer evaluator against a high-precision oracle and record the worst input and signed error.
3. For a large raw domain, partition the interval and combine interval bounds with targeted checks at endpoints, stationary points of the real error, range-reduction boundaries, and rounding-transition neighborhoods.
4. Prove that every intermediate accumulator stays within its intended width, or verify the exact saturation / infinity result for paths where overflow is allowed.
5. Test both arithmetic modes and rounding modes claimed by the function. A coefficient set approved for one policy is not automatically approved for another.
6. Store generation metadata with the raw coefficient table: function, reduced interval, basis, degree, Q format, objective, generator precision, maximum real error, maximum implemented error, and worst-case input.

The release criterion should be expressed in raw units or ULPs of the target Fixmath format. For example, a strict `<= 1 raw unit` target means the verified result differs from the correctly rounded target by at most `2^-N` everywhere in the supported interval.

## Proposed command-line workflow

The primary interface should consume a checked-in JSON specification instead of relying on a long command line. JSON keeps the first implementation within the Python standard library and makes every coefficient set reproducible.

```text
python tools/approx/generate.py \
    --spec tools/approx/specs/sin_q32_32.json \
    --output build/approx/sin_q32_32
```

The command resolves and validates the specification, runs all configured phases, writes artifacts to the output directory, and returns a nonzero exit status if convergence, range, overflow, or accuracy requirements fail. A future `--check` mode may regenerate into memory and compare against checked-in artifacts without rewriting them.

Reference functions should come from a reviewed registry in `specification.py`. The first implementation must not evaluate arbitrary Python supplied by a JSON file. Named transformed functions such as `sin_over_x` may be registered when range reduction requires them.

## Generated artifacts

Each successful run should produce three logical artifacts:

### Machine-readable manifest

The JSON manifest is the source record for review and regeneration. It contains:

- the fully resolved approximation specification;
- tool and algorithm versions;
- real minimax coefficients at generator precision;
- emitted raw coefficients and their ordering;
- Remez reference points, iteration count, and final extrema;
- coefficient-search method and search bounds;
- measured real-polynomial and exact-evaluator errors;
- worst-case inputs found by each verification phase;
- intermediate range bounds;
- verification level and any optional backend versions.

### C++ coefficient include

The `.inl` output contains only the raw integer constants and metadata required by the runtime implementation. Coefficients are emitted in the order consumed by Horner evaluation, with the Q format, interval, basis, and verified error recorded in comments. The generated file must follow the repository's C++ formatting and UTF-8-with-BOM requirements before it can be committed.

### Human-readable report

The report summarizes convergence, equioscillation, quantization changes, overflow margins, and accuracy results. It should make review possible without manually decoding the manifest, but must not be the only place where evidence is stored.

Tracked artifacts should be byte-for-byte reproducible. Wall-clock timestamps, temporary paths, process IDs, and unordered mappings must not affect generated content. If timestamps are useful for local reports, keep them outside tracked outputs.

## Verification levels

The manifest labels the strongest completed level:

| Level | Meaning |
| --- | --- |
| `sampled` | High-precision sampling and extrema searches found no violation; not a proof |
| `exhaustive-raw` | Every raw input in the declared reduced discrete domain was checked |
| `interval-bounded` | Interval subdivision produced a conservative bound over the continuous domain |
| `cross-checked` | An independent tool such as Sollya agreed within the recorded tolerance |

`cross-checked` is supplementary and does not replace exhaustive or interval coverage. Documentation and generated comments must use words such as "guaranteed" or "certified" only when the recorded evidence justifies them.

## First implementation milestone

For the first generated transcendental function:

1. Choose a symmetry-reduced interval and an odd/even factored basis.
2. Generate a high-precision Chebyshev interpolant to estimate the required degree.
3. Run high-precision Remez on the exact target basis for the real minimax polynomial.
4. Run the built-in integer-neighborhood search for coefficients representable in the chosen Fixmath Q format; compare with `fpminimax(..., fixed, absolute)` when Sollya is available.
5. Emit coefficients as raw integer constants in descending Horner order.
6. Evaluate each Horner stage with the existing `_fm_div2n_round` semantics.
7. Verify the exact raw evaluator and publish the measured or certified error bound with the coefficient table.

This keeps approximation generation offline. The Fixmath headers contain only reviewed raw constants and a small deterministic evaluator, with no dependency on the generation tool.

## References

- [Chebfun Guide: Chebyshev series, interpolants, and best approximations](https://www.chebfun.org/docs/guide/guide04.html)
- [`mpmath` arbitrary-precision arithmetic](https://mpmath.org/)
- [`mpmath.chebyfit` documentation](https://mpmath.org/doc/current/calculus/approximation.html)
- [`mpmath` matrix and linear-system documentation](https://mpmath.org/doc/1.3.0/matrices.html)
- [`python-flint` repository and installation support](https://github.com/flintlib/python-flint)
- [Sollya compilation dependencies](https://sollya.org/sollya-8.0/sollya.php)
- [Sollya `remez` documentation](https://sollya.org/sollya-8.0/help.php?name=remez)
- [Sollya `fpminimax` documentation](https://sollya.org/sollya-8.0/help.php?name=fpminimax)
- [Brisebarre and Chevillard, Efficient Polynomial L-infinity Approximations](https://www.lirmm.fr/arith18/papers/brisebarre-chevillard-Linfini.pdf)
- [Sollya `dirtyinfnorm` documentation and validation warning](https://sollya.org/sollya-current/help.php?name=dirtyinfnorm)
- [LolRemez repository](https://github.com/samhocevar/lolremez)
- [Boost minimax tool documentation](https://www.boost.org/doc/libs/boost_1_46_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html)
