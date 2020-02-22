#pragma once

#include <cstdint>
#include <tuple>
#include <algorithm>
#include <iosfwd>
#include <string>

#if defined __cpp_lib_three_way_comparison
# include <compare>
using partial_ordering = std::partial_ordering;
#else

struct MustBeZeroTag;
using MustBeZeroParam = void (MustBeZeroTag::*)();

class partial_ordering {
	signed char _value;
	constexpr partial_ordering(int value) : _value(value) {}
public:
	const static partial_ordering less;
	const static partial_ordering equivalent;
	const static partial_ordering greater;
	const static partial_ordering unordered;

	friend constexpr bool operator==(partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator!=(partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator< (partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator<=(partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator> (partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator>=(partial_ordering v, MustBeZeroParam);
	friend constexpr bool operator==(MustBeZeroParam, partial_ordering v);
	friend constexpr bool operator!=(MustBeZeroParam, partial_ordering v);
	friend constexpr bool operator< (MustBeZeroParam, partial_ordering v);
	friend constexpr bool operator<=(MustBeZeroParam, partial_ordering v);
	friend constexpr bool operator> (MustBeZeroParam, partial_ordering v);
	friend constexpr bool operator>=(MustBeZeroParam, partial_ordering v);
};

inline constexpr bool operator==(partial_ordering v, MustBeZeroParam) { return v._value != -127 && v._value == 0; }
inline constexpr bool operator< (partial_ordering v, MustBeZeroParam) { return v._value != -127 && v._value < 0; }
inline constexpr bool operator<=(partial_ordering v, MustBeZeroParam) { return v._value != -127 && v._value <= 0; }
inline constexpr bool operator> (partial_ordering v, MustBeZeroParam) { return v._value != -127 && v._value > 0; }
inline constexpr bool operator>=(partial_ordering v, MustBeZeroParam) { return v._value != -127 && v._value >= 0; }
inline constexpr bool operator==(MustBeZeroParam, partial_ordering v) { return v._value != -127 && 0 == v._value; }
inline constexpr bool operator< (MustBeZeroParam, partial_ordering v) { return v._value != -127 && 0 < v._value; }
inline constexpr bool operator<=(MustBeZeroParam, partial_ordering v) { return v._value != -127 && 0 <= v._value; }
inline constexpr bool operator> (MustBeZeroParam, partial_ordering v) { return v._value != -127 && 0 > v._value; }
inline constexpr bool operator>=(MustBeZeroParam, partial_ordering v) { return v._value != -127 && 0 >= v._value; }
inline constexpr bool operator!=(partial_ordering v, MustBeZeroParam) { return v._value == -127 || v._value != 0; }
inline constexpr bool operator!=(MustBeZeroParam, partial_ordering v) { return v._value == -127 || v._value != 0; }

#endif

using std::int64_t;
using std::uint64_t;


class Fix32 {
	int64_t _value = 0;
	struct from_raw_init_t {};
	constexpr Fix32(int64_t raw, from_raw_init_t) : _value(raw) {}
public:
	constexpr static Fix32 from_raw(int64_t value) { return { value, from_raw_init_t{} }; }
	const static int64_t ZERO_RAW;
	const static int64_t ONE_RAW;
	const static int64_t MIN_RAW;
	const static int64_t MAX_RAW;
	const static int64_t MAX_INT_RAW;
	const static int64_t MIN_INT_RAW;
	const static int64_t INF_RAW;
	const static int64_t NaN_RAW;
	const static int64_t DELTA_RAW;
	const static int64_t PI_RAW;
	const static int64_t E_RAW;

	const static Fix32 ZERO;
	const static Fix32 ONE;
	const static Fix32 MAX;
	const static Fix32 MIN;
	const static Fix32 MAX_INT;
	const static Fix32 MIN_INT;
	const static Fix32 INF;
	const static Fix32 NaN;
	const static Fix32 DELTA;
	const static Fix32 E;
	const static Fix32 PI;
	const static Fix32 PI_MUL_2;
	const static Fix32 PI_DIV_6;
	const static Fix32 PI_DIV_3;
	const static Fix32 PI_DIV_2;
	const static Fix32 SIN_TABLE_STEP;

	static Fix32 from_integer(int value);
	static Fix32 from_integer(uint32_t value);
	static Fix32 from_integer(int64_t value);
	static Fix32 from_integer(uint64_t value);
	static Fix32 from_real(float value);
	static Fix32 from_real(double value);
	static Fix32 from_string(const std::string& s);

	Fix32() = default;
	Fix32(int value);
	explicit Fix32(uint32_t value);
	explicit Fix32(int64_t value);
	explicit Fix32(uint64_t value);
	explicit Fix32(float value);
	explicit Fix32(double value);

	bool is_positive_infinity() const;
	bool is_negative_infinity() const;
	bool is_infinity() const;
	int infinity_sign() const;
	bool is_nan() const;

	template<class T>
	T to_real() const;

	std::string to_string() const;
	int64_t to_raw() const;

	explicit operator int() const { return static_cast<int>(_value >> 32); }

	friend Fix32 operator+(Fix32 a);
	friend Fix32 operator-(Fix32 a);

	friend Fix32 operator+(Fix32 a, Fix32 b);
	friend Fix32 operator-(Fix32 a, Fix32 b);
	friend Fix32 operator*(Fix32 a, Fix32 b);
	friend Fix32 operator/(Fix32 a, Fix32 b);
	friend Fix32 operator%(Fix32 a, Fix32 b);

	friend Fix32& operator+=(Fix32& a, Fix32 b);
	friend Fix32& operator-=(Fix32& a, Fix32 b);
	friend Fix32& operator*=(Fix32& a, Fix32 b);
	friend Fix32& operator/=(Fix32& a, Fix32 b);
	friend Fix32& operator%=(Fix32& a, Fix32 b);

	partial_ordering compare(Fix32 b) const;
	friend bool operator>(Fix32 a, Fix32 b);
	friend bool operator>=(Fix32 a, Fix32 b);
	friend bool operator<(Fix32 a, Fix32 b);
	friend bool operator<=(Fix32 a, Fix32 b);
	friend bool operator==(Fix32 a, Fix32 b);
	friend bool operator!=(Fix32 a, Fix32 b);

	friend std::ostream& operator<<(std::ostream& os, Fix32 a);
};

template<class T>
inline T Fix32::to_real() const {
	static_assert("T must be one of float, double or long double");
	return T();
}

template<>
float Fix32::to_real<float>() const;
template<>
double Fix32::to_real<double>() const;
template<>
long double Fix32::to_real<long double>() const;

#if defined _MSC_VER
#  define likely(expr)    (expr)
#  define unlikely(expr)  (expr)
#  define assume(cond)
#else
#  define likely(expr)    (__builtin_expect(!!(expr), 1))
#  define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#  define assume(cond)    do { if (!(cond)) __builtin_unreachable(); } while (0)
#endif