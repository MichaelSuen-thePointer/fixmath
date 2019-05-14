#pragma once

#include "Fix32.hpp"

Fix32 abs(Fix32 a)
{
	if unlikely(a.is_nan()) {
		return a;
	}
	return a > 0 ? a : -a;
}

std::pair<int, Fix32> debug_sin(Fix32 a)
{
	if unlikely(a.is_nan() || a.is_infinity()) {
		return { 0, Fix32::NaN };
	}
	bool neg = a < 0;
	a = abs(a);
	a = a % Fix32::PI;
	Fix32 result = a;
	int sign = -1;
	Fix32 bottom = 1;
	int i = 1;
	Fix32 new_result = a;
	int count = 0;
	Fix32 d = a;
	a *= a;
	do {
		result = new_result;
		d *= a;
		if (d.is_infinity()) {
			return { count, result };
		}
		i += 2;
		bottom /= i * (i - 1);
		if (bottom == 0) {
			return { count, result };
		}
		auto t = sign * d * bottom;
		new_result = result + t;
		count += 1;
		sign = -sign;
	} while (new_result != result);
	return { count, result };
}