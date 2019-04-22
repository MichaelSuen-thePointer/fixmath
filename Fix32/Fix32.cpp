#include "Fix32.hpp"
#include <tuple>
#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <ostream>
#include <cassert>
#include <array>
#include <string>

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
#  define assume(cond)
#else
int clzll(uint64_t value) {
	return __builtin_clzll(value);
}
#  define likely(expr)    (__builtin_expect(!!(expr), 1))
#  define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#  define assume(cond)    do { if (!(cond)) __builtin_unreachable(); } while (0)
#endif

#if !defined __has_include || !__has_include(<compare>)
constexpr partial_ordering partial_ordering::less = -1;
constexpr partial_ordering partial_ordering::equivalent = 0;
constexpr partial_ordering partial_ordering::greater = 1;
constexpr partial_ordering partial_ordering::unordered = -127;
#endif

using std::pair;
using std::tuple;
using i64limits = std::numeric_limits<int64_t>;
using i32limits = std::numeric_limits<int32_t>;
using u32limits = std::numeric_limits<uint32_t>;
using u64limits = std::numeric_limits<uint64_t>;

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
	assume(divisor != 0);
	auto abs_divisor = std::abs(divisor);
	auto[abs_low, abs_high] = abs(low, high);
	uint64_t quotient_high = 0, quotient_low = 0, remainder = 0;
	if (abs_high == 0) { // 64bit divrem 64bit
		quotient_low = abs_low / abs_divisor;
		remainder = abs_low % abs_divisor;
	} else if ((abs_divisor & (abs_divisor - 1)) == 0) { // divrem by power of 2
		remainder = abs_low & (abs_divisor - 1);
		if (abs_divisor == 1) {
			quotient_high = abs_high;
			quotient_low = abs_low;
		} else {
			auto sr = 63 - clzll(abs_divisor);
			quotient_high = abs_high >> sr;
			quotient_low = (abs_high << (64 - sr)) | (abs_low >> sr);
		}
	} else {
		uint64_t remainder_high = 0;
		uint64_t remainder_low = 0;
		auto sr = 1 + 64 + clzll(abs_divisor) - clzll(abs_high);
		if (sr == 64) {
			quotient_low = 0;
			quotient_high = abs_low;
			remainder_high = 0;
			remainder_low = abs_high;
		} else if (sr < 64) {
			quotient_low = 0;
			quotient_high = abs_low << (64 - sr);
			remainder_high = abs_high >> sr;
			remainder_low = (abs_high << (64 - sr)) | (abs_low >> sr);
		} else {
			quotient_low = abs_low << (128 - sr);
			quotient_high = (abs_high << (128 - sr)) | (abs_low >> (sr - 64));
			remainder_high = 0;
			remainder_low = abs_high >> (sr - 64);
		}
		uint32_t carry = 0;
		for (; sr > 0; --sr) {
			remainder_high = (remainder_high << 1) | (remainder_low >> 63);
			remainder_low = (remainder_low << 1) | (quotient_high >> 63);
			quotient_high = (quotient_high << 1) | (quotient_low >> 63);
			quotient_low = (quotient_low << 1) | carry;

			int64_t s = remainder_high > 0 ? -1 : (int64_t)(abs_divisor - remainder_low - 1) >> 63;
			carry = s & 1;
			auto old_remainder_low = remainder_low;
			remainder_low -= abs_divisor & s;
			remainder_high -= old_remainder_low < remainder_low;
		}
		quotient_high = (quotient_high << 1) | (quotient_low >> 63);
		quotient_low = (quotient_low << 1) | carry;
		remainder = remainder_low;
		assert(remainder_high == 0);
	}

	if ((high ^ divisor) >= 0) {
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

tuple<uint64_t, uint64_t, uint64_t> shifted_uint64_div32(uint64_t a, uint32_t b) {
	uint32_t quo3 = (uint32_t)((a >> 32) / b);
	uint32_t rem3 = (uint32_t)((a >> 32) % b);
	uint64_t t = (uint64_t)rem3 << 32 | (uint32_t)a;
	uint32_t quo2 = (uint32_t)(t / b);
	uint32_t rem2 = (uint32_t)(t % b);
	t = (uint64_t)rem2 << 32;
	uint32_t quo1 = (uint32_t)(t / b);
	uint32_t rem1 = (uint32_t)(t % b);
	return { (uint64_t)quo2 << 32 | quo1, quo3, rem1 };
}

tuple<uint64_t, uint64_t, uint64_t> shifted_uint64_div(uint64_t a, uint64_t b) {
	//implements (a << 32) / b
	//use multiprecision division, consider a and b as 2-digit 2^32 based number
	//see Knuth Vol.2 for algorithm explaination
	assert((a >> 63) == 0 && (b >> 63) == 0);
	if ((b & (b - 1)) == 0) {
		int32_t shamt = 31 - clzll(b);
		if (shamt >= 0) {
			return { a >> shamt, 0, a & ((1 << shamt) - 1) };
		} else {
			return { a << -shamt, a >> (64 + shamt), 0 };
		}
	}
	if ((a >> 32) == 0) {
		return { (a << 32) / b, 0, (a << 32) % b };
	}
	if ((b >> 32) == 0) {
		return shifted_uint64_div32(a, (uint32_t)b);
	}
	const unsigned n = 2;
	auto log_d = clzll(b);
	std::array<uint32_t, 4> u;
	std::array<uint32_t, 2> v = { (uint32_t)(b << log_d), (uint32_t)(b << log_d >> 32) };
	if (log_d != 0) {
		u = { 0, (uint32_t)(a << log_d), (uint32_t)(a << log_d >> 32), (uint32_t)(a >> (64 - log_d)) };
	} else {
		u = { 0, (uint32_t)a, (uint32_t)(a >> 32), 0 };
	}
	std::array<uint32_t, 2> quotient = {};
	{
		const unsigned j = 1;
		uint64_t a = (uint64_t)u[j + n] << 32 | u[j + n - 1];
		uint64_t q = a / v[n - 1];
		uint64_t r = a % v[n - 1];
		if unlikely(q == u32limits::max() || q * v[n - 2] > (((uint64_t)r << 32) | u[j + n - 2])) {
			q -= 1;
			r += v[n - 1];
		}
		std::array<uint32_t, 3> qv;
		{
			uint64_t t = (uint64_t)v[0] * q;
			qv[0] = (uint32_t)t;
			uint32_t temp_carry = (uint32_t)(t >> 32);
			t = (uint64_t)v[1] * q + temp_carry;
			qv[1] = (uint32_t)t;
			temp_carry = (uint32_t)(t >> 32);
			qv[2] = temp_carry;
		}
		bool neg = std::make_tuple(u[3], u[2], u[1]) < std::make_tuple(qv[2], qv[1], qv[0]);

		qv[0] = ~qv[0];
		qv[1] = ~qv[1];
		qv[2] = ~qv[2];
		qv[2] += qv[1] == u32limits::max() && qv[0] == u32limits::max();
		qv[1] += qv[0] == u32limits::max();
		qv[0] += 1;
		{
			uint64_t t = (uint64_t)u[1] + qv[0];
			u[1] = (uint32_t)t;
			uint32_t carry = (uint32_t)(t >> 32);
			t = (uint64_t)u[2] + qv[1] + carry;
			u[2] = (uint32_t)t;
			carry = t >> 32;
			u[3] += qv[2] + carry;
		}

		if (neg) {
			quotient[j] = (uint32_t)(q - 1);
			{
				uint64_t t = (uint64_t)u[1] + v[0];
				u[1] = (uint32_t)t;
				uint32_t carry = (uint32_t)(t >> 32);
				t = (uint64_t)u[2] + v[1] + carry;
				u[2] = (uint32_t)t;
				carry = t >> 32;
				u[3] += carry;
			}
		} else {
			quotient[j] = (uint32_t)q;
		}
	}
	{
		const unsigned j = 0;
		uint64_t a = (uint64_t)u[j + n] << 32 | u[j + n - 1];
		uint64_t q = a / v[n - 1];
		uint64_t r = a % v[n - 1];
		if unlikely(q == u32limits::max() || q * v[n - 2] > ((uint64_t)r << 32 | u[j + n - 2])) {
			q -= 1;
			r += v[n - 1];
		}
		std::array<uint32_t, 3> qv;
		{
			uint64_t t = (uint64_t)v[0] * q;
			qv[0] = (uint32_t)t;
			uint32_t temp_carry = (uint32_t)(t >> 32);
			t = (uint64_t)v[1] * q + temp_carry;
			qv[1] = (uint32_t)t;
			temp_carry = (uint32_t)(t >> 32);
			qv[2] = temp_carry;
		}
		bool neg = std::make_tuple(u[2], u[1], u[0]) < std::make_tuple(qv[2], qv[1], qv[0]);

		qv[0] = ~qv[0];
		qv[1] = ~qv[1];
		qv[2] = ~qv[2];
		qv[2] += qv[1] == u32limits::max() && qv[0] == u32limits::max();
		qv[1] += qv[0] == u32limits::max();
		qv[0] += 1;
		{
			uint64_t t = (uint64_t)u[0] + qv[0];
			u[0] = (uint32_t)t;
			uint32_t carry = (uint32_t)(t >> 32);
			t = (uint64_t)u[1] + qv[1] + carry;
			u[1] = (uint32_t)t;
			carry = t >> 32;
			u[2] += qv[2] + carry;
		}

		if (neg) {
			quotient[j] = (uint32_t)(q - 1);
			{
				uint64_t t = (uint64_t)u[0] + v[0];
				u[0] = (uint32_t)t;
				uint32_t carry = (uint32_t)(t >> 32);
				t = (uint64_t)u[1] + v[1] + carry;
				u[1] = (uint32_t)t;
				carry = t >> 32;
				u[2] += carry;
			}
		} else {
			quotient[j] = (uint32_t)q;
		}
	}
	if (log_d) {
		std::array<uint32_t, 2> remainder = { (u[0] >> log_d) | (u[1] << (32 - log_d)), (u[1] >> log_d) | (u[2] << (32 - log_d)) };
		return { quotient[0] | ((uint64_t)quotient[1] << 32), 0, remainder[0] | ((uint64_t)remainder[1] << 32) };
	}
	return { quotient[0] | (uint64_t)quotient[1] << 32, 0, u[0] | ((uint64_t)u[1] << 32) };
}

std::tuple<uint64_t, int64_t, int64_t> shifted_int64_div(int64_t _a, int64_t _b) {
	assert(_a != i64limits::min() && _b != i64limits::min());
	int64_t a = std::abs(_a);
	int64_t b = std::abs(_b);
	uint64_t rlo;
	int64_t rhi, rem;
	std::tie(rlo, rhi, rem) = shifted_uint64_div(a, b);
	if ((_a ^ _b) < 0) {
		std::tie(rlo, rhi) = negate(rlo, rhi);
		if (_a < 0) {
			rem = -rem;
		}
	}
	return { rlo, rhi, rem };
}

pair<uint64_t, int64_t> int128_mul(int64_t _a, int64_t _b) {
	assert(_a != i64limits::min() && _b != i64limits::min());
	int64_t a = std::abs(_a);
	int64_t b = std::abs(_b);
	uint64_t rhi, rlo;
	if ((a & (a - 1)) == 0) {
		std::swap(a, b);
		std::swap(_a, _b);
	}
	if ((b & (b - 1)) == 0) {
		if (b <= 1) {
			rlo = a * b;
			rhi = 0;
		} else {
			uint32_t shamt = 63 - clzll(b);
			rhi = a >> (64 - shamt);
			rlo = a << shamt;
		}
	} else if (a <= i32limits::max() && b <= i32limits::max()) {
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
	if ((_a ^ _b) >= 0) {
		return { rlo, rhi };
	}
	return negate(rlo, rhi);
}

int64_t shl32_div(int64_t a, int64_t b) {
	auto[lo, hi, rem] = shifted_int64_div(a, b);
	(void)hi;
	(void)rem;
	return lo;
}

int64_t mul_shr32(int64_t a, int64_t b) {
	auto[lo, hi] = int128_mul(a, b);
	auto mask = (uint64_t)(hi >> 63) >> 32;
	hi += (lo & mask) != 0;
	lo += mask;
	return (hi << 32) | (lo >> 32);
}

std::pair<int64_t, int> safe_shl32_div(int64_t a, int64_t b) {
	assert(b != 0);
	uint64_t lo;
	int64_t hi;
	int64_t rem;
	std::tie(lo, hi, rem) = shifted_int64_div(a, b);
	int overflow = 0;
	if unlikely(hi == 0 && (int64_t)lo < 0 || hi == -1 && ((int64_t)lo >= 0 || (int64_t)lo == i64limits::min())) { //低位溢出
		overflow = hi >= 0 ? 1 : -1;
	} else if unlikely(hi > 0) {
		overflow = 1;
	} else if unlikely(hi < -1) {
		overflow = -1;
	}
	return { lo, overflow };
}

std::pair<int64_t, int> safe_mul_shr32(int64_t a, int64_t b) {
	auto[lo, hi] = int128_mul(a, b);
	auto mask = (uint64_t)(hi >> 63) >> 32;
	hi += (lo & mask) != 0;
	lo += mask;
	lo = (hi << 32) | (lo >> 32);
	hi >>= 32;
	int overflow = 0;
	if unlikely(hi == 0 && (int64_t)lo < 0 || hi == -1 && ((int64_t)lo >= 0 || (int64_t)lo == i64limits::min())) { //低位溢出
		overflow = hi >= 0 ? 1 : -1;
	} else if unlikely(hi > 0) {
		overflow = 1;
	} else if unlikely(hi < -1) {
		overflow = -1;
	}
	return { lo, overflow };
}

Fix32 operator+(Fix32 a) {
	return a;
}

Fix32 operator-(Fix32 a) {
	return Fix32::from_raw((int64_t)(~(uint64_t)a._value + 1ull));
}

Fix32 operator+(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NaN;
	}
	if unlikely(a.infinity_sign() * b.infinity_sign() < 0) {
		return Fix32::NaN;
	}
	if unlikely(a.is_infinity()) {
		return a;
	}
	if unlikely(b.is_infinity()) {
		return b;
	}
	int64_t raw_result = (int64_t)((uint64_t)a._value + (uint64_t)b._value);
	if unlikely((a._value ^ b._value) >= 0 && (a._value ^ raw_result) < 0) {
		//操作数同号，结果与操作数异号，溢出
		return raw_result > 0 ? -Fix32::INF : Fix32::INF;
	}
	if unlikely(raw_result == i64limits::min()) {
		//结果刚好等于i64min，溢出
		return -Fix32::INF;
	}
	//结果等于INFINITY_RAW, 恰好溢出，无需判断
	return Fix32::from_raw(raw_result);
}

Fix32 operator-(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NaN;
	}
	if unlikely(a.infinity_sign() * b.infinity_sign() > 0) {
		return Fix32::NaN;
	}
	if unlikely(a.is_infinity()) {
		return a;
	}
	if unlikely(b.is_infinity()) {
		return -b;
	}
	int64_t raw_result = (int64_t)((uint64_t)a._value - (uint64_t)b._value);
	if unlikely((a._value ^ b._value) < 0 && (a._value ^ raw_result) < 0) {
		//操作数同号，结果与操作数异号，溢出
		return raw_result > 0 ? -Fix32::INF : Fix32::INF;
	}
	if unlikely(raw_result == i64limits::min()) {
		return -Fix32::INF;
	}
	//结果等于INFINITY_RAW, 恰好溢出，无需判断
	return Fix32::from_raw(raw_result);
}

