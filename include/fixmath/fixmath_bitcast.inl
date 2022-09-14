/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

constexpr double _fm_assemble_double(int mantissa_bits, int exponent) {
    mantissa_bits = ::std::min(mantissa_bits, 52);
    uint64_t raw = 0;
    raw |= uint64_t((exponent + 1023) & 0x7FF) << 52;
    raw |= (uint64_t(-1) >> 1 >> (63 - mantissa_bits)) << (52 - mantissa_bits);
    return ::std::bit_cast<double>(raw);
}

constexpr float _fm_assemble_float(int mantissa_bits, int exponent) {
    mantissa_bits = ::std::min(mantissa_bits, 23);
    uint32_t raw = 0;
    raw |= uint32_t((exponent + 127) & 0xFF) << 23;
    raw |= (uint32_t(-1) >> 1 >> (31 - mantissa_bits)) << (23 - mantissa_bits);
    return ::std::bit_cast<float>(raw);
}

} // namespace fixmath
