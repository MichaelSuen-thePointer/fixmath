/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

#if FIXMATH_WIN_X64
#  include <intrin.h>
#endif
#include "fixmath_speedups.inl"
#include "fixmath_clz.inl"
#include "fixmath_muldiv128.inl"

namespace fixmath {

template<class policy>
constexpr Fixed<policy>::Fixed(float value)
    : value(
        value != value
            ? nan().raw()
        : value > MAX_REPRESENTABLE_FLOAT
            ? inf().raw()
        : value < MIN_REPRESENTABLE_FLOAT
            ? -inf().raw()
        : policy::rounding
            ? raw_t((value * RATIO) + (value >= 0 ? 0.5f : -0.5f))
        : raw_t(value * RATIO)
    )
{}

template<class policy>
constexpr Fixed<policy>::Fixed(double value)
    : value(
        value != value
            ? nan().raw()
        : value > MAX_REPRESENTABLE_DOUBLE
            ? inf().raw()
        : value < MIN_REPRESENTABLE_DOUBLE
            ? -inf().raw()
        : policy::rounding
            ? raw_t((value * RATIO) + (value >= 0 ? 0.5 : -0.5))
        : raw_t(value * RATIO)
    )
{}

template<class policy>
constexpr Fixed<policy>::Fixed(int32_t value)
    : value(
        value > MAX_REPRESENTABLE_INT32
            ? inf().raw()
        : value < MIN_REPRESENTABLE_INT32
            ? -inf().raw()
        : value * RATIO
    )
{}


template<class policy>
constexpr Fixed<policy>::operator bool() const {
	return !!value;
}

template<class policy>
constexpr Fixed<policy>::operator float() const {
	if constexpr (policy::strict_mode) {
		if (is_nan()) {
			return ::std::numeric_limits<float>::quiet_NaN();
		}
		if (is_inf()) {
			return value > 0 ? ::std::numeric_limits<float>::infinity() : -::std::numeric_limits<float>::infinity();
		}
	}
	return static_cast<float>(value) / RATIO;
}

template<class policy>
constexpr Fixed<policy>::operator double() const {
	if constexpr (policy::strict_mode) {
		if (is_nan()) {
			return ::std::numeric_limits<double>::quiet_NaN();
		}
		if (is_inf()) {
			return value > 0 ? ::std::numeric_limits<double>::infinity() : -::std::numeric_limits<double>::infinity();
		}
	}
	return static_cast<double>(value) / RATIO;
}

template<class policy>
constexpr Fixed<policy>::operator int32_t() const {
	raw_t result = value / RATIO;
	if constexpr (policy::saturation_mode) {
		if (result > MAX_REPRESENTABLE_INT32) {
			return MAX_REPRESENTABLE_INT32;
		}
		if (result < MIN_REPRESENTABLE_INT32) {
			return MIN_REPRESENTABLE_INT32;
		}
	}
	if constexpr (policy::strict_mode) {
		FIXMATH_ASSERT(!is_nan() && !is_inf(), "value cannot be represented in int32_t");
		FIXMATH_ASSERT(MIN_REPRESENTABLE_INT32 <= result && result <= MAX_REPRESENTABLE_INT32, "value out of range");
	}
	return static_cast<int32_t>(result);
}

template<class policy>
constexpr bool Fixed<policy>::is_nan() const {
	if constexpr (policy::strict_mode) {
		return value == nan().raw();
	} else {
		return false;
	}
}

template<class policy>
constexpr bool Fixed<policy>::is_inf() const {
	if constexpr (policy::strict_mode) {
		return abs(value) == inf().raw();
	} else {
		return false;
	}
}

template<class policy>
constexpr Fixed<policy> operator+(Fixed<policy> a, Fixed<policy> b) {
	using Fixed = Fixed<policy>;
	using raw_t = typename Fixed::raw_t;
	using uraw_t = typename Fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || b.is_nan())) {
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf() && b.is_inf())) {
			return a == b ? a : Fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf())) {
			return a;
		}
		if (FIXMATH_UNLIKELY(b.is_inf())) {
			return b;
		}
	}
	raw_t r = 0;
	if constexpr (policy::ignore_mode) {
		r = raw_t(uraw_t(a.raw()) + uraw_t(b.raw()));
	} else {
		bool overflow = false;
		r = _fm_checked_add(a.raw(), b.raw(), overflow);
		if (FIXMATH_UNLIKELY(overflow)) {
			// same check in strict_mode or saturate_mode
			return r > 0 ? -Fixed::inf() : Fixed::inf();
		}
		if constexpr (policy::strict_mode) {
			if (FIXMATH_UNLIKELY(r == Fixed::nan().raw())) {
				return -Fixed::inf();
			}
		}
	}
	return Fixed::from_raw(r);
}

