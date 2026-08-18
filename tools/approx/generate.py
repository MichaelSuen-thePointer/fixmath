#!/usr/bin/env python3
"""Generate fixed-point polynomial coefficients from an explicit JSON spec."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import mpmath as mp

from fixmath_approx.chebyshev import initial_fit
from fixmath_approx.emit import emit
from fixmath_approx.quantize import search
from fixmath_approx.remez import run
from fixmath_approx.specification import load_spec
from fixmath_approx.verify import verify


def main() -> int:
	parser = argparse.ArgumentParser()
	parser.add_argument("--spec", type=Path, required=True)
	parser.add_argument("--output", type=Path, required=True)
	args = parser.parse_args()
	spec = load_spec(args.spec)
	mp.mp.dps = spec.precision_digits
	chebyshev_coefficients, chebyshev_error = initial_fit(spec.target, spec.interval, len(spec.powers))
	if len(chebyshev_coefficients) != len(spec.powers):
		raise RuntimeError("unexpected Chebyshev coefficient count")
	remez = run(spec.target, spec.interval, spec.powers, spec.remez_max_iterations, spec.remez_grid_size, spec.remez_tolerance)
	if not remez.converged:
		print("error: Remez exchange did not converge", file=sys.stderr)
		return 2
	quantization = search(spec, remez.coefficients)
	verification = verify(spec, quantization.raw_coefficients, quantization.extrema)
	manifest = emit(args.output, spec, chebyshev_error, remez, quantization, verification)
	print(f"raw coefficients (Horner order): {manifest['quantization']['raw_coefficients_horner_order']}")
	print(f"maximum implemented error: {verification.maximum_implemented_error_ulp} ulp ({verification.checked_inputs} inputs checked)")
	print(f"verification level: {verification.level}")
	if verification.overflow:
		print("error: evaluator overflow observed", file=sys.stderr)
		return 3
	if verification.maximum_implemented_error_ulp > spec.target_ulp:
		print(f"error: target is {spec.target_ulp} ulp", file=sys.stderr)
		return 4
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