Fix32 operator*(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NaN;
	}
	if unlikely((a._value == 0 && b.is_infinity()) || (a.is_infinity() && b._value == 0)) {
		return Fix32::NaN;
	}
	if unlikely(a.is_infinity() || b.is_infinity()) {
		return (a._value ^ b._value) >= 0 ? Fix32::INF : -Fix32::INF;
	}
	auto[r, overflow] = safe_mul_shr32(a._value, b._value);
	if likely(!overflow) {
		return Fix32::from_raw(r);
	}
	return overflow > 0 ? Fix32::INF : -Fix32::INF;
}

Fix32 operator%(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NaN;
	}
	if unlikely(a.is_infinity() || b._value == 0) {
		return Fix32::NaN;
	}
	if unlikely(b.is_infinity()) {
		return a;
	}
	return Fix32::from_raw(a._value % b._value);
}

Fix32 operator/(Fix32 a, Fix32 b) {
	if unlikely(a.is_nan() || b.is_nan()) {
		return Fix32::NaN;
	}
	if unlikely((a._value == 0 && b._value == 0) || (a.is_infinity() && b.is_infinity())) {
		return Fix32::NaN;
	}
	if unlikely(a.is_infinity()) {
		return b >= 0 ? a : -a;
	}
	if unlikely(b.is_infinity()) {
		return Fix32::ZERO;
	}
	if unlikely(b._value == 0) {
		return a._value > 0 ? Fix32::INF : -Fix32::INF;
	}
	auto[r, overflow] = safe_shl32_div(a._value, b._value);
	if likely(!overflow) {
		return Fix32::from_raw(r);
	}
	return overflow > 0 ? Fix32::INF : -Fix32::INF;
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

Fix32& operator%=(Fix32& a, Fix32 b) {
	return a = a % b;
}

bool operator>(Fix32 a, Fix32 b) {
	return a.compare(b) > 0;
}

bool operator>=(Fix32 a, Fix32 b) {
	return a.compare(b) >= 0;
}

bool operator<(Fix32 a, Fix32 b) {
	return a.compare(b) < 0;
}

bool operator<=(Fix32 a, Fix32 b) {
	return a.compare(b) <= 0;
}

bool operator==(Fix32 a, Fix32 b) {
	return a.compare(b) == 0;
}

bool operator!=(Fix32 a, Fix32 b) {
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

inline Fix32 Fix32::from_raw(int64_t value) {
	Fix32 r;
	r._value = value;
	return r;
}

Fix32 Fix32::from_integer(int value) {
	if unlikely(value == i32limits::min()) {
		return -INF;
	}
	return Fix32::from_raw((int64_t)value << 32);
}

Fix32 Fix32::from_integer(uint32_t value) {
	if unlikely(value > (uint32_t)i32limits::max()) {
		return INF;
	}
	return Fix32::from_raw((int64_t)value << 32);
}

Fix32 Fix32::from_integer(int64_t value) {
	if unlikely(value > i32limits::max()) {
		return INF;
	}
	if unlikely(value < i32limits::min() + 1) {
		return -INF;
	}
	return Fix32::from_raw(value << 32);
}

Fix32 Fix32::from_integer(uint64_t value) {
	if unlikely(value > (uint64_t)i32limits::max()) {
		return INF;
	}
	return Fix32((int64_t)value << 32);
}

Fix32 Fix32::from_real(float value) {
	using flimits = std::numeric_limits<float>;
	if unlikely(value != value) {
		return NaN;
	}
	if unlikely(value == flimits::infinity()) {
		return INF;
	}
	if unlikely(value == -flimits::infinity()) {
		return -INF;
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
		if unlikely(shift_amount < -63) {
			return { 0 };
		}
		if unlikely(shift_amount > clzll(raw) - 1) {
			return sign ? -Fix32::INF : Fix32::INF;
		}
		if (shift_amount < 0) {
			shift_amount = -shift_amount;
			auto old_raw = raw;
			raw >>= shift_amount;
			if ((old_raw >> (shift_amount - 1)) & 1) {
				raw += 1;
			}
		} else {
			raw <<= shift_amount;
		}
		return Fix32::from_raw(sign ? -raw : raw);
	}
	return { 0 };
}

Fix32 Fix32::from_real(double value) {
	using flimits = std::numeric_limits<double>;
	if unlikely(value != value) {
		return NaN;
	}
	if unlikely(value == flimits::infinity()) {
		return INF;
	}
	if unlikely(value == -flimits::infinity()) {
		return -INF;
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
	int32_t exp = (ivalue >> 52) & 0x7FF;
	uint64_t frac = ivalue & 0xF'FFFF'FFFF'FFFFull;
	if (exp != 0) {
		exp -= 1023;
		frac |= 0x10'0000'0000'0000;
		int64_t raw = frac;
		auto shift_amount = 32 - 52 + exp;
		if unlikely(shift_amount < -63) {
			return { 0 };
		}
		if unlikely(shift_amount > clzll(raw) - 1) {
			return sign ? -Fix32::INF : Fix32::INF;
		}
		if (shift_amount < 0) {
			shift_amount = -shift_amount;
			auto old_raw = raw;
			raw >>= shift_amount;
			if ((old_raw >> (shift_amount - 1)) & 1) {
				raw += 1;
			}
		} else {
			raw <<= shift_amount;
		}
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
	return _value == INF_RAW;
}

bool Fix32::is_negative_infinity() const {
	return _value == -INF_RAW;
}

bool Fix32::is_infinity() const {
	return is_positive_infinity() || is_negative_infinity();
}

int Fix32::infinity_sign() const {
	return _value == -INF_RAW ? -1 : _value == INF_RAW ? 1 : 0;
}

bool Fix32::is_nan() const {
	return _value == NaN_RAW;
}

template<>
float Fix32::to_real<float>() const {
	using flimits = std::numeric_limits<float>;
	if unlikely(is_nan()) {
		return flimits::quiet_NaN();
	}
	if unlikely(is_negative_infinity()) {
		return -flimits::infinity();
	}
	if unlikely(is_positive_infinity()) {
		return flimits::infinity();
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
		if (((value >> (shamt - 1)) & 1)) {
			value = (value >> shamt) + 1;
			if unlikely(value >> 24) {
				value >>= 1;
				exp += 1;
			}
		} else {
			value >>= shamt;
		}
	} else {
		value <<= -shamt;
	}

	auto ivalue = (((uint32_t)sign) << 31) | ((uint32_t)value & 0x7F'FFFF) | (exp << 23);
	return reinterpret_cast<float&>(ivalue);
}
template<>
double Fix32::to_real<double>() const {
	using flimits = std::numeric_limits<double>;
	if unlikely(is_nan()) {
		return flimits::quiet_NaN();
	}
	if unlikely(is_negative_infinity()) {
		return -flimits::infinity();
	}
	if unlikely(is_positive_infinity()) {
		return flimits::infinity();
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
		if (((value >> (shamt - 1)) & 1)) {
			value = (value >> shamt) + 1;
			if unlikely(value >> 53) {
				value >>= 1;
				exp += 1;
			}
		} else {
			value >>= shamt;
		}
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

partial_ordering Fix32::compare(Fix32 b) const {
	if unlikely(this->_value == NaN_RAW || b._value == NaN_RAW) { return partial_ordering::unordered; }
	if (this->_value > b._value) { return partial_ordering::greater; }
	if (this->_value < b._value) { return partial_ordering::less; }
	return partial_ordering::equivalent;
}

const int64_t Fix32::ZERO_RAW = 0;
const int64_t Fix32::ONE_RAW = 1ll << 32;
const int64_t Fix32::MAX_RAW = i64limits::max() - 1;
const int64_t Fix32::MIN_RAW = i64limits::min() + 2;
const int64_t Fix32::MAX_INT_RAW = (int64_t)i32limits::max() << 32;
const int64_t Fix32::MIN_INT_RAW = (int64_t)((uint64_t)-i32limits::max() << 32);
const int64_t Fix32::INF_RAW = i64limits::max();
const int64_t Fix32::NaN_RAW = i64limits::min();
const int64_t Fix32::DELTA_RAW = 1;

const Fix32 Fix32::ZERO{ Fix32::from_raw(Fix32::ZERO_RAW) };
const Fix32 Fix32::ONE{ Fix32::from_raw(Fix32::ONE_RAW) };
const Fix32 Fix32::MAX{ Fix32::from_raw(Fix32::MAX_RAW) };
const Fix32 Fix32::MIN{ Fix32::from_raw(Fix32::MIN_RAW) };
const Fix32 Fix32::MAX_INT{ Fix32::from_raw(Fix32::MAX_INT_RAW) };
const Fix32 Fix32::MIN_INT{ Fix32::from_raw(Fix32::MIN_INT_RAW) };
const Fix32 Fix32::INF{ Fix32::from_raw(Fix32::INF_RAW) };
const Fix32 Fix32::NaN{ Fix32::from_raw(Fix32::NaN_RAW) };
const Fix32 Fix32::DELTA{ Fix32::from_raw(Fix32::DELTA_RAW) };

template float Fix32::to_real<float>() const;
template double Fix32::to_real<double>() const;
template long double Fix32::to_real<long double>() const;
