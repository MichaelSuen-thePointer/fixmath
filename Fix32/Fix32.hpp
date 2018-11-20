#pragma once
#include <cstdint>
#include <tuple>
using std::int64_t;
using std::uint64_t;

class Fix32
{
	int64_t _value = 0;
	static Fix32 from_raw(int64_t value)
	{
		Fix32 r;
		r._value = value;
		return r;
	}
public:
	const static Fix32 MAX;
	const static Fix32 MIN;
	static Fix32 from_integer(int value);
	static Fix32 from_integer(uint32_t value);
	static Fix32 from_integer(int64_t value);
	static Fix32 from_integer(uint64_t value);
	static Fix32 from_real(float value);
	static Fix32 from_real(double value);
	Fix32() = default;
	Fix32(int value);
	explicit Fix32(uint32_t value);
	explicit Fix32(int64_t value);
	explicit Fix32(uint64_t value);
	explicit Fix32(float value);
	explicit Fix32(double value);
	
	template<class T>
	T to_real() const;

	friend Fix32 operator+(Fix32 a, Fix32 b);
	friend Fix32 operator-(Fix32 a, Fix32 b);
	friend Fix32 operator*(Fix32 a, Fix32 b);
	friend Fix32 operator/(Fix32 a, Fix32 b);

	friend Fix32& operator+=(Fix32& a, Fix32 b);
	friend Fix32& operator-=(Fix32& a, Fix32 b);
	friend Fix32& operator*=(Fix32& a, Fix32 b);
	friend Fix32& operator/=(Fix32& a, Fix32 b);

	int compare(Fix32 b) const;
	friend bool operator>(Fix32 a, Fix32 b);
	friend bool operator>=(Fix32 a, Fix32 b);
	friend bool operator<(Fix32 a, Fix32 b);
	friend bool operator<=(Fix32 a, Fix32 b);
	friend bool operator==(Fix32 a, Fix32 b);
	friend bool operator!=(Fix32 a, Fix32 b);
};

template<class T>
inline T Fix32::to_real() const
{
	static_assert("T must be one of float, double or long double");
	return T();
}

template<>
float Fix32::to_real<float>() const;
template<>
double Fix32::to_real<double>() const;
template<>
long double Fix32::to_real<long double>() const;

//extern template float Fix32::to_real<float>() const;
//extern template double Fix32::to_real<double>() const;
//extern template long double Fix32::to_real<long double>() const;