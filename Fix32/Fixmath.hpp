#pragma once
#include "Fix32.hpp"
#include <cassert>
#include <iterator>

Fix32 abs(Fix32 a)
{
	if unlikely(a.is_nan())
	{
		return a;
	}
	return a > 0 ? a : -a;
}

std::pair<int, Fix32> debug_sqrt(Fix32 a)
{
	Fix32 result = 1;
	Fix32 new_result = 1;
	int count = 0;
	do {
		result = new_result;
		auto new_result = (result + a / result) / 2;
		count += 1;
	} while (new_result == result);
	return { count, result };
}

constexpr static inline Fix32 sin_table[] = {
	Fix32::from_raw(0),
	Fix32::from_raw(0b0'00001000011000000100011100011110ll),
	Fix32::from_raw(0b0'00010000101111100100001001101101ll),
	Fix32::from_raw(0b0'00011001000101111010011010111100ll),
	Fix32::from_raw(0b0'00100001011010100010101000011110ll),
	Fix32::from_raw(0b0'00101001101100111000010010001010ll),
	Fix32::from_raw(0b0'00110001111100010111000001111000ll),
	Fix32::from_raw(0b0'00111010001000011010101110000011ll),
	Fix32::from_raw(0b0'01000010010000011111011100000110ll),
	Fix32::from_raw(0b0'01001010010100000001100010111011ll),
	Fix32::from_raw(0b0'01010010010010011101101101010111ll),
	Fix32::from_raw(0b0'01011010001011010000111100100011ll),
	Fix32::from_raw(0b0'01100001111101111000101010011010ll),
	Fix32::from_raw(0b0'01101001101001110010101011111011ll),
	Fix32::from_raw(0b0'01110001001110011101010011100011ll),
	Fix32::from_raw(0b0'01111000101011010111010011100000ll),
	Fix32::from_raw(0b0'10000000000000000000000000000000ll),
	Fix32::from_raw(0b0'10000111001011110111010001100100ll),
	Fix32::from_raw(0b0'10001110001110011101100111001101ll),
	Fix32::from_raw(0b0'10010101000111010100001000100010ll),
	Fix32::from_raw(0b0'10011011110101111100100111111100ll),
	Fix32::from_raw(0b0'10100010011001111001100100101000ll),
	Fix32::from_raw(0b0'10101000110010101110001100101000ll),
	Fix32::from_raw(0b0'10101110111111111110011110110100ll),
	Fix32::from_raw(0b0'10110101000001001111001100110011ll),
	Fix32::from_raw(0b0'10111010110110000101111100110010ll),
	Fix32::from_raw(0b0'11000000011110001001001011011000ll),
	Fix32::from_raw(0b0'11000101111001000000001101011000ll),
	Fix32::from_raw(0b0'11001011000110010011010001011010ll),
	Fix32::from_raw(0b0'11010000000101101011100001100110ll),
	Fix32::from_raw(0b0'11010100110110110011000101001000ll),
	Fix32::from_raw(0b0'11011001011001010101000001101101ll),
	Fix32::from_raw(0b0'11011101101100111101011101000010ll),
	Fix32::from_raw(0b0'11100001110001011001011110001100ll),
	Fix32::from_raw(0b0'11100101100110010111001110110101ll),
	Fix32::from_raw(0b0'11101001001011100101111100100011ll),
	Fix32::from_raw(0b0'11101100100000110101111001111001ll),
	Fix32::from_raw(0b0'11101111100101111000011111100011ll),
	Fix32::from_raw(0b0'11110010011010100000001101010001ll),
	Fix32::from_raw(0b0'11110100111110100000101010110110ll),
	Fix32::from_raw(0b0'11110111010001101110101000111010ll),
	Fix32::from_raw(0b0'11111001010100000000000001110000ll),
	Fix32::from_raw(0b0'11111011000101001011111001111111ll),
	Fix32::from_raw(0b0'11111100100101001010100001001100ll),
	Fix32::from_raw(0b0'11111101110011110101010010010111ll),
	Fix32::from_raw(0b0'11111110110001000110110100011110ll),
	Fix32::from_raw(0b0'11111111011100111010111010110001ll),
	Fix32::from_raw(0b0'11111111110111001110100101000100ll),
	Fix32::from_raw(0b1'00000000000000000000000000000000ll),
};

constexpr static inline Fix32 C_1_DIV_2 = Fix32::from_raw(0b0'10000000000000000000000000000000ll);
constexpr static inline Fix32 C_1_DIV_6 = Fix32::from_raw(0b0'10000000000000000000000000000000ll);

Fix32 sin(Fix32 a)
{
	if unlikely(a.is_nan() || a.is_infinity())
	{
		return Fix32::NaN;
	}
	bool neg = a < 0;
	a = abs(a);
	neg = neg ^ (a % Fix32::PI_MUL_2 > Fix32::PI);
	a = a % Fix32::PI;
	if (a > Fix32::PI_DIV_2) {
		a = Fix32::PI - a;
	}
	int nearest_table = static_cast<int>(a / Fix32::SIN_TABLE_STEP);
	Fix32 x = a % Fix32::SIN_TABLE_STEP;
	if (x == 0) {
		return neg ? -sin_table[nearest_table] : sin_table[nearest_table];
	}
	if (x > Fix32::SIN_TABLE_STEP / 2) {
		x = x - Fix32::SIN_TABLE_STEP;
		nearest_table += 1;
	}
	Fix32 sin_t = sin_table[nearest_table];
	Fix32 cos_t = sin_table[std::size(sin_table) - nearest_table - 1];
	Fix32 result = sin_t;
	//max num of iteration needed is 5
	//manual loop unroll
	{
		Fix32 t = x * cos_t;
		result += t;
	}
	Fix32 pow_x = x * x;
	{
		Fix32 t = -(pow_x * sin_t / 2);
		result += t;
	}
	pow_x *= x;
	{
		Fix32 t = -(pow_x * cos_t / 6);
		result += t;
	}
	pow_x *= x;
	{
		Fix32 t = pow_x * sin_t / 24;
		result += t;
	}
	return neg ? -result : result;
}

std::pair<int, Fix32> debug_sin(Fix32 a)
{
	if unlikely(a.is_nan() || a.is_infinity())
	{
		return { 0, Fix32::NaN };
	}
	bool neg = a < 0;
	a = abs(a);
	a = a % Fix32::PI;
	if (a > Fix32::PI_DIV_2) {
		a = Fix32::PI - a;
	}
	int nearest_table = static_cast<int>(a / Fix32::SIN_TABLE_STEP);
	Fix32 x = a % Fix32::SIN_TABLE_STEP;
	if (x > Fix32::SIN_TABLE_STEP / 2) {
		x = x - Fix32::SIN_TABLE_STEP;
		nearest_table += 1;
	}
	Fix32 curr_a = sin_table[nearest_table];
	Fix32 next_a = sin_table[std::size(sin_table) - nearest_table - 1];
	Fix32 result = 0;
	Fix32 new_result = curr_a;
	int sign = 0;
	Fix32 pow_x = 1;
	Fix32 bottom = 1;
	int i;
	for(i = 0; i < 5; i++) {
		std::swap(curr_a, next_a);
		result = new_result;
		pow_x *= x;
		sign += 1;
		sign &= 0x3;
		bottom *= i+1;
		//printf("%lld * %lld / %d -> ", curr_a.to_raw(), pow_x.to_raw(), (int)bottom);
		Fix32 t = curr_a * pow_x / bottom;
		if (sign >> 1) {
			t = -t;
		}
		new_result += t;
		//printf("%lld, %lld\t", t.to_raw(), new_result.to_raw());
	}
	return { i, result };
}