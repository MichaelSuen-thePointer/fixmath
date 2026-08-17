/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

template <FixedPolicy policy>
constexpr fixed<policy> sqrt(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan())) {
			return fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf() && a > 0)) {
			return fixed::inf();
		}
		if (FIXMATH_UNLIKELY(a < 0)) {
			FIXMATH_ERROR("sqrt(<0)");
			return fixed::nan();
		}
	}
	if constexpr (policy::saturation_mode) {
		if (FIXMATH_UNLIKELY(a < fixed(0))) {
			FIXMATH_ERROR("sqrt(<0)");
			return fixed::min_sat();
		}
	}
	if (a == 0) {
		return 0;
	}
	uraw_t value = a.uraw();
	if constexpr (fixed::FRACTION_BITS & 1) {
		value <<= 1;
	}
	uraw_t root = 0;
	uraw_t remainder = 0;
	raw_t start_i = _fm_clz(value) >> 1;
	value <<= (start_i << 1);
	for (raw_t i = start_i; i < fixed::ALL_BITS / 2; ++i) {
		root <<= 1;
		remainder = (remainder << 2) | (value >> (fixed::ALL_BITS - 2));
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			root |= 1;
			remainder -= tester;
		}
		value <<= 2;
	}
	for (raw_t i = 0; i < fixed::FRACTION_BITS / 2; ++i) {
		root <<= 1;
		remainder <<= 2;
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			root |= 1;
			remainder -= tester;
		}
	}
	uraw_t result = root;
	if constexpr (policy::rounding) {
		root <<= 1;
		remainder <<= 2;
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			if (remainder == tester) {
				result += result & 1;
			} else {
				result += 1;
			}
		}
	}
	if constexpr (!policy::ignore_mode) {
		if (FIXMATH_UNLIKELY(result > static_cast<uraw_t>(fixed::max_sat().raw()))) {
			return fixed::max_sat();
		}
	}
	return fixed::from_raw(result);
}

} // namespace fixmath
