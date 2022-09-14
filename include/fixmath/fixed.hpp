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
#include "fixmath_bitcast.inl"

namespace fixmath {

template<FixedPolicy _policy>
class fixed final {
public:
	using policy = _policy;
	using raw_t = typename policy::raw_t;
	using uraw_t = std::make_unsigned_t<raw_t>;

	constexpr const static raw_t ALL_BITS          = sizeof(raw_t) * CHAR_BIT;
	constexpr const static raw_t FRACTION_BITS     = policy::fraction_bits;
	constexpr const static raw_t INTEGER_BITS      = ALL_BITS - FRACTION_BITS;
	constexpr const static raw_t RATIO             = raw_t(1) << FRACTION_BITS;
	constexpr const static raw_t FRACTION_MASK     = raw_t(uraw_t(-1) >> INTEGER_BITS);
	constexpr const static raw_t INTEGER_MASK      = raw_t(uraw_t(-1) << FRACTION_BITS);
	constexpr const static raw_t FRACTION_MSB_MASK = raw_t(1) << (FRACTION_BITS - 1);

	using common_int32_t = ::std::common_type_t<raw_t, int32_t>;
	using ordering_t = ::std::conditional_t<policy::strict_mode, ::std::partial_ordering, ::std::strong_ordering>;

	constexpr const static raw_t   MAX_REPRESENTABLE_INTEGER = ::std::max<raw_t>(0, (raw_t(1) << (INTEGER_BITS - 1)) - 1);
	constexpr const static int32_t MAX_REPRESENTABLE_INT32   = int32_t(::std::min<common_int32_t>(MAX_REPRESENTABLE_INTEGER, ::std::numeric_limits<int32_t>::max()));
	constexpr const static double  MAX_REPRESENTABLE_DOUBLE  = _fm_assemble_double(ALL_BITS - 2, INTEGER_BITS - 2);
	constexpr const static float   MAX_REPRESENTABLE_FLOAT   = _fm_assemble_float(ALL_BITS - 2, INTEGER_BITS - 2);

	constexpr const static raw_t   MIN_REPRESENTABLE_INTEGER = policy::strict_mode ? -MAX_REPRESENTABLE_INTEGER : -MAX_REPRESENTABLE_INTEGER - 1;
	constexpr const static int32_t MIN_REPRESENTABLE_INT32   = int32_t(::std::max<common_int32_t>(MIN_REPRESENTABLE_INTEGER, ::std::numeric_limits<int32_t>::min()));
	constexpr const static double  MIN_REPRESENTABLE_DOUBLE  = policy::strict_mode ? -MAX_REPRESENTABLE_DOUBLE : -_fm_assemble_double(0, INTEGER_BITS - 1);
	constexpr const static float   MIN_REPRESENTABLE_FLOAT   = policy::strict_mode ? -MAX_REPRESENTABLE_FLOAT : -_fm_assemble_float(0, INTEGER_BITS - 1);

	constexpr static fixed epsilon() { return fixed::from_raw(1LL); }
	constexpr static fixed nan() { return fixed::from_raw(::std::numeric_limits<raw_t>::min()); }
	constexpr static fixed inf() { return fixed::from_raw(::std::numeric_limits<raw_t>::max()); }
	constexpr static fixed max_sat() { return fixed::from_raw(::std::numeric_limits<raw_t>::max()); }
	constexpr static fixed min_sat() { return fixed::from_raw(::std::numeric_limits<raw_t>::min() + (policy::strict_mode ? 1 : 0)); }
	constexpr static fixed max_fix() { return fixed::from_raw(::std::numeric_limits<raw_t>::max() - (policy::strict_mode ? 1 : 0)); }
	constexpr static fixed min_fix() { return fixed::from_raw(::std::numeric_limits<raw_t>::min() + (policy::strict_mode ? 2 : 0)); }
	constexpr static fixed max_int() { return fixed(MAX_REPRESENTABLE_INTEGER); }
	constexpr static fixed min_int() { return fixed(MIN_REPRESENTABLE_INTEGER); }

	static constexpr fixed from_raw(raw_t v) { return fixed(from_raw_t{}, v); }
	static constexpr fixed from_raw(uraw_t v) { return fixed(from_raw_t{}, v); }

	constexpr raw_t raw() const { return value; }

	constexpr fixed() = default;
	constexpr fixed(const fixed&) = default;
	constexpr fixed(fixed&&) noexcept = default;

	fixed& operator=(const fixed&) = default;
	fixed& operator=(fixed&&) noexcept = default;

	explicit constexpr fixed(float value);
	explicit constexpr fixed(double value);
	constexpr fixed(int32_t value);

	explicit constexpr operator bool()    const;
	explicit constexpr operator float()   const;
	explicit constexpr operator double()  const;
	explicit constexpr operator int32_t() const;

	constexpr bool is_nan() const;
	constexpr bool is_inf() const;

private:
	raw_t value = 0;
	struct from_raw_t {};
	inline constexpr fixed(from_raw_t, raw_t v)
		: value(v)
	{}
};

using Fix32 = fixed<fixed_policy<int64_t, 32, arithmetic_mode::SaturationMode, rounding_mode::RoundToEven>>;

} // namespace fixmath

#include "fixed_impl.inl"