template<class policy>
constexpr Fixed<policy> operator-(Fixed<policy> a, Fixed<policy> b) {
	using Fixed = Fixed<policy>;
	using raw_t = typename Fixed::raw_t;
	using uraw_t = typename Fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || b.is_nan())) {
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf() && b.is_inf())) {
			return a == b ? Fixed::nan() : a;
		}
		if (FIXMATH_UNLIKELY(a.is_inf())) {
			return a;
		}
		if (FIXMATH_UNLIKELY(b.is_inf())) {
			return -b;
		}
	}
	raw_t r = 0;
	if constexpr (policy::ignore_mode) {
		r = raw_t(uraw_t(a.raw()) - uraw_t(b.raw()));
	} else {
		bool overflow = false;
		r = _fm_checked_sub(a.raw(), b.raw(), overflow);
		if (FIXMATH_UNLIKELY(overflow)) {
			// same check in strict_mode or saturate_mode
			return r > 0 ? -Fixed::inf() : Fixed::inf();
		}
		if constexpr (policy::strict_mode) {
			if (FIXMATH_UNLIKELY(r == Fixed::nan().raw())) {
				return -Fixed::inf();
			}
		}
	}
	return Fixed::from_raw(r);
}

template<class policy>
constexpr Fixed<policy> operator*(Fixed<policy> a, Fixed<policy> b) {
	using Fixed = Fixed<policy>;
	using raw_t = typename Fixed::raw_t;
	using uraw_t = typename Fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || b.is_nan())) {
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY((a.raw() == 0 && b.is_inf()) || (a.is_inf() && b.raw() == 0))) {
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf() || b.is_inf())) {
			return (a.raw() ^ b.raw()) >= 0 ? Fixed::inf() : -Fixed::inf();
		}
	}
	if constexpr (sizeof(raw_t) == 8) {
		raw_t r = 0;
		// use extended 128bit multiplication
		if (FIXMATH_LIKELY(static_cast<int32_t>(a.raw()) == a.raw() && static_cast<int32_t>(b.raw()) == b.raw())) {
			r = a.raw() * b.raw();
			r = _fm_div2n_round<policy, Fixed::FRACTION_BITS>(r);
		} else {
			raw_t rhi = 0;
			r = _fm_mul128(a.raw(), b.raw(), rhi);
			r = _fm_div2n_round<policy, Fixed::FRACTION_BITS>(rhi, r, rhi);
			if constexpr (!policy::ignore_mode) {
				// check overfow
				if (FIXMATH_UNLIKELY(rhi != (r >> 63))) {
					return rhi > 0 ? Fixed::inf() : -Fixed::inf();
				}
				if constexpr (policy::strict_mode) {
					if (FIXMATH_UNLIKELY(r == Fixed::nan().raw())) {
						return -Fixed::inf();
					}
				}
			}
		}
		return Fixed::from_raw(r);
	} else {
		int64_t r64 = a.raw();
		r64 *= b.raw();
		r64 = _fm_div2n_round<policy, Fixed::FRACTION_BITS>(r64);
		if constexpr (!policy::ignore_mode) {
			if (FIXMATH_UNLIKELY(r64 > Fixed::inf().raw())) {
				return Fixed::inf();
			} else if (FIXMATH_UNLIKELY(r64 < -Fixed::inf().raw())) {
				return -Fixed::inf();
			}
		}
		return Fixed::from_raw(raw_t(r64));
	}
}

