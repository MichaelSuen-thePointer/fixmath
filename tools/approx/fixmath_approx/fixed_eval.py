"""Exact integer model of the Fixmath factored Horner evaluator."""

from dataclasses import dataclass


def div_pow2(value: int, fraction_bits: int, rounding: str) -> int:
	scale = 1 << fraction_bits
	if rounding == "RoundToZero":
		return value // scale if value >= 0 else -((-value) // scale)
	quotient, remainder = divmod(value, scale)
	half = scale >> 1
	if remainder > half or (remainder == half and quotient & 1):
		quotient += 1
	return quotient


@dataclass
class Evaluation:
	raw: int
	maximum_numerator_bits: int
	maximum_stage_raw: int
	overflow: bool


def evaluate_factored(raw_x: int, raw_coefficients: list[int], width: int, fraction_bits: int, rounding: str) -> Evaluation:
	scale = 1 << fraction_bits
	limit_min, limit_max = -(1 << (width - 1)), (1 << (width - 1)) - 1
	wide_limit_min, wide_limit_max = -(1 << (2 * width - 1)), (1 << (2 * width - 1)) - 1
	maximum_bits = 0
	maximum_stage = 0
	overflow = False

	def stage(numerator):
		nonlocal maximum_bits, maximum_stage, overflow
		maximum_bits = max(maximum_bits, abs(numerator).bit_length() + 1)
		overflow = overflow or not (wide_limit_min <= numerator <= wide_limit_max)
		value = div_pow2(numerator, fraction_bits, rounding)
		maximum_stage = max(maximum_stage, abs(value))
		overflow = overflow or not (limit_min <= value <= limit_max)
		return value

	z = stage(raw_x * raw_x)
	horner = raw_coefficients[-1]
	maximum_stage = max(maximum_stage, abs(horner))
	overflow = overflow or not (limit_min <= horner <= limit_max)
	for coefficient in reversed(raw_coefficients[:-1]):
		horner = stage(horner * z + coefficient * scale)
	result = stage(raw_x * horner)
	return Evaluation(result, maximum_bits, maximum_stage, overflow)
