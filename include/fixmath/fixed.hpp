/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <bit> // for std::bit_cast
#include <cstdint> // for int64_t ...
#include <string> // for std::string
#include <limits> // for std::numeric_limits
#include <ostream> // for std::basic_ostream
#include <type_traits> // for std::integral_constant, std::decay ...
#include <system_error> // for std::errc
#include <algorithm> // for std::max
#include <compare> // for std::strong_ordering
#include <climits> // for CHAR_BIT
#include "fixmath_config.hpp"
#include "fixmath_traits.inl"

namespace fixmath {

template<class policy>
class Fixed final {
public:
	using raw_t = typename policy::raw_t;
	using uraw_t = std::make_unsigned_t<raw_t>;

	constexpr const static raw_t ALL_BITS          = sizeof(raw_t) * CHAR_BIT;
	constexpr const static raw_t FRACTION_BITS     = policy::fraction_bits;
	constexpr const static raw_t INTEGER_BITS      = ALL_BITS - FRACTION_BITS;
	constexpr const static raw_t RATIO             = raw_t(1) << FRACTION_BITS;
	constexpr const static raw_t FRACTION_MASK     = raw_t(uraw_t(-1) >> INTEGER_BITS);
	constexpr const static raw_t INTEGER_MASK      = raw_t(uraw_t(-1) << FRACTION_BITS);
	constexpr const static raw_t FRACTION_MSB_MASK = raw_t(1) << (FRACTION_BITS - 1);

	using common_int32_t = std::common_type_t<raw_t, int32_t>;

	constexpr const static raw_t   MAX_REPRESENTABLE_INTEGER = std::max<raw_t>(0, (raw_t(1) << (INTEGER_BITS - 1)) - 1);
	constexpr const static int32_t MAX_REPRESENTABLE_INT32   = int32_t(std::min<common_int32_t>(MAX_REPRESENTABLE_INTEGER, std::numeric_limits<int32_t>::max()));
	constexpr const static double  MAX_REPRESENTABLE_DOUBLE  = _fm_assemble_double(ALL_BITS - 2, INTEGER_BITS - 2);
	constexpr const static float   MAX_REPRESENTABLE_FLOAT   = _fm_assemble_float(ALL_BITS - 2, INTEGER_BITS - 2);

	constexpr const static raw_t   MIN_REPRESENTABLE_INTEGER = policy::strict_mode ? -MAX_REPRESENTABLE_INTEGER : -MAX_REPRESENTABLE_INTEGER - 1;
	constexpr const static int32_t MIN_REPRESENTABLE_INT32   = int32_t(std::max<common_int32_t>(MIN_REPRESENTABLE_INTEGER, std::numeric_limits<int32_t>::min()));
	constexpr const static double  MIN_REPRESENTABLE_DOUBLE  = policy::strict_mode ? -MAX_REPRESENTABLE_DOUBLE : -_fm_assemble_double(0, INTEGER_BITS - 1);
	constexpr const static float   MIN_REPRESENTABLE_FLOAT   = policy::strict_mode ? -MAX_REPRESENTABLE_FLOAT : -_fm_assemble_float(0, INTEGER_BITS - 1);

	constexpr static Fixed epsilon() { return Fixed::from_raw(1LL); }
	constexpr static Fixed nan() { return Fixed::from_raw(std::numeric_limits<raw_t>::min()); }
	constexpr static Fixed inf() { return Fixed::from_raw(std::numeric_limits<raw_t>::max()); }
	constexpr static Fixed max_fix() { return Fixed::from_raw(std::numeric_limits<raw_t>::max() - (policy::strict_mode ? 1 : 0)); }
	constexpr static Fixed min_fix() { return Fixed::from_raw(std::numeric_limits<raw_t>::min() + (policy::strict_mode ? 2 : 0)); }
	constexpr static Fixed max_int() { return Fixed(MAX_REPRESENTABLE_INTEGER); }
	constexpr static Fixed min_int() { return Fixed(MIN_REPRESENTABLE_INTEGER); }

	static constexpr Fixed from_raw(raw_t v) { return Fixed(from_raw_t{}, v); }
	static constexpr Fixed from_raw(uraw_t v) { return Fixed(from_raw_t{}, v); }

	constexpr raw_t raw() const { return value; }

	constexpr Fixed() = default;
	constexpr Fixed(const Fixed&) = default;
	constexpr Fixed(Fixed&&) noexcept = default;

	Fixed& operator=(const Fixed&) = default;
	Fixed& operator=(Fixed&&) noexcept = default;

	explicit constexpr Fixed(float value);
	explicit constexpr Fixed(double value);
	constexpr Fixed(int32_t value);

	explicit constexpr operator bool()    const;
	explicit constexpr operator float()   const;
	explicit constexpr operator double()  const;
	explicit constexpr operator int32_t() const;

	constexpr bool is_nan() const;
	constexpr bool is_inf() const;

private:
	raw_t value = 0;
	struct from_raw_t {};
	inline constexpr Fixed(from_raw_t, raw_t v)
		: value(v)
	{}
};

using Fix32 = Fixed<fixed_policy<int64_t, 32, arithmetic_mode::SaturationMode, rounding_mode::RoundToEven>>;

} // namespace fixmath

#include "fixed_impl.inl"
