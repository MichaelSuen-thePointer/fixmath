#include "Fix32.hpp"
#include <tuple>
#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <ostream>

#ifdef _MSC_VER
#  include <intrin.h>
#  ifdef _WIN64
int clzll(uint64_t value) {
	unsigned long leading_zero = 0;
	if (_BitScanReverse64(&leading_zero, value)) {
		return 63 - leading_zero;
	} else {
		// Same remarks as above
		return 64;
	}
}
#  elif _WIN32
int clzll(uint64_t value) {
	unsigned long leading_zero = 0;
	if (_BitScanReverse(&leading_zero, (unsigned long)(value >> 32))) {
		return 31 - leading_zero;
	} else if (_BitScanReverse(&leading_zero, (unsigned long)value)) {
		return 63 - leading_zero;
	} else {
		// Same remarks as above
		return 64;
	}
}
#  else
#    error "platform not supported"
#  endif
#  define likely(expr)    (expr)
#  define unlikely(expr)  (expr)
#else
int clzll(uint64_t value) {
	return __builtin_clzll(value);
}
#  define likely(expr)    (__builtin_expect(!!(expr), 1))
#  define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#endif

using std::pair;
using std::tuple;
using i64limits = std::numeric_limits<int64_t>;
using i32limits = std::numeric_limits<int32_t>;

pair<uint64_t, uint64_t> negate(uint64_t low, int64_t high) {
	uint64_t l = -(int64_t)low;
	uint64_t h = low != 0;
	h = -high - h;
	return { l, h };
}

pair<uint64_t, uint64_t> abs(uint64_t low, int64_t high) {
	if (high < 0) {
		return negate(low, high);
	}
	return { low, (uint64_t)high };
}

tuple<uint64_t, int64_t, int64_t> int128_div_rem(uint64_t low, int64_t high, int64_t divisor) {
	auto[abs_low, abs_high] = abs(low, high);
	auto abs_divisor = std::abs(divisor);
	uint64_t quotient_high = 0, quotient_low = 0, remainder = 0;
	if (high == 0) {
		quotient_low = abs_low / abs_divisor;
		remainder = abs_low % abs_divisor;
	} else {
		quotient_high = abs_high / abs_divisor;
		remainder = abs_high % abs_divisor;
		if (remainder == 0) {
			quotient_low = abs_low / abs_divisor;
			remainder = abs_low % abs_divisor;
		} else {
			auto msb = clzll(remainder);
			quotient_low = 0;
			auto copy_low = abs_low;

			int bits_remain = 64;
			while (bits_remain) {
				auto new_dividend = (remainder << msb) | (copy_low >> (64 - msb));
				auto new_quo = new_dividend / abs_divisor;
				remainder = new_dividend % abs_divisor;
				bits_remain -= msb;
				quotient_low |= new_quo << bits_remain;
				copy_low <<= msb;
				if (remainder == 0 && bits_remain) {
					new_dividend = copy_low >> (64 - bits_remain);
					new_quo = new_dividend / abs_divisor;
					remainder = new_dividend % abs_divisor;
					quotient_low |= new_quo;
					break;
				}
				msb = std::min(bits_remain, clzll(remainder));
			}
		}
	}
	if ((high < 0 && divisor < 0) || (high > 0 && divisor > 0)) {
		return { quotient_low, quotient_high, (int64_t)remainder };
	}
	if (high < 0) {
		return tuple_cat(negate(quotient_low, quotient_high), std::make_tuple(-(int64_t)remainder));
	}
	if (divisor < 0) {
		return tuple_cat(negate(quotient_low, quotient_high), std::make_tuple((int64_t)remainder));
	}
	return { quotient_low, quotient_high, (int64_t)remainder };
}

