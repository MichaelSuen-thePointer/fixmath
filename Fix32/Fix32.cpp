#include "Fix32.hpp"
#include <tuple>
#include <algorithm>
#include <limits>
#include <stdexcept>

#ifdef _MSC_VER
#  include <intrin.h>
#  ifdef _WIN64
int clzll(uint64_t value)
{
	unsigned long leading_zero = 0;
	if (_BitScanReverse64(&leading_zero, value))
	{
		return 63 - leading_zero;
	}
	else
	{
		// Same remarks as above
		return 64;
	}
}
#  elif _WIN32
int clzll(uint64_t value)
{
	unsigned long leading_zero = 0;
	if (_BitScanReverse(&leading_zero, (unsigned long)(value >> 32)))
	{
		return 31 - leading_zero;
	}
	else if (_BitScanReverse(&leading_zero, (unsigned long)value))
	{
		return 63 - leading_zero;
	}
	else
	{
		// Same remarks as above
		return 64;
	}
}
#  else
#    error "platform not supported"
#  endif
#else
int clzll(uint64_t value)
{
	return __builtin_clzll(value);
}
#endif

using std::pair;
using std::tuple;

pair<uint64_t, uint64_t> negate(int64_t high, uint64_t low)
{
	auto old_l = ~low;
	auto l = old_l + 1;
	auto h = ~(uint64_t)high;
	h += l < old_l;
	return { h, l };
}

pair<uint64_t, uint64_t> abs(int64_t high, uint64_t low)
{
	if (high < 0)
	{
		return negate(high, low);
	}
	return { (uint64_t)high, low };
}

tuple<int64_t, uint64_t, int64_t> int128_div_rem(int64_t high, uint64_t low, int64_t divisor)
{
	auto[abs_high, abs_low] = abs(high, low);
	auto abs_divisor = abs(divisor);

	auto quotient_high = abs_high / abs_divisor;
	auto remainder_high = abs_high % abs_divisor;
	auto msb = clzll(remainder_high);
	uint64_t quotient_low = 0;
	auto copy_low = abs_low;
	auto copy_rem = remainder_high;
	int bits_remain = 64;
	while (bits_remain)
	{
		auto new_dividend = (copy_rem << msb) | (copy_low >> (64 - msb));
		auto new_quo = new_dividend / abs_divisor;
		copy_rem = new_dividend % abs_divisor;
		bits_remain -= msb;
		quotient_low |= new_quo << bits_remain;
		copy_low <<= msb;
		msb = std::min(bits_remain, clzll(copy_rem));
	}
	if ((high < 0 && divisor < 0) || (high > 0 && divisor > 0))
	{
		return { quotient_high, quotient_low, (int64_t)copy_rem };
	}
	if (high < 0)
	{
		return tuple_cat(negate(quotient_high, quotient_low), std::make_tuple(-(int64_t)copy_rem));
	}
	if (divisor < 0)
	{
		return tuple_cat(negate(quotient_high, quotient_low), std::make_tuple((int64_t)copy_rem));
	}
	return { quotient_high, quotient_low, (int64_t)copy_rem };
}

pair<int64_t, uint64_t> int128_mul(int64_t _a, int64_t _b)
{
	int64_t a = abs(_a);
	int64_t b = abs(_b);
	using int_limits = std::numeric_limits<int32_t>;
	uint64_t rhi, rlo;
	if (a <= int_limits::max() && b <= int_limits::max())
	{
		rhi = 0;
		rlo = a * b;
	}
	else if ((a & 0xFFFFFFFFu) == 0 && (b & 0xFFFFFFFFu) == 0)
	{
		rhi = (a >> 32) * (b >> 32);
		rlo = 0;
	}
	else
	{
		uint64_t ahi = (uint64_t)a >> 32;
		uint64_t alo = (uint32_t)a;
		uint64_t bhi = (uint64_t)b >> 32;
		uint64_t blo = (uint32_t)b;

		auto alo_blo = alo * blo;
		auto alo_bhi = alo * bhi;
		auto ahi_blo = ahi * blo;
		auto ahi_bhi = ahi * bhi;

		auto ahi_blo_p_alo_bhi = ahi_blo;
		ahi_blo_p_alo_bhi += alo_bhi;
		auto carry1 = ahi_blo_p_alo_bhi < ahi_blo;

		rhi = ahi_bhi + ((uint64_t)carry1 << 32);

		rlo = alo_blo;
		rlo += ahi_blo_p_alo_bhi << 32;
		auto carry2 = rlo < alo_blo;

		rhi += carry2 + (ahi_blo_p_alo_bhi >> 32);
	}
	if (_a < 0 && _b < 0 || _a > 0 && _b > 0)
	{
		return { rhi, rlo };
	}
	return negate(rhi, rlo);
}

