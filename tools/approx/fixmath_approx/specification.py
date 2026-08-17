"""Specification loading and reviewed reference-function registry."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

import mpmath as mp


def _sin_over_x_squared(z: mp.mpf) -> mp.mpf:
	if z == 0:
		return mp.mpf(1)
	x = mp.sqrt(z)
	return mp.sin(x) / x


FUNCTIONS: dict[str, Callable[[mp.mpf], mp.mpf]] = {
	"sin": mp.sin,
	"sin_over_x_squared": _sin_over_x_squared,
}


def parse_number(value: Any) -> mp.mpf:
	"""Parse a JSON number without evaluating arbitrary expressions."""
	if isinstance(value, (int, float)):
		return mp.mpf(str(value))
	constants = {"pi": mp.pi, "pi/2": mp.pi / 2, "pi/4": mp.pi / 4, "pi^2/16": mp.pi**2 / 16}
	if value in constants:
		return constants[value]
	return mp.mpf(value)


@dataclass(frozen=True)
class ApproximationSpec:
	raw: dict[str, Any]
	name: str
	target_name: str
	reference_name: str
	interval: tuple[mp.mpf, mp.mpf]
	public_interval: tuple[mp.mpf, mp.mpf]
	powers: tuple[int, ...]
	width: int
	fraction_bits: int
	rounding: str
	arithmetic: str
	precision_digits: int
	target_ulp: int
	remez_max_iterations: int
	remez_grid_size: int
	remez_tolerance: mp.mpf
	quantize_radius: int
	quantize_passes: int
	verification_samples: int

	@property
	def target(self) -> Callable[[mp.mpf], mp.mpf]:
		return FUNCTIONS[self.target_name]

	@property
	def reference(self) -> Callable[[mp.mpf], mp.mpf]:
		return FUNCTIONS[self.reference_name]

	@property
	def scale(self) -> int:
		return 1 << self.fraction_bits


def load_spec(path: Path) -> ApproximationSpec:
	raw = json.loads(path.read_text(encoding="utf-8"))
	required = {"name", "function", "reference_function", "public_interval", "reduced_interval", "range_reduction", "basis", "degree", "error_objective", "q_format", "arithmetic_mode", "rounding_mode", "precision_digits", "accuracy", "remez", "quantization", "verification"}
	missing = required - raw.keys()
	if missing:
		raise ValueError(f"missing specification fields: {', '.join(sorted(missing))}")
	# Numeric strings and named constants must be materialized at generator precision,
	# not at mpmath's process default precision.
	mp.mp.dps = int(raw["precision_digits"])
	if raw["function"] not in FUNCTIONS or raw["reference_function"] not in FUNCTIONS:
		raise ValueError("function is not present in the reviewed registry")
	basis = raw["basis"]
	if basis.get("kind") != "factored" or basis.get("variable") != "x_squared" or basis.get("reconstruction") != "x_times_polynomial":
		raise ValueError("the first implementation supports x * q(x^2) only")
	powers = tuple(basis["powers"])
	if not powers or powers != tuple(range(len(powers))):
		raise ValueError("basis powers must be consecutive and start at zero")
	q_format = raw["q_format"]
	if q_format["width"] not in (32, 64) or not 0 < q_format["fraction_bits"] < q_format["width"] - 1:
		raise ValueError("unsupported Q format")
	if raw["rounding_mode"] not in ("RoundToEven", "RoundToZero"):
		raise ValueError("unsupported rounding mode")
	if raw["arithmetic_mode"] not in ("Ignore", "SaturationMode", "StrictMode"):
		raise ValueError("unsupported arithmetic mode")
	interval = tuple(parse_number(v) for v in raw["reduced_interval"])
	public_interval = tuple(parse_number(v) for v in raw["public_interval"])
	maximum_public_raw = mp.nint(public_interval[1] * (1 << q_format["fraction_bits"]))
	if maximum_public_raw > (1 << (q_format["width"] - 1)) - 1:
		raise ValueError("public interval is outside the requested raw format")
	if len(interval) != 2 or interval[0] < 0 or interval[0] >= interval[1]:
		raise ValueError("invalid reduced interval")
	if interval[0] != public_interval[0] ** 2 or not mp.almosteq(interval[1], public_interval[1] ** 2):
		raise ValueError("reduced interval must equal the square of the public interval")
	if int(raw["degree"]) != 2 * powers[-1] + 1:
		raise ValueError("degree does not match the factored basis")
	if raw["accuracy"].get("unit") != "ulp" or raw["accuracy"].get("objective") != "absolute":
		raise ValueError("the first implementation accepts an absolute ULP target")
	return ApproximationSpec(
		raw=raw,
		name=raw["name"], target_name=raw["function"], reference_name=raw["reference_function"],
		interval=(interval[0], interval[1]), public_interval=(public_interval[0], public_interval[1]), powers=powers,
		width=q_format["width"], fraction_bits=q_format["fraction_bits"], rounding=raw["rounding_mode"], arithmetic=raw["arithmetic_mode"],
		precision_digits=int(raw["precision_digits"]), target_ulp=int(raw["accuracy"]["maximum"]),
		remez_max_iterations=int(raw["remez"]["max_iterations"]), remez_grid_size=int(raw["remez"]["grid_size"]), remez_tolerance=parse_number(raw["remez"]["tolerance"]),
		quantize_radius=int(raw["quantization"]["radius"]), quantize_passes=int(raw["quantization"]["passes"]), verification_samples=int(raw["verification"]["samples"]),
	)