pair<uint64_t, int64_t> int128_mul(int64_t _a, int64_t _b) {
	int64_t a = std::abs(_a);
	int64_t b = std::abs(_b);
	using int_limits = std::numeric_limits<int32_t>;
	uint64_t rhi, rlo;
	if (a <= int_limits::max() && b <= int_limits::max()) {
		rhi = 0;
		rlo = a * b;
	} else if ((a & 0xFFFFFFFFu) == 0 && (b & 0xFFFFFFFFu) == 0) {
		rhi = (a >> 32) * (b >> 32);
		rlo = 0;
	} else {
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
	if ((_a < 0 && _b < 0) || (_a > 0 && _b > 0)) {
		return { rlo, rhi };
	}
	return negate(rlo, rhi);
}

int64_t shl32_div(int64_t a, int64_t b) {
	auto[lo, hi, rem] = int128_div_rem(a << 32, a >> 32, b);
	(void)hi;
	(void)rem;
	return lo;
}

int64_t mul_shr32(int64_t a, int64_t b) {
	auto[lo, hi] = int128_mul(a, b);
	return (hi << 32) | (lo >> 32);
}

std::pair<int64_t, int64_t> safe_shl32_div(int64_t a, int64_t b) {
	auto[lo, hi, rem] = int128_div_rem(a << 32, a >> 32, b);
	(void)rem;
	int64_t overflow = 0;
	if unlikely((int64_t)lo < i64limits::min() + 2 || (int64_t)lo > i64limits::max() - 1) { //低位溢出
		overflow = hi >= 0 ? 1 : -1;
	} else { //高位溢出
		overflow = hi > 0 ? 1 : hi < -1 ? -1 : 0;
	}
	return { lo, overflow };
}

std::pair<int64_t, int64_t> safe_mul_shr32(int64_t a, int64_t b) {
	auto[lo, hi] = int128_mul(a, b);
	lo = (hi << 32) | (lo >> 32);
	hi >>= 32;
	int64_t overflow = 0;
	if unlikely((int64_t)lo < i64limits::min() + 2 || (int64_t)lo > i64limits::max() - 1) { //低位溢出
		overflow = hi >= 0 ? 1 : -1;
	} else { //高位溢出
		overflow = hi > 0 ? 1 : hi < -1 ? -1 : 0;
	}
	return { lo, overflow };
}

Fix32 operator+(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely(a.infinity_sign() * b.infinity_sign() < 0) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely(a.infinity_sign() + b.infinity_sign() > 0) {
		return Fix32::POSITIVE_INFINITY;
	}
	if unlikely(a.infinity_sign() + b.infinity_sign() < 0) {
		return Fix32::NEGATIVE_INFINITY;
	}
	int64_t raw_result = (int64_t)((uint64_t)a._value + (uint64_t)b._value);
	if unlikely((a._value ^ b._value) > 0 && (a._value ^ raw_result) < 0) {
		//操作数同号，结果与操作数异号，溢出
		return raw_result > 0 ? Fix32::NEGATIVE_INFINITY : Fix32::POSITIVE_INFINITY;
	}
	if unlikely(raw_result == i64limits::min()) {
		//结果刚好等于i64min，溢出
		return Fix32::NEGATIVE_INFINITY;
	}
	return Fix32::from_raw(raw_result);
}

Fix32 operator-(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely((a.is_positive_infinity() && b.is_positive_infinity())
		|| (a.is_negative_infinity() && b.is_negative_infinity())) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely(a.is_infinity()) {
		return a;
	}
	if unlikely(b.is_infinity()) {
		return b;
	}
	if unlikely(a._value > 0 && b._value < 0 && i64limits::max() - 1 + b._value < a._value) {
		return Fix32::POSITIVE_INFINITY;
	}
	if unlikely(a._value < 0 && b._value > 0 && i64limits::min() + 2 + b._value > a._value) {
		return Fix32::NEGATIVE_INFINITY;
	}
	return Fix32::from_raw(a._value - b._value);
}

Fix32 operator*(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely((a._value == 0 && b.is_infinity()) || (a.is_infinity() && b._value == 0)) {
		return Fix32::NOT_A_NUMBER;
	}
	if (uint64_t abs_value = std::abs(b._value); (abs_value & (abs_value - 1)) == 0) {
		auto shamt = 31 - clzll(abs_value);
		auto r_value = a._value;
		if (shamt > 0) {
			if unlikely(clzll(std::abs(r_value)) <= shamt) {
				return r_value > 0 ? Fix32::POSITIVE_INFINITY : Fix32::NEGATIVE_INFINITY;
			}
			r_value <<= shamt;
		} else {
			r_value >>= -shamt;
		}
		return Fix32::from_raw((b._value < 0 ? -1 : 1) * r_value);
	}
	auto[r, overflow] = safe_mul_shr32(a._value, b._value);
	if likely(!overflow) {
		return Fix32::from_raw(r);
	}
	return overflow > 0 ? Fix32::POSITIVE_INFINITY : Fix32::NEGATIVE_INFINITY;
}

Fix32 operator/(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NOT_A_NUMBER;
	}
	if unlikely((a._value == 0 && b._value == 0) || (a.is_infinity() && b.is_infinity())) {
		return Fix32::NOT_A_NUMBER;
	}
	if (a._value == 0) {
		return Fix32::ZERO;
	}
	if (b._value == 0) {
		return a._value > 0 ? Fix32::POSITIVE_INFINITY : Fix32::NEGATIVE_INFINITY;
	}
	if (uint64_t abs_value = std::abs(b._value); (abs_value & (abs_value - 1)) == 0) {
		auto shamt = 31 - clzll(abs_value);
		auto r_value = a._value;
		if (shamt > 0) {
			r_value >>= shamt;
		} else {
			if unlikely(clzll(std::abs(r_value)) <= -shamt) {
				return r_value > 0 ? Fix32::POSITIVE_INFINITY : Fix32::NEGATIVE_INFINITY;
			}
			r_value <<= -shamt;
		}
		return Fix32::from_raw((b._value < 0 ? -1 : 1) * r_value);
	}
	auto[r, overflow] = safe_shl32_div(a._value, b._value);
	if likely(!overflow) {
		return Fix32::from_raw(r);
	}
	return overflow > 0 ? Fix32::POSITIVE_INFINITY : Fix32::NEGATIVE_INFINITY;
}