template<class policy>
constexpr Fixed<policy> operator/(Fixed<policy> a, Fixed<policy> b) {
	using Fixed = Fixed<policy>;
	using raw_t = typename Fixed::raw_t;
	using uraw_t = typename Fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || b.is_nan())) {
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY((a.raw() == 0 && b.raw() == 0) || (a.is_inf() && b.is_inf()))) {
			FIXMATH_ASSERT(b.raw() != 0, "division by 0");
			return Fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf())) {
			return b.raw() >= 0 ? a : -a;
		}
		if (FIXMATH_UNLIKELY(b.is_inf())) {
			return 0;
		}
		if (FIXMATH_UNLIKELY(b.raw() == 0)) {
			FIXMATH_ERROR("division by 0");
			return a.raw() > 0 ? Fixed::inf() : -Fixed::inf();
		}
	}
	if constexpr (policy::saturation_mode) {
		if (FIXMATH_UNLIKELY(b.raw() == 0)) {
			FIXMATH_ERROR("division by 0");
			if (a.raw() == 0) {
				return Fixed::nan();
			} else if (a.raw() > 0) {
				return Fixed::inf();
			} else {
				return -Fixed::inf();
			}
		}
	}
	int64_t qhi = 0;
	int64_t qlo = 0;
	int64_t rem = 0;
	if constexpr (sizeof(raw_t) == 8) {
		// use extended int128 division
		raw_t _check_bits = a.raw() >> (Fixed::ALL_BITS - Fixed::FRACTION_BITS - 1); // check for simple division
		if (FIXMATH_LIKELY(_check_bits == 0 || _check_bits == -1)) {
			qlo = a.raw() * Fixed::RATIO;
			if constexpr (policy::rounding) {
				rem = qlo % b.raw();
			}
			qlo /= b.raw();
			qhi = qlo >> 63;
		} else {
			_int128_s _r = {};
			if constexpr (Fixed::FRACTION_BITS == 32) {
				// speed up for common cases
				_r = _fm_shl32div(a.raw(), b.raw(), rem);
			} else {
				_r = _fm_div128(a.raw() >> (Fixed::ALL_BITS - Fixed::FRACTION_BITS), a.raw() << Fixed::FRACTION_BITS, b.raw(), rem);
			}
			qlo = _r.lo;
			qhi = _r.hi;
		}
	} else {
		qlo = a.raw();
		qlo *= Fixed::RATIO;
		if constexpr (policy::rounding) {
			rem = qlo % b.raw();
		}
		qlo /= b.raw();
		qhi = qlo >> 63;
	}
	if constexpr (policy::rounding) {
		uint64_t abs_rem = static_cast<uint64_t>(rem);
		if (rem < 0) {
			abs_rem = 0 - abs_rem;
		}
		uint64_t abs_b = static_cast<uint64_t>(b.raw());
		if (b.raw() < 0) {
			abs_b = 0 - abs_b;
		}
		bool carry1 = (abs_rem > abs_b / 2);
		bool carry2 = (abs_rem == abs_b / 2) & !(abs_b & 1) & (qlo & 1);
		if (FIXMATH_LIKELY(carry1 | carry2)) {
			uint64_t ucarry = qhi < 0 ? -1 : 1;
			int testhi = qhi < 0 ? -1 : 0;
			qlo = static_cast<uint64_t>(qlo) + ucarry;
			qhi = static_cast<uint64_t>(qhi) + (qlo == testhi ? ucarry : 0);
		}
	}
	if constexpr (!policy::ignore_mode) {
		if (FIXMATH_UNLIKELY(qhi != (qlo >> 63))) {
			return qhi >= 0 ? Fixed::inf() : -Fixed::inf();
		}
		if (FIXMATH_UNLIKELY(qlo > Fixed::inf().raw())) {
			return Fixed::inf();
		} else if (FIXMATH_UNLIKELY(qlo < -Fixed::inf().raw())) {
			return -Fixed::inf();
		}
	}
	return Fixed::from_raw(raw_t(qlo));
}

template<class policy>
constexpr Fixed<policy> operator+(int a, Fixed<policy> b) {
	return Fixed<policy>(a) + b;
}

template<class policy>
constexpr Fixed<policy> operator-(int a, Fixed<policy> b) {
	return Fixed<policy>(a) - b;
}

template<class policy>
constexpr Fixed<policy> operator*(int a, Fixed<policy> b) {
	return Fixed<policy>(a) * b;
}

template<class policy>
constexpr Fixed<policy> operator/(int a, Fixed<policy> b) {
	return Fixed<policy>(a) / b;
}

template<class policy>
constexpr Fixed<policy> operator+(Fixed<policy> a, int b) {
	return a + Fixed<policy>(b);
}

