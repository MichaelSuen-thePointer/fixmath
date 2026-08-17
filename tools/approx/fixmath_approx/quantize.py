"""Search the shared-Q coefficient lattice around the real minimax solution."""

from dataclasses import dataclass

import mpmath as mp

from .fixed_eval import evaluate_factored
from .remez import locate_extrema


@dataclass
class QuantizationResult:
	raw_coefficients: list[int]
	initial_raw_coefficients: list[int]
	maximum_sampled_error: int
	worst_raw_input: int
	evaluations: int
	sampled_inputs: int
	extrema: list[tuple[mp.mpf, mp.mpf]]


def round_even(value: mp.mpf) -> int:
	return int(mp.nint(value))


def _objective(spec, coefficients, raw_inputs, oracle):
	worst_error = -1
	worst_input = 0
	for raw_x, expected in zip(raw_inputs, oracle):
		actual = evaluate_factored(raw_x, coefficients, spec.width, spec.fraction_bits, spec.rounding).raw
		error = abs(actual - expected)
		if error > worst_error:
			worst_error, worst_input = error, raw_x
	return worst_error, worst_input


def _real_samples(spec, sample_count=2049):
	result = []
	for index in range(sample_count):
		x = spec.public_interval[1] * index / (sample_count - 1)
		result.append((x, x * x, spec.reference(x)))
	return result


def _real_objective(coefficients, scale, samples):
	maximum = mp.mpf(0)
	for x, z, target in samples:
		horner = mp.mpf(coefficients[-1]) / scale
		for coefficient in reversed(coefficients[:-1]):
			horner = horner * z + mp.mpf(coefficient) / scale
		maximum = max(maximum, abs(x * horner - target))
	return maximum


def search(spec, coefficients, sample_count=16385):
	scale = spec.scale
	initial = [round_even(value * scale) for value in coefficients]
	limit_min, limit_max = -(1 << (spec.width - 1)), (1 << (spec.width - 1)) - 1
	if any(value < limit_min or value > limit_max for value in initial):
		raise OverflowError("rounded coefficient is outside the requested raw format")
	maximum_raw = round_even(spec.public_interval[1] * scale)
	raw_inputs = sorted({maximum_raw * i // (sample_count - 1) for i in range(sample_count)} | {0, maximum_raw})
	oracle = [round_even(spec.reference(mp.mpf(value) / scale) * scale) for value in raw_inputs]
	real_samples = _real_samples(spec)
	best = initial[:]
	best_score, best_input = _objective(spec, best, raw_inputs, oracle)
	best_real_score = _real_objective(best, scale, real_samples)
	evaluations = 1
	for _ in range(spec.quantize_passes):
		changed = False
		for index in range(len(best)):
			center = best[index]
			local_best = (best_score, best_real_score, abs(center - initial[index]), center, best_input)
			for delta in range(-spec.quantize_radius, spec.quantize_radius + 1):
				candidate_value = center + delta
				if candidate_value < limit_min or candidate_value > limit_max:
					continue
				candidate = best[:]
				candidate[index] = candidate_value
				score, worst_input = _objective(spec, candidate, raw_inputs, oracle)
				real_score = _real_objective(candidate, scale, real_samples)
				evaluations += 1
				key = (score, real_score, abs(candidate_value - initial[index]), candidate_value, worst_input)
				if key < local_best:
					local_best = key
			if local_best[3] != best[index]:
				best[index] = local_best[3]
				changed = True
			best_score, best_real_score, best_input = local_best[0], local_best[1], local_best[4]
		if not changed:
			break
	quantized = [mp.mpf(value) / scale for value in best]
	extrema = locate_extrema(spec.target, quantized, spec.interval, spec.remez_grid_size)
	return QuantizationResult(best, initial, best_score, best_input, evaluations, len(raw_inputs), extrema)