Fix32& operator+=(Fix32& a, Fix32 b) {
	return a = a + b;
}

Fix32& operator-=(Fix32& a, Fix32 b) {
	return a = a - b;
}

Fix32& operator*=(Fix32& a, Fix32 b) {
	return a = a * b;
}

Fix32& operator/=(Fix32& a, Fix32 b) {
	return a = a / b;
}

bool operator>(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return false;
	}
	return a.compare(b) > 0;
}

bool operator>=(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return false;
	}
	return a.compare(b) >= 0;
}

bool operator<(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return false;
	}
	return a.compare(b) < 0;
}

bool operator<=(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return false;
	}
	return a.compare(b) <= 0;
}

bool operator==(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return false;
	}
	return a.compare(b) == 0;
}

bool operator!=(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() && b.is_nan()) {
		return true;
	}
	return a.compare(b) != 0;
}

std::ostream& operator<<(std::ostream& os, Fix32 a) {
	if (a.is_positive_infinity()) {
		return os << "inf";
	}
	if (a.is_negative_infinity()) {
		return os << "-inf";
	}
	if (a.is_nan()) {
		return os << "nan";
	}
	return os << a.to_real<double>();
}

Fix32 Fix32::from_integer(int value) {
	if unlikely(value == i32limits::min()) {
		return NEGATIVE_INFINITY;
	}
	return Fix32::from_raw((int64_t)value << 32);
}

Fix32 Fix32::from_integer(uint32_t value) {
	if unlikely(value >> 31) {
		return POSITIVE_INFINITY;
	}
	return Fix32::from_raw((int64_t)value << 32);
}

Fix32 Fix32::from_integer(int64_t value) {
	if unlikely(value > i32limits::max()) {
		return POSITIVE_INFINITY;
	}
	if unlikely(value < -i32limits::max()) {
		return NEGATIVE_INFINITY;
	}
	return Fix32::from_raw(value << 32);
}

Fix32 Fix32::from_integer(uint64_t value) {
	if unlikely(value > i32limits::max()) {
		return POSITIVE_INFINITY;
	}
	return Fix32(value);
}

Fix32 Fix32::from_real(float value) {
	using flimits = std::numeric_limits<float>;
	if unlikely(value != value) {
		return NOT_A_NUMBER;
	}
	if unlikely(value == flimits::infinity()) {
		return POSITIVE_INFINITY;
	}
	if unlikely(value == -flimits::infinity()) {
		return NEGATIVE_INFINITY;
	}
	if (value == 0) {
		return { 0 };
	}
	if (value == 1) {
		return { 1 };
	}
	if (value == -1) {
		return { -1 };
	}
	uint32_t ivalue = reinterpret_cast<uint32_t&>(value);
	bool sign = !!(ivalue >> 31);
	int32_t exp = (ivalue >> 23) & 0xFF;
	uint32_t frac = ivalue & 0x7F'FFFF;
	if (exp != 0) {
		exp -= 127;
		frac |= 0x80'0000;
		int64_t raw = frac;
		auto shift_amount = 32 - 23 + exp;
		if (shift_amount > 63 || shift_amount < -63) {
			return { 0 };
		}
		raw = shift_amount < 0 ? raw >> -shift_amount : raw << shift_amount;
		return Fix32::from_raw(sign ? -raw : raw);
	}
	return { 0 };
}

Fix32 Fix32::from_real(double value) {
	using flimits = std::numeric_limits<double>;
	if unlikely(value != value) {
		return NOT_A_NUMBER;
	}
	if unlikely(value == flimits::infinity()) {
		return POSITIVE_INFINITY;
	}
	if unlikely(value == -flimits::infinity()) {
		return NEGATIVE_INFINITY;
	}
	if (value == 0) {
		return { 0 };
	}
	if (value == 1) {
		return { 1 };
	}
	if (value == -1) {
		return { -1 };
	}

	uint64_t ivalue = reinterpret_cast<uint64_t&>(value);
	bool sign = !!(ivalue >> 63);
	int32_t exp = (ivalue >> 53) & 0x7FF;
	uint64_t frac = ivalue & 0xF'FFFF'FFFF'FFFFull;
	if (exp != 0) {
		exp -= 1023;
		frac |= 0x10'0000'0000'0000;
		int64_t raw = frac;
		auto shift_amount = 32 - 52 + exp;
		if (shift_amount > 63 || shift_amount < -63) {
			return { 0 };
		}
		raw = shift_amount < 0 ? raw >> -shift_amount : raw << shift_amount;
		return Fix32::from_raw(sign ? -raw : raw);
	}
	return { 0 };
}

