"""High-precision Remez exchange for a monomial basis."""

from dataclasses import dataclass

import mpmath as mp


@dataclass
class RemezResult:
	coefficients: list[mp.mpf]
	error_amplitude: mp.mpf
	reference_points: list[mp.mpf]
	extrema: list[tuple[mp.mpf, mp.mpf]]
	iterations: int
	converged: bool


def evaluate(coefficients, x):
	result = mp.mpf(0)
	for coefficient in reversed(coefficients):
		result = result * x + coefficient
	return result


def _solve(function, references, powers):
	matrix = mp.matrix(len(references), len(powers) + 1)
	rhs = mp.matrix(len(references), 1)
	for row, x in enumerate(references):
		for column, power in enumerate(powers):
			matrix[row, column] = x**power
		matrix[row, len(powers)] = -1 if row % 2 else 1
		rhs[row] = function(x)
	solution = mp.lu_solve(matrix, rhs)
	return [solution[i] for i in range(len(powers))], solution[len(powers)]


def _maximize_abs(error, left, center, right):
	# Golden-section search is stable here because the dense scan brackets one local extremum.
	phi = (mp.sqrt(5) - 1) / 2
	a, b = left, right
	c = b - phi * (b - a)
	d = a + phi * (b - a)
	for _ in range(90):
		if abs(error(c)) < abs(error(d)):
			a, c = c, d
			d = a + phi * (b - a)
		else:
			b, d = d, c
			c = b - phi * (b - a)
	x = (a + b) / 2
	return x, error(x)


def locate_extrema(function, coefficients, interval, grid_size):
	a, b = interval
	error = lambda x: evaluate(coefficients, x) - function(x)
	xs = [a + (b - a) * i / grid_size for i in range(grid_size + 1)]
	values = [error(x) for x in xs]
	candidates = [(a, values[0]), (b, values[-1])]
	for i in range(1, grid_size):
		if abs(values[i]) >= abs(values[i - 1]) and abs(values[i]) >= abs(values[i + 1]):
			candidates.append(_maximize_abs(error, xs[i - 1], xs[i], xs[i + 1]))
	candidates.sort(key=lambda item: item[0])
	return candidates


def _alternating_candidates(candidates):
	result = []
	for candidate in candidates:
		sign = mp.sign(candidate[1])
		if sign == 0:
			continue
		if result and mp.sign(result[-1][1]) == sign:
			if abs(candidate[1]) > abs(result[-1][1]):
				result[-1] = candidate
		else:
			result.append(candidate)
	return result


def _select(candidates, count):
	alternating = _alternating_candidates(candidates)
	if len(alternating) < count:
		raise RuntimeError(f"Remez found only {len(alternating)} alternating extrema; need {count}")
	if len(alternating) == count:
		return alternating
	# Any valid exchange is consecutive after equal-sign extrema have been collapsed.
	windows = [alternating[i : i + count] for i in range(len(alternating) - count + 1)]
	return max(windows, key=lambda window: min(abs(item[1]) for item in window))


def run(function, interval, powers, max_iterations, grid_size, tolerance):
	count = len(powers) + 1
	a, b = interval
	references = sorted((a + b) / 2 + (b - a) * mp.cos(mp.pi * j / (count - 1)) / 2 for j in range(count))
	previous_amplitude = None
	converged = False
	for iteration in range(1, max_iterations + 1):
		coefficients, amplitude = _solve(function, references, powers)
		all_extrema = locate_extrema(function, coefficients, interval, grid_size)
		selected = _select(all_extrema, count)
		new_references = [item[0] for item in selected]
		spread = max(abs(item[1]) for item in selected) - min(abs(item[1]) for item in selected)
		scale = max(abs(item[1]) for item in selected)
		movement = max(abs(x - y) for x, y in zip(references, new_references))
		if spread <= tolerance * max(mp.mpf(1), scale) and movement <= mp.sqrt(tolerance):
			converged = True
			references = new_references
			break
		if previous_amplitude is not None and abs(abs(amplitude) - previous_amplitude) <= tolerance * max(mp.mpf(1), abs(amplitude)) and movement <= mp.sqrt(tolerance):
			converged = True
			references = new_references
			break
		previous_amplitude = abs(amplitude)
		references = new_references
	coefficients, amplitude = _solve(function, references, powers)
	extrema = _select(locate_extrema(function, coefficients, interval, grid_size), count)
	return RemezResult(coefficients, amplitude, references, extrema, iteration, converged)
