import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from fixmath_approx.fixed_eval import div_pow2, evaluate_factored
from fixmath_approx.remez import run
from fixmath_approx.specification import FUNCTIONS, load_spec

import mpmath as mp


class FixedEvaluatorTests(unittest.TestCase):
	def test_round_to_even_signed_ties(self):
		self.assertEqual(div_pow2(3, 1, "RoundToEven"), 2)
		self.assertEqual(div_pow2(5, 1, "RoundToEven"), 2)
		self.assertEqual(div_pow2(-3, 1, "RoundToEven"), -2)
		self.assertEqual(div_pow2(-5, 1, "RoundToEven"), -2)

	def test_factored_identity(self):
		result = evaluate_factored(1 << 31, [1 << 32], 64, 32, "RoundToEven")
		self.assertEqual(result.raw, 1 << 31)
		self.assertFalse(result.overflow)
		self.assertEqual(evaluate_factored(0, [123, -456, 789], 64, 32, "RoundToEven").raw, 0)

	def test_tan_factored_target_is_regular_at_zero(self):
		self.assertEqual(FUNCTIONS["tan_over_x_squared"](mp.mpf(0)), 1)
		x = mp.mpf("0.5")
		self.assertEqual(FUNCTIONS["tan_over_x_squared"](x * x), mp.tan(x) / x)

	def test_cot_residual_factored_target_is_regular_at_zero(self):
		self.assertEqual(FUNCTIONS["cot_residual"](mp.mpf(0)), 0)
		self.assertEqual(FUNCTIONS["cot_residual_over_x_squared"](mp.mpf(0)), -mp.mpf(1) / 3)
		x = mp.mpf("0.5")
		self.assertEqual(FUNCTIONS["cot_residual_over_x_squared"](x * x), (mp.cot(x) - 1 / x) / x)

	def test_example_spec_is_explicit_and_valid(self):
		raw_spec = {
			"name": "sin_q32_32_test",
			"function": "sin_over_x_squared",
			"reference_function": "sin",
			"public_interval": ["0", "pi/4"],
			"reduced_interval": ["0", "pi^2/16"],
			"range_reduction": "none; z = round_to_even(x * x)",
			"basis": {"kind": "factored", "variable": "x_squared", "powers": [0, 1, 2, 3, 4, 5], "reconstruction": "x_times_polynomial"},
			"degree": 11,
			"error_objective": "absolute",
			"q_format": {"width": 64, "fraction_bits": 32},
			"arithmetic_mode": "SaturationMode",
			"rounding_mode": "RoundToEven",
			"precision_digits": 100,
			"accuracy": {"objective": "absolute", "unit": "ulp", "maximum": 1},
			"remez": {"max_iterations": 20, "grid_size": 8192, "tolerance": "1e-70"},
			"quantization": {"radius": 3, "passes": 3},
			"verification": {"samples": 1000001},
		}
		with tempfile.TemporaryDirectory() as directory:
			path = Path(directory) / "spec.json"
			path.write_text(json.dumps(raw_spec), encoding="utf-8")
			spec = load_spec(path)
		self.assertEqual(spec.fraction_bits, 32)
		self.assertEqual(spec.powers, (0, 1, 2, 3, 4, 5))
		self.assertLess(abs(spec.public_interval[1] - mp.pi / 4), mp.mpf("1e-90"))

	def test_remez_recovers_best_linear_approximation(self):
		mp.mp.dps = 50
		result = run(lambda x: x * x, (mp.mpf(0), mp.mpf(1)), (0, 1), 10, 256, mp.mpf("1e-35"))
		self.assertTrue(result.converged)
		self.assertLess(abs(result.coefficients[0] + mp.mpf(1) / 8), mp.mpf("1e-30"))
		self.assertLess(abs(result.coefficients[1] - 1), mp.mpf("1e-30"))


if __name__ == "__main__":
	unittest.main()