Fix32::Fix32(int value)
	: Fix32(Fix32::from_integer(value)) {
}

Fix32::Fix32(uint32_t value)
	: Fix32(Fix32::from_integer(value)) {
}

Fix32::Fix32(int64_t value)
	: Fix32(Fix32::from_integer(value)) {
}

Fix32::Fix32(uint64_t value)
	: Fix32(Fix32::from_integer(value)) {
}

Fix32::Fix32(float value)
	: Fix32(Fix32::from_real(value)) {
}

Fix32::Fix32(double value)
	: Fix32(Fix32::from_real(value)) {
}

bool Fix32::is_positive_infinity() const {
	return _value == POSITIVE_INFINITY._value;
}

bool Fix32::is_negative_infinity() const {
	return _value == NEGATIVE_INFINITY._value;
}

bool Fix32::is_infinity() const {
	return is_positive_infinity() || is_negative_infinity();
}

int Fix32::infinity_sign() const {
	return _value == NEGATIVE_INFINITY._value ? -1 : _value == POSITIVE_INFINITY._value ? 1 : 0;
}

bool Fix32::is_nan() const {
	return _value == NOT_A_NUMBER._value;
}

template<>
float Fix32::to_real<float>() const {
	using flimits = std::numeric_limits<float>;
	if (is_negative_infinity()) {
		return -flimits::infinity();
	}
	if (is_positive_infinity()) {
		return flimits::infinity();
	}
	if (is_nan()) {
		return flimits::quiet_NaN();
	}
	auto value = _value;
	if (value == 0) {
		return 0;
	}
	bool sign = value < 0;
	value = sign ? -value : value;
	auto clz = clzll(value) + 1;
	auto exp = 32 - clz;
	exp += 127;

	auto shamt = 64 - clz - 23;
	if (shamt > 0) {
		value >>= shamt;
	} else {
		value <<= -shamt;
	}

	auto ivalue = (((uint32_t)sign) << 31) | ((uint32_t)value & 0x7F'FFFF) | (exp << 23);
	return reinterpret_cast<float&>(ivalue);
}
template<>
double Fix32::to_real<double>() const {
	using flimits = std::numeric_limits<double>;
	if (is_negative_infinity()) {
		return -flimits::infinity();
	}
	if (is_positive_infinity()) {
		return flimits::infinity();
	}
	if (is_nan()) {
		return flimits::quiet_NaN();
	}
	auto value = _value;
	if (value == 0) {
		return 0;
	}
	bool sign = value < 0;
	value = sign ? -value : value;
	auto clz = clzll(value) + 1;
	auto exp = 32 - clz;
	exp += 1023;

	auto shamt = 64 - clz - 52;
	if (shamt > 0) {
		value >>= shamt;
	} else {
		value <<= -shamt;
	}

	auto ivalue = (((uint64_t)sign) << 63) | (value & 0xF'FFFF'FFFF'FFFFull) | ((uint64_t)exp << 52);
	return reinterpret_cast<double&>(ivalue);
}

template<>
long double Fix32::to_real<long double>() const {
	if constexpr (sizeof(long double) > sizeof(double)) {
		return (long double)_value / std::numeric_limits<uint32_t>::max();
	} else {
		return to_real<double>();
	}
}

int Fix32::compare(Fix32 b) const {
	if (this->_value > b._value) { return 1; }
	if (this->_value < b._value) { return -1; }
	return 0;
}

const Fix32 Fix32::ZERO{ Fix32::from_raw(0) };
const Fix32 Fix32::ONE{ Fix32::from_raw(1ll << 32) };
const Fix32 Fix32::MAX{ Fix32::from_raw(i64limits::max() - 1) };
const Fix32 Fix32::MIN{ Fix32::from_raw(i64limits::min() + 2) };
const Fix32 Fix32::MAX_INTEGER{ Fix32::from_raw((int64_t)i32limits::max() << 32) };
const Fix32 Fix32::MIN_INTEGER{ Fix32::from_raw((int64_t)-i32limits::max() << 32) };
const Fix32 Fix32::POSITIVE_INFINITY{ Fix32::from_raw(i64limits::max()) };
const Fix32 Fix32::NEGATIVE_INFINITY{ Fix32::from_raw(i64limits::min() + 1) };
const Fix32 Fix32::NOT_A_NUMBER{ Fix32::from_raw(i64limits::min()) };

template float Fix32::to_real<float>() const;
template double Fix32::to_real<double>() const;
template long double Fix32::to_real<long double>() const;