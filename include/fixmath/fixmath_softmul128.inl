/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

inline uint64_t _softumul128(uint64_t a, uint64_t b, uint64_t& uhi) {
	uint64_t ahi = a >> 32;
	uint64_t alo = a & 0xFFFF'FFFF;
	uint64_t bhi = b >> 32;
	uint64_t blo = b & 0xFFFF'FFFF;

	uint64_t ulo = alo * blo;
	uhi = ahi * bhi;

	uint64_t carry1;
	uint64_t alo_bhi = alo * bhi;
	ulo = _fm_checked_add(ulo, alo_bhi << 32, carry1);
	uhi += (alo_bhi >> 32) + carry1;

	uint64_t carry2;
	uint64_t ahi_blo = ahi * blo;
	ulo = _fm_checked_add(ulo, ahi_blo << 32, carry2);
	uhi += (ahi_blo >> 32) + carry2;

	return ulo;
}

inline int64_t _softmul128(int64_t a, int64_t b, int64_t& rhi) {
	uint64_t va = static_cast<uint64_t>(a);
	uint64_t vb = static_cast<uint64_t>(b);
	if (a < 0) {
		va = 0 - va;
	}
	if (b < 0) {
		vb = 0 - vb;
	}
	uint64_t uhi = 0;
	uint64_t ulo = _softumul128(va, vb, uhi);
	if ((a ^ b) < 0) {
		_fm_neg128(uhi, ulo);
	}
	rhi = static_cast<int64_t>(uhi);
	return static_cast<int64_t>(ulo);
}

} // namespace fixmath
