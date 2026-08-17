"""Chebyshev initialization."""

import mpmath as mp


def initial_fit(function, interval, coefficient_count):
	coefficients, error = mp.chebyfit(function, interval, coefficient_count, error=True)
	return list(reversed(coefficients)), error