template<class policy>
constexpr Fixed<policy> operator-(Fixed<policy> a, int b) {
	return a - Fixed<policy>(b);
}

template<class policy>
constexpr Fixed<policy> operator*(Fixed<policy> a, int b) {
	return a * Fixed<policy>(b);
}

template<class policy>
constexpr Fixed<policy> operator/(Fixed<policy> a, int b) {
	return a / Fixed<policy>(b);
}

template<class policy>
constexpr Fixed<policy> operator+(Fixed<policy> a) {
	return a;
}

template<class policy>
constexpr Fixed<policy> operator-(Fixed<policy> a) {
	using Fixed = Fixed<policy>;
	using raw_t = typename Fixed::raw_t;
	using uraw_t = typename Fixed::uraw_t;
	return Fixed::from_raw(raw_t(0 - uraw_t(a.raw())));
}

template<class policy>
constexpr bool operator!(Fixed<policy> a) {
	return !a.raw();
}

template<class policy>
constexpr ::std::conditional_t<policy::strict_mode, ::std::partial_ordering, ::std::strong_ordering>
operator<=>(Fixed<policy> a, Fixed<policy> b) {
	if constexpr (policy::strict_mode) {
		return FIXMATH_UNLIKELY(a.is_nan() || b.is_nan())
			? ::std::partial_ordering::unordered
		: a.raw() > b.raw()
			? ::std::partial_ordering::greater
		: a.raw() < b.raw()
			? ::std::partial_ordering::less
		: ::std::partial_ordering::equivalent;
	} else {
		return a.raw() == b.raw()
			? ::std::strong_ordering::equal
		: a.raw() > b.raw()
			? ::std::strong_ordering::greater
		: ::std::strong_ordering::less;
	}
}

template<class policy>
constexpr ::std::conditional_t<policy::strict_mode, ::std::partial_ordering, ::std::strong_ordering>
operator<=>(int a, Fixed<policy> b) {
	return Fixed<policy>(a) <=> b;
}

template<class policy>
constexpr ::std::conditional_t<policy::strict_mode, ::std::partial_ordering, ::std::strong_ordering>
operator<=>(Fixed<policy> a, int b) {
	return a <=> Fixed<policy>(b);
}

template<class policy>
constexpr bool operator==(Fixed<policy> a, Fixed<policy> b) {
	return (a <=> b) == 0;
}

template<class policy>
constexpr bool operator==(int a, Fixed<policy> b) {
	return (a <=> b) == 0;
}

template<class policy>
constexpr bool operator==(Fixed<policy> a, int b) {
	return (a <=> b) == 0;
}

template<class policy>
constexpr bool operator!=(Fixed<policy> a, Fixed<policy> b) {
	return (a <=> b) != 0;
}

template<class policy>
constexpr bool operator!=(int a, Fixed<policy> b) {
	return (a <=> b) != 0;
}

template<class policy>
constexpr bool operator!=(Fixed<policy> a, int b) {
	return (a <=> b) != 0;
}


} // namespace fixmath

namespace std {

template<class policy>
class numeric_limits<fixmath::Fixed<policy>>
{
    using Fixed = fixmath::Fixed<policy>;
public:
    static constexpr bool is_specialized = true;

    static constexpr Fixed
    min() noexcept { return Fixed::epsilon(); }

    static constexpr Fixed
    max() noexcept { return Fixed::max_fix(); }

    static constexpr Fixed
    lowest() noexcept { return Fixed::min_fix(); }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr int radix = 2;

    static constexpr Fixed
    epsilon() noexcept { return Fixed::epsilon(); }

    static constexpr Fixed
    round_error() noexcept { return policy::rounding ? Fixed(0.5) : Fixed(1); }

    static constexpr bool has_infinity = policy::strict_mode;
    static constexpr bool has_quiet_NaN = policy::strict_mode;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;

    static constexpr Fixed
    infinity() noexcept
    { return Fixed::inf(); }

    static constexpr Fixed
    quiet_NaN() noexcept
    { return Fixed::nan(); }

    static constexpr Fixed
    signaling_NaN() noexcept
    { return {}; }

    static constexpr Fixed
    denorm_min() noexcept
    { return {}; }

    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;

    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;
    static constexpr float_round_style round_style = policy::rounding ? round_to_nearest : round_toward_zero;
};

} // namespace std