int64_t shl32_div(int64_t a, int64_t b)
{
	auto[hi, lo, rem] = int128_div_rem(a >> 32, a << 32, b);
	(void)hi;
	(void)rem;
	return lo;
}

int64_t mul_shr32(int64_t a, int64_t b)
{
	auto[hi, lo] = int128_mul(a, b);
	return (hi << 32) | (lo >> 32);
}

Fix32 operator+(Fix32 a, Fix32 b)
{
	return Fix32::from_raw(a._value + b._value);
}

Fix32 operator-(Fix32 a, Fix32 b)
{
	return Fix32::from_raw(a._value - b._value);
}

Fix32 operator*(Fix32 a, Fix32 b)
{
	if (a == 0 || b == 0)
	{
		return { 0 };
	}
	if (uint64_t abs_value = abs(b._value); (abs_value & (abs_value - 1)) == 0)
	{
		auto shamt = 31 - clzll(abs_value);
		auto r_value = a._value;
		if (shamt > 0)
		{
			r_value <<= shamt;
		}
		else
		{
			r_value >>= -shamt;
		}
		return Fix32::from_raw((b._value < 0 ? -1 : 1) * r_value);
	}
	return Fix32::from_raw(mul_shr32(a._value, b._value));
}

Fix32 operator/(Fix32 a, Fix32 b)
{
	if (a == 0)
	{
		return { 0 };
	}
	if (b == 0)
	{
		throw std::runtime_error("division by 0");
	}
	if (uint64_t abs_value = abs(b._value); (abs_value & (abs_value - 1)) == 0)
	{
		auto shamt = 31 - clzll(abs_value);
		auto r_value = a._value;
		if (shamt > 0)
		{
			r_value >>= shamt;
		}
		else
		{
			r_value <<= -shamt;
		}
		return Fix32::from_raw((b._value < 0 ? -1 : 1) * r_value);
	}
	return Fix32::from_raw(shl32_div(a._value, b._value));
}

Fix32& operator+=(Fix32& a, Fix32 b)
{
	return a = a + b;
}

Fix32& operator-=(Fix32& a, Fix32 b)
{
	return a = a - b;
}

Fix32& operator*=(Fix32& a, Fix32 b)
{
	return a = a * b;
}

Fix32& operator/=(Fix32& a, Fix32 b)
{
	return a = a / b;
}

bool operator>(Fix32 a, Fix32 b)
{
	return a.compare(b) > 0;
}

bool operator>=(Fix32 a, Fix32 b)
{
	return a.compare(b) >= 0;
}

bool operator<(Fix32 a, Fix32 b)
{
	return a.compare(b) < 0;
}

bool operator<=(Fix32 a, Fix32 b)
{
	return a.compare(b) <= 0;
}

bool operator==(Fix32 a, Fix32 b)
{
	return a.compare(b) == 0;
}

bool operator!=(Fix32 a, Fix32 b)
{
	return a.compare(b) != 0;
}

Fix32 Fix32::from_integer(int value)
{
	return Fix32(value);
}

Fix32 Fix32::from_integer(uint32_t value)
{
	return Fix32(value);
}

Fix32 Fix32::from_integer(int64_t value)
{
	return Fix32(value);
}

Fix32 Fix32::from_integer(uint64_t value)
{
	return Fix32(value);
}

