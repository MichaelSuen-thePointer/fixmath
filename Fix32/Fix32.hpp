#pragma once
#include <cstdint>

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
	static Fix32 from_float(float value);
	Fix32() = default;
	Fix32(int value)
		: _value((int64_t)value << 32)
	{
	}
	Fix32(float value)
		: Fix32(Fix32::from_float(value))
	{
	}
	float to_float() const;

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