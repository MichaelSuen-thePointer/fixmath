/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

template <class R, size_t fraction_bits, bool round_to_even>
constexpr R _fm_binary_to_fixed_raw(bool negative, uint64_t significand, int exponent) {
	constexpr const int RAW_BITS = sizeof(R) * CHAR_BIT;
	static_assert(::std::is_signed_v<R>, "result type should be signed");
	static_assert(sizeof(R) <= sizeof(int64_t), "result type should fit in int64_t");

	const int shift = exponent + int(fraction_bits);
	uint64_t magnitude = 0;
	if (shift >= 0) {
		magnitude = significand << shift;
	} else {
		const int right_shift = -shift;
		uint64_t truncated = 0;
		bool carry = false;
		if (right_shift < 64) {
			truncated = significand >> right_shift;
			if constexpr (round_to_even) {
				const uint64_t mask = (uint64_t{1} << right_shift) - 1;
				const uint64_t remainder = significand & mask;
				const uint64_t half = uint64_t{1} << (right_shift - 1);
				carry = remainder > half || (remainder == half && (truncated & 1));
			}
		}
		magnitude = truncated + carry;
	}

	int64_t result = 0;
	const uint64_t sign_bit = uint64_t{1} << (RAW_BITS - 1);
	if (negative) {
		result = RAW_BITS == 64 && magnitude == sign_bit ? ::std::numeric_limits<int64_t>::min() : -static_cast<int64_t>(magnitude);
	} else {
		result = static_cast<int64_t>(magnitude);
	}
	return static_cast<R>(result);
}

template <class R, size_t fraction_bits, bool round_to_even>
constexpr R _fm_float_to_fixed_raw(float value) {
	const uint32_t bits = ::std::bit_cast<uint32_t>(value);
	const bool negative = bool(bits >> 31);
	const uint32_t encoded_exponent = (bits >> 23) & 0xFF;
	uint64_t significand = bits & 0x7F'FFFF;
	int exponent = 1 - 127 - 23;
	if (encoded_exponent != 0) {
		significand |= 0x80'0000;
		exponent = int(encoded_exponent) - 127 - 23;
	}
	return _fm_binary_to_fixed_raw<R, fraction_bits, round_to_even>(negative, significand, exponent);
}

template <class R, size_t fraction_bits, bool round_to_even>
constexpr R _fm_float_to_fixed_raw(double value) {
	const uint64_t bits = ::std::bit_cast<uint64_t>(value);
	const bool negative = bool(bits >> 63);
	const uint64_t encoded_exponent = (bits >> 52) & 0x7FF;
	uint64_t significand = bits & 0xF'FFFF'FFFF'FFFF;
	int exponent = 1 - 1023 - 52;
	if (encoded_exponent != 0) {
		significand |= 0x10'0000'0000'0000;
		exponent = int(encoded_exponent) - 1023 - 52;
	}
	return _fm_binary_to_fixed_raw<R, fraction_bits, round_to_even>(negative, significand, exponent);
}

constexpr double _fm_assemble_double(int mantissa_bits, int exponent) {
	mantissa_bits = ::std::min(mantissa_bits, 52);
	uint64_t raw = 0;
	const uint64_t raw_exponent = (exponent + 1023) & 0x7FF;
	raw |= raw_exponent << 52;
	raw |= ((~uint64_t{0}) >> 1 >> (63 - mantissa_bits)) << (52 - mantissa_bits);
	return ::std::bit_cast<double>(raw);
}

constexpr float _fm_assemble_float(int mantissa_bits, int exponent) {
	mantissa_bits = ::std::min(mantissa_bits, 23);
	uint32_t raw = 0;
	const uint32_t raw_exponent = (exponent + 127) & 0xFF;
	raw |= raw_exponent << 23;
	raw |= ((~uint32_t{0}) >> 1 >> (31 - mantissa_bits)) << (23 - mantissa_bits);
	return ::std::bit_cast<float>(raw);
}

} // namespace fixmath