Fix32 Fix32::from_real(float value)
{
	using flimits = std::numeric_limits<float>;
	if (value != value)
	{
		throw std::runtime_error("value is NAN");
	}
	if (value == 0)
	{
		return { 0 };
	}
	if (value == 1)
	{
		return { 1 };
	}
	if (value == -1)
	{
		return { -1 };
	}
	if (value == flimits::infinity())
	{
		return 0;
	}
	uint32_t ivalue = reinterpret_cast<uint32_t&>(value);
	bool sign = !!(ivalue >> 31);
	int32_t exp = (ivalue >> 23) & 0xFF;
	uint32_t frac = ivalue & 0x7F'FFFF;
	if (exp != 0)
	{
		exp -= 127;
		frac |= 0x80'0000;
		int64_t raw = frac;
		auto shift_amount = 32 - 23 + exp;
		if (shift_amount > 63 || shift_amount < -63)
		{
			return { 0 };
		}
		raw = shift_amount < 0 ? raw >> -shift_amount : raw << shift_amount;
		return Fix32::from_raw(sign ? -raw : raw);
	}
	return { 0 };
}

Fix32 Fix32::from_real(double value)
{
	if (value != value)
	{
		throw std::runtime_error("value is NAN");
	}
	if (value == 0)
	{
		return { 0 };
	}
	if (value == 1)
	{
		return { 1 };
	}
	if (value == -1)
	{
		return { -1 };
	}

	uint64_t ivalue = reinterpret_cast<uint64_t&>(value);
	bool sign = !!(ivalue >> 63);
	int32_t exp = (ivalue >> 53) & 0x7FF;
	uint64_t frac = ivalue & 0xF'FFFF'FFFF'FFFFull;
	if (exp != 0)
	{
		exp -= 1023;
		frac |= 0x10'0000'0000'0000;
		int64_t raw = frac;
		auto shift_amount = 32 - 52 + exp;
		if (shift_amount > 63 || shift_amount < -63)
		{
			return { 0 };
		}
		raw = shift_amount < 0 ? raw >> -shift_amount : raw << shift_amount;
		return Fix32::from_raw(sign ? -raw : raw);
	}
	return { 0 };
}

Fix32::Fix32(int value)
	: _value((int64_t)value << 32)
{
}

Fix32::Fix32(uint32_t value)
	: _value((int64_t)value << 32)
{
}

Fix32::Fix32(int64_t value)
	: _value(value << 32)
{
}

Fix32::Fix32(uint64_t value)
	: _value(value << 32)
{
}

Fix32::Fix32(float value)
	: Fix32(Fix32::from_real(value))
{
}

Fix32::Fix32(double value)
	: Fix32(Fix32::from_real(value))
{
}


template<>
float Fix32::to_real<float>() const
{
	auto value = _value;
	if (value == 0)
	{
		return 0;
	}
	bool sign = value < 0;
	value = sign ? -value : value;
	auto clz = clzll(value) + 1;
	auto exp = 32 - clz;
	exp += 127;

	auto shamt = 64 - clz - 23;
	if (shamt > 0)
	{
		value >>= shamt;
	}
	else
	{
		value <<= -shamt;
	}

	auto ivalue = (((uint32_t)sign) << 31) | ((uint32_t)value & 0x7F'FFFF) | (exp << 23);
	return reinterpret_cast<float&>(ivalue);
}
template<>
double Fix32::to_real<double>() const
{
	auto value = _value;
	if (value == 0)
	{
		return 0;
	}
	bool sign = value < 0;
	value = sign ? -value : value;
	auto clz = clzll(value) + 1;
	auto exp = 32 - clz;
	exp += 1023;

	auto shamt = 64 - clz - 52;
	if (shamt > 0)
	{
		value >>= shamt;
	}
	else
	{
		value <<= -shamt;
	}

	auto ivalue = (((uint64_t)sign) << 63) | (value & 0xF'FFFF'FFFF'FFFFull) | ((uint64_t)exp << 52);
	return reinterpret_cast<double&>(ivalue);
}

template<>
long double Fix32::to_real<long double>() const
{
	return (long double)_value / std::numeric_limits<uint32_t>::max();
}

int Fix32::compare(Fix32 b) const
{
	if (this->_value > b._value) { return 1; }
	if (this->_value < b._value) { return -1; }
	return 0;
}

const Fix32 Fix32::MAX{ Fix32::from_raw(std::numeric_limits<int64_t>::max()) };
const Fix32 Fix32::MIN{ Fix32::from_raw(std::numeric_limits<int64_t>::min()) };

template float Fix32::to_real<float>() const;
template double Fix32::to_real<double>() const;
template long double Fix32::to_real<long double>() const;