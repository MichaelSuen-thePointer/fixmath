/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#define _NDEBUG 0
#include <cmath>
#include <limits>
#include <random>
#include "gtest/gtest.h"
#define FIXMATH_USE_ASSERT 1
#include "fixed.hpp"
using namespace fixmath;

std::mt19937_64 mtg{ std::random_device{}() };
const double pi = std::acos(-1);

namespace {


using i32 = fixmath::int32_t;
using i64 = fixmath::int64_t;
using u64 = fixmath::uint64_t;
using i32l = std::numeric_limits<i32>;
using i64l = std::numeric_limits<i64>;
using f32l = std::numeric_limits<float>;
using f64l = std::numeric_limits<double>;

using fix32l = std::numeric_limits<fixmath::Fix32>;

const double ABSERROR = static_cast<double>(fix32l::epsilon());

#define EXPECT_FIX_NEAR(a, b) EXPECT_NEAR((double)(a), (double)(b), ABSERROR)
#define EXPECT_FIX_POS_OVERFLOW(a) EXPECT_EQ((a), Fix32::max_sat())
#define EXPECT_FIX_NEG_OVERFLOW(a) EXPECT_EQ((a), Fix32::min_sat())
#define EXPECT_FIX_DOMAIN_ERROR(a) EXPECT_DEBUG_DEATH((a), ".*")

static_assert(Fix32::MAX_REPRESENTABLE_DOUBLE == 0x1.ffff'ffff'ffff'fp30);
static_assert(Fix32::MIN_REPRESENTABLE_DOUBLE == -0x1.0p31);

TEST(FIXMATH, INT128MUL) {

	std::uniform_int_distribution<i64> rand{i64l::min(), i64l::max()};
	for (int i = 0; i < 1048576; ++i) {
		i64 a = rand(mtg);
		i64 b = rand(mtg);
		i64 ahi;
		i64 alo = fixmath::_fm_mul128(a, b, ahi);
		i64 bhi;
		i64 blo = fixmath::_softmul128(a, b, bhi);
		EXPECT_EQ(ahi, bhi);
		EXPECT_EQ(alo, blo);
	}
	for (int i = 0; i < 1048576; ++i) {
		i64 a = rand(mtg) >> 32;
		i64 b = rand(mtg) >> 32;
		i64 ahi;
		i64 alo = fixmath::_fm_mul128(a, b, ahi);
		i64 bhi;
		i64 blo = fixmath::_softmul128(a, b, bhi);
		EXPECT_EQ(ahi, bhi);
		EXPECT_EQ(alo, blo);
	}
	for (int i = 0; i < 1048576; ++i) {
		i64 a = rand(mtg) << 32;
		i64 b = rand(mtg) << 32;
		i64 ahi;
		i64 alo = fixmath::_fm_mul128(a, b, ahi);
		i64 bhi;
		i64 blo = fixmath::_softmul128(a, b, bhi);
		EXPECT_EQ(ahi, bhi);
		EXPECT_EQ(alo, blo);
	}
	for (int i = 0; i < 1048576; ++i) {
		i64 a = rand(mtg) << 32;
		i64 b = rand(mtg) >> 32;
		i64 ahi;
		i64 alo = fixmath::_fm_mul128(a, b, ahi);
		i64 bhi;
		i64 blo = fixmath::_softmul128(a, b, bhi);
		EXPECT_EQ(ahi, bhi);
		EXPECT_EQ(alo, blo);
	}
}

TEST(FIXMATH, INT128DIV) {
	std::uniform_int_distribution<i64> rand{i64l::min(), i64l::max()};
	for (int i = 0; i < 1048576; ++i) {
		i64 a = rand(mtg) >> 32;
		i64 b = rand(mtg);
		i64 c = rand(mtg);
		i64 arem;
		u64 ubrem;
		auto ra = fixmath::_fm_div128(a, b, c, arem);
		u64 ua = a;
		u64 ub = b;
		u64 uc = c;
		if (a < 0) {
			ub = 0 - ub;
			ua = ub == 0 ? 0 - ua : ~ua;
		}
		if (c < 0) {
			uc = 0 - uc;
		}
		u64 ubhi = 0;
		if (ua >= uc) {
			ubhi = ua / uc;
			ubrem = ua % uc;
		} else {
			ubhi = 0;
			ubrem = ua;
		}
		auto ublo = fixmath::_softudiv128(ubrem, ub, uc, &ubrem);
		if ((a ^ c) < 0) {
			ubhi = ublo ? ~ubhi : 0 - ubhi;
			ublo = 0 - ublo;
		}
		if (a < 0) {
			ubrem = 0 - ubrem;
		}
		EXPECT_EQ(ra.hi, (i64)ubhi);
		EXPECT_EQ(ra.lo, (i64)ublo);
		EXPECT_EQ(arem, (i64)ubrem);
	}
}

TEST(FIXMATH, CAST) {
	EXPECT_FIX_NEAR(Fix32(0.3), 0.3);
	EXPECT_FIX_NEAR(Fix32(-0.3), -0.3);
	EXPECT_FIX_NEAR(Fix32(-0.3), -0.3);
	EXPECT_FIX_NEAR(Fix32(0.125), 0.125);
	EXPECT_FIX_NEAR(Fix32(-0.125), -0.125);
	EXPECT_FIX_NEAR(Fix32(10000.25), Fix32(10000.25));
	EXPECT_FIX_NEAR(Fix32(2147483647.99998), 2147483647.99998);
	EXPECT_FIX_NEAR(Fix32(-2147483647.99998), -2147483647.99998);

	EXPECT_EQ(Fix32(1.16415321826934814453125e-10), Fix32::epsilon());

	EXPECT_FIX_NEAR(Fix32(-2147483648.0), -2147483648.0);
	EXPECT_FIX_POS_OVERFLOW(Fix32(2147483648.0));
	EXPECT_FIX_NEG_OVERFLOW(Fix32(-2147483648.00001));

	auto big = f64l::infinity();
	EXPECT_FIX_POS_OVERFLOW(Fix32(big));
	EXPECT_FIX_NEG_OVERFLOW(Fix32(-big));
	auto nan = f64l::quiet_NaN();
	EXPECT_TRUE(Fix32(nan) == Fix32::nan());
	EXPECT_EQ(Fix32(0.0), 0);

	EXPECT_EQ(Fix32(1), 1);
	EXPECT_EQ(Fix32(-1), -1);
	EXPECT_EQ(Fix32(100), 100);
	EXPECT_EQ(Fix32(-100), -100);
	EXPECT_EQ(Fix32(2147483647), 2147483647);
	EXPECT_EQ(Fix32(-2147483647), -2147483647);
	EXPECT_EQ(Fix32(-2147483647-1), -2147483647-1);
	EXPECT_FIX_NEAR(Fix32(26061.96161111111), 26061.9616111111);

}

TEST(FIXMATH, CONSTANT) {
	EXPECT_EQ(double(Fix32::epsilon()), 2.3283064365386962890625e-10);
	EXPECT_FALSE(Fix32::nan().is_nan());
	EXPECT_FALSE(Fix32::max_sat().is_inf());
	EXPECT_FALSE(Fix32::min_sat().is_inf());
	EXPECT_EQ(Fix32::min_sat(), Fix32::nan());
	EXPECT_EQ(Fix32::nan(), Fix32::nan());
	EXPECT_EQ(-Fix32::min_sat(), Fix32::min_sat());
	EXPECT_EQ(Fix32::min_sat(), -Fix32::max_sat() - Fix32::epsilon());
	EXPECT_EQ(Fix32::min_sat(), Fix32::min_fix());
	EXPECT_EQ(Fix32::max_sat(), Fix32::max_fix());
}

TEST(FIXMATH, ROUNDING) {
	auto p = Fix32::epsilon();
	auto p1 = p;
	EXPECT_EQ(p1 / Fix32(2), Fix32(0));
	auto p11= p * Fix32(2) + p;
	auto p10 = p * Fix32(2);
	EXPECT_EQ(p11 / Fix32(2), p10); // 3 / 2 = 1.5 ~ 2 余数0.5，商奇数，舍入
	auto p100 = p * Fix32(4);
	EXPECT_EQ(p100 / Fix32(2), p10); // 4/2 = 2 不舍入
	auto p101 = p100 + p;
	EXPECT_EQ(p101 / Fix32(2), p10); // 5/2 = 2 余数0.5，商偶数不舍入
	auto p111 = p101 + p10;
	EXPECT_EQ(p111 / Fix32(2), p100); // 7/2 = 4 余数0.5，商奇数，舍入
	auto p1001 = p100 * Fix32(2) + p;
	EXPECT_EQ(p1001 / Fix32(2), p100); // 9/2 = 4 余数0.5，商偶数不舍入
	auto p1101 = p111 * Fix32(2) - p10 + p;
	EXPECT_EQ(p1101 / Fix32(8), p10); // 13/8 = 1.625 = 2 除数偶数，余数大于0.5，舍入
	EXPECT_EQ(p1101 / Fix32(5), p11); // 13/5 = 2.6 = 3 除数偶数，余数大于0.5，舍入
	EXPECT_EQ(p1101 / Fix32(3), p100); // 13/3 = 4.333 = 4 除数奇数，余数小于0.5，不舍入
	EXPECT_EQ(p1101 / Fix32(4), p11); // 13/4 = 3.25 = 3 除数偶数，余数小于0.5，不舍入

	auto h = Fix32(0.5);
	EXPECT_EQ(p1 * h, Fix32(0));
	EXPECT_EQ(p11 * h, p10); // 3 / 2 = 1.5 ~ 2 余数0.5，商奇数，舍入
	EXPECT_EQ(p100 * h, p10); // 4/2 = 2 不舍入
	EXPECT_EQ(p101 * h, p10); // 5/2 = 2 余数0.5，商偶数不舍入
	EXPECT_EQ(p111 * h, p100); // 7/2 = 4 余数0.5，商奇数，舍入
	EXPECT_EQ(p1001 * h, p100); // 9/2 = 4 余数0.5，商偶数不舍入
	EXPECT_EQ(p1101 * Fix32(0.125), p10); // 13/8 = 1.625 = 2 除数偶数，余数大于0.5，舍入
	EXPECT_EQ(p1101 * Fix32(0.2), p11); // 13/5 = 2.6 = 3 除数偶数，余数大于0.5，舍入
	auto one_third = Fix32(1) / Fix32(3);
	EXPECT_EQ(p1101 * one_third, p100); // 13/3 = 4.333 = 4 除数奇数，余数小于0.5，不舍入
	EXPECT_EQ(p1101 * Fix32(0.25), p11); // 13/4 = 3.25 = 3 除数偶数，余数小于0.5，不舍入
}

TEST(FIXMATH, SUB) {
	EXPECT_FIX_NEAR(Fix32(1.1) - Fix32(0.1), Fix32(1.0));
	EXPECT_FIX_NEAR(Fix32(1.11) - Fix32(0.1), Fix32(1.01));
	EXPECT_FIX_NEAR(Fix32(1.0) - Fix32(0.1), Fix32(0.9));
	EXPECT_FIX_NEAR(Fix32(1.1) - Fix32(0.2), Fix32(0.9));
	EXPECT_FIX_NEAR(Fix32(0.1) - Fix32(1.0), Fix32(-0.9));
	EXPECT_FIX_NEAR(Fix32(1.1111111) - Fix32(0.22222222), Fix32(0.88888888));
	EXPECT_FIX_NEAR(Fix32(i32l::max()) - Fix32(i32l::max()), 0);
	EXPECT_EQ(Fix32(i32l::max()) - Fix32(-i32l::max()), Fix32::max_sat());
	EXPECT_EQ(Fix32(-i32l::max()) - Fix32(i32l::max()), Fix32::min_sat());
	EXPECT_EQ(-1 - Fix32::max_sat(), Fix32::min_sat());
	EXPECT_EQ(Fix32::max_sat() - Fix32::max_sat(), 0);
	EXPECT_EQ(Fix32(1) - 1, 0);
	EXPECT_EQ(1 - Fix32(1), 0);
}

TEST(FIXMATH, ADD) {
	EXPECT_FIX_NEAR(Fix32(1.1) + Fix32(0.1), Fix32(1.2));
	EXPECT_FIX_NEAR(Fix32(1.11) + Fix32(0.1), Fix32(1.21));
	EXPECT_FIX_NEAR(Fix32(0.9) + Fix32(0.1), Fix32(1.0));
	EXPECT_FIX_NEAR(Fix32(0.1) + Fix32(1.0), Fix32(1.1));
	EXPECT_FIX_NEAR(Fix32(1.1111111) + Fix32(0.22222222), Fix32(1.33333332));
	EXPECT_FIX_NEAR(Fix32(i32l::max()) + Fix32(-i32l::max()), 0);
	EXPECT_EQ(Fix32(i32l::max()) + 1, Fix32::max_sat());
	EXPECT_EQ(Fix32(i32l::max()) + Fix32::max_sat(), Fix32::max_sat());
	EXPECT_EQ(Fix32::max_sat() + Fix32::min_sat(), -Fix32::epsilon());
	EXPECT_EQ(Fix32(1) + 1, 2);
	EXPECT_EQ(1 + Fix32(1), 2);
}

TEST(FIXMATH, MUL) {
#define CHECK_FIX_MUL(a, b) EXPECT_FIX_NEAR(Fix32(a) * Fix32(b), double(Fix32(a)) * double(Fix32(b)))
	CHECK_FIX_MUL(1.1, 0.1);
	CHECK_FIX_MUL(1.1111, 0.1);
	CHECK_FIX_MUL(1.1111, 23456);
	CHECK_FIX_MUL(1.1, 34567);
	CHECK_FIX_MUL(3, 0.3333333333333);
	CHECK_FIX_MUL(1.1111111, 0.22222222);
	CHECK_FIX_MUL(3, -0.3333333333333);
	CHECK_FIX_MUL(ABSERROR, 0);
	CHECK_FIX_MUL(-ABSERROR, 0);
	CHECK_FIX_MUL(-1.1111, -0.1);
	EXPECT_EQ(Fix32(32767) * Fix32(32767), 1073676289);
	EXPECT_EQ(Fix32(65536) * Fix32(65536), Fix32::max_sat());
	EXPECT_EQ(Fix32::max_fix() * Fix32::max_fix(), Fix32::max_sat());
	EXPECT_EQ(Fix32::max_fix() * Fix32::min_fix(), Fix32::min_sat());
	EXPECT_EQ(Fix32::min_fix() * Fix32::min_fix(), Fix32::max_sat());
}

TEST(FIXMATH, DIV) {
#define CHECK_FIX_DIV(a, b) EXPECT_FIX_NEAR(Fix32(a) / Fix32(b), double(Fix32(a)) / double(Fix32(b)))
	CHECK_FIX_DIV(55554288, 984466);
	CHECK_FIX_DIV(1.1111, 1);
	CHECK_FIX_DIV(1.111, 3.33333);
	CHECK_FIX_DIV(1.111, 0.333333);
	CHECK_FIX_DIV(1.234, 1.234);
	CHECK_FIX_DIV(1.234, 4.321);
	CHECK_FIX_DIV(ABSERROR, ABSERROR);
	CHECK_FIX_DIV(-ABSERROR, ABSERROR);
	CHECK_FIX_DIV(ABSERROR, -ABSERROR);
	CHECK_FIX_DIV(-1.234, -4.321);
	CHECK_FIX_DIV(-1.234, 4.321);
	EXPECT_EQ(1 / Fix32::epsilon(), Fix32::max_sat());
	EXPECT_EQ(1 / -Fix32::epsilon(), Fix32::min_sat());
	EXPECT_EQ(Fix32::max_fix() / Fix32(0.5), Fix32::max_sat());
	EXPECT_EQ(Fix32::max_fix() / Fix32(-0.5), Fix32::min_sat());
	EXPECT_EQ(Fix32::max_fix() / Fix32::max_fix(), 1);
	EXPECT_EQ(Fix32::min_fix() / Fix32::min_fix(), 1);
	EXPECT_FIX_DOMAIN_ERROR(Fix32(1.1111) / Fix32(0));
}

TEST(FIXMATH, UNARY) {
	EXPECT_FIX_NEAR(double(-Fix32(55554288)), -55554288);
	EXPECT_FIX_NEAR(double(+Fix32(1.1111)), 1.1111);
	EXPECT_FIX_NEAR(double(-Fix32(1.1111)), -1.1111);
	EXPECT_EQ(!Fix32(1.1111), false);
	EXPECT_EQ(!!Fix32(1.1111), true);
	EXPECT_EQ(!!Fix32(ABSERROR), true);
	EXPECT_EQ(!!Fix32(0.), false);
	EXPECT_EQ(!!!Fix32(0.), true);
}

TEST(FIXMATH, COMPARE) {
	EXPECT_EQ(Fix32(55554288) > Fix32(55554288), false);
	EXPECT_EQ(Fix32(1.1) > Fix32(1.01), true);
	EXPECT_EQ(Fix32(ABSERROR) > Fix32(0), true);
	EXPECT_EQ(Fix32(-ABSERROR) > Fix32(0), false);
	EXPECT_EQ(Fix32(-ABSERROR) < Fix32(0), true);
	EXPECT_EQ(Fix32(-ABSERROR) < Fix32(-ABSERROR), false);
	EXPECT_EQ(Fix32(-ABSERROR) <= Fix32(-ABSERROR), true);
	EXPECT_EQ(Fix32(ABSERROR) >= Fix32(ABSERROR), true);
	EXPECT_EQ(Fix32(0) >= Fix32(0), true);
	EXPECT_EQ(Fix32(0) <= Fix32(0), true);
	EXPECT_EQ(Fix32(0) == Fix32(0), true);
	EXPECT_EQ(Fix32(ABSERROR) == Fix32(ABSERROR), true);
	EXPECT_EQ(Fix32(ABSERROR) != Fix32(ABSERROR), false);
	EXPECT_EQ(Fix32(-ABSERROR) != Fix32(ABSERROR), true);
}

template<class T, class U> requires FixedImplicitBinaryOperable<T, U>
constexpr int func(T, U) { return 1; }

template<class T, class U> requires FixedImplicitBinaryOperable<U, T> && std::same_as<T, i32>
constexpr int func(T, U) { return 2; }

TEST(FIXMATH, CONCEPT) {
	static_assert(func(1, Fix32(1)) == 2);
	static_assert(func(Fix32(1), 1) == 1);

	static_cast<const Fix32>(Fix32(1)) + 1;

	static_assert(std::is_same_v<std::common_type_t<Fix32, int>, Fix32>);
	static_assert(std::is_same_v<std::common_type_t<Fix32, int, int>, Fix32>);
	static_assert(std::is_same_v<std::common_type_t<int, Fix32>, Fix32>);
	static_assert(std::is_same_v<std::common_type_t<int, Fix32, Fix32>, Fix32>);
	static_assert(std::is_same_v<std::common_type_t<Fix32, Fix32>, Fix32>);
	static_assert(std::is_same_v<std::common_type_t<Fix32, Fix32, Fix32>, Fix32>);
}

}	// namespace
