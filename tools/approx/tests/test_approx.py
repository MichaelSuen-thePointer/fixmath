import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from fixmath_approx.fixed_eval import div_pow2, evaluate_factored
from fixmath_approx.remez import run
from fixmath_approx.specification import load_spec

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

	def test_example_spec_is_explicit_and_valid(self):
		spec = load_spec(ROOT / "specs" / "sin_q32_32.json")
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
