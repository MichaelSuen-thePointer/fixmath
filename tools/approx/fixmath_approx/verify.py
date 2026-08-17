"""High-precision sampled verification of real and exact fixed evaluators."""

from dataclasses import dataclass

import mpmath as mp

from .fixed_eval import evaluate_factored


@dataclass
class VerificationResult:
	level: str
	checked_inputs: int
	maximum_implemented_error_ulp: int
	worst_raw_input: int
	maximum_real_error: mp.mpf
	worst_real_input: mp.mpf
	maximum_numerator_bits: int
	maximum_stage_raw: int
	overflow: bool


def _poly(coefficients, z):
	value = mp.mpf(0)
	for coefficient in reversed(coefficients):
		value = value * z + coefficient
	return value


def verify(spec, raw_coefficients, extrema):
	scale = spec.scale
	maximum_raw = int(mp.nint(spec.public_interval[1] * scale))
	count = spec.verification_samples
	raw_inputs = {maximum_raw * i // (count - 1) for i in range(count)}
	# Remez extrema are in z=x^2. Probe their raw neighborhoods, where evaluator rounding can change.
	for z, _ in extrema:
		x_raw = int(mp.nint(mp.sqrt(max(z, 0)) * scale))
		for delta in range(-8, 9):
			if 0 <= x_raw + delta <= maximum_raw:
				raw_inputs.add(x_raw + delta)
	raw_inputs.update((0, 1, maximum_raw - 1, maximum_raw))
	maximum_error = -1
	worst_input = 0
	maximum_bits = 0
	maximum_stage = 0
	overflow = False
	for raw_x in sorted(raw_inputs):
		evaluation = evaluate_factored(raw_x, raw_coefficients, spec.width, spec.fraction_bits, spec.rounding)
		expected = int(mp.nint(spec.reference(mp.mpf(raw_x) / scale) * scale))
		error = abs(evaluation.raw - expected)
		if error > maximum_error:
			maximum_error, worst_input = error, raw_x
		maximum_bits = max(maximum_bits, evaluation.maximum_numerator_bits)
		maximum_stage = max(maximum_stage, evaluation.maximum_stage_raw)
		overflow = overflow or evaluation.overflow
	real_coefficients = [mp.mpf(value) / scale for value in raw_coefficients]
	real_count = max(32769, min(count, 262145))
	maximum_real_error = mp.mpf(-1)
	worst_real_input = mp.mpf(0)
	for i in range(real_count):
		x = spec.public_interval[1] * i / (real_count - 1)
		approximation = x * _poly(real_coefficients, x * x)
		error = abs(approximation - spec.reference(x))
		if error > maximum_real_error:
			maximum_real_error, worst_real_input = error, x
	return VerificationResult("sampled", len(raw_inputs), maximum_error, worst_input, maximum_real_error, worst_real_input, maximum_bits, maximum_stage, overflow)
