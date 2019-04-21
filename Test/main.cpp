#define BOOST_TEST_MODULE Fix32

#include <boost/test/unit_test.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <tuple>
#include <random>

using std::pair;
using std::tuple;
using std::make_pair;
using namespace boost::unit_test;
using namespace boost::multiprecision;

int128_t make_int128_t(pair<uint64_t, uint64_t> v) {
	auto[lo, hi] = v;
	return (((int128_t)(int64_t)hi) << 64) | (int128_t)lo;
}

int128_t make_uint128_t(pair<uint64_t, uint64_t> v) {
	auto[lo, hi] = v;
	return (((uint128_t)(uint64_t)hi) << 64) | (uint128_t)lo;
}

BOOST_AUTO_TEST_CASE(helper_function_clzll) {
	int clzll(uint64_t value);
	BOOST_TEST(clzll(0xFFFF'FFFF'FFFF'FFFF) == 0);
	BOOST_TEST(clzll(0x7FFF'FFFF'FFFF'FFFF) == 1);
	BOOST_TEST(clzll(0x1'FFFF'FFFF) == 31);
	BOOST_TEST(clzll(0xFFFF'FFFF) == 32);
	BOOST_TEST(clzll(0x7FFF'FFFF) == 33);
	BOOST_TEST(clzll(1) == 63);
	BOOST_TEST(clzll(0) == 64);
}

BOOST_AUTO_TEST_CASE(helper_function_negate) {
	pair<uint64_t, uint64_t> negate(uint64_t low, int64_t high);

#define CHECK(lo, hi) BOOST_TEST(-make_int128_t({(uint64_t)lo, hi}) == make_int128_t(negate(lo, hi)))
	CHECK(1, 0);
	CHECK(0, 0);
	CHECK(-1, -1);
	CHECK(-1, 0x7FFF'FFFF'FFFF'FFFF);
	CHECK(0x123456788765431, 0x876543212345678);
#undef CHECK
}

BOOST_AUTO_TEST_CASE(helper_function_int128_mul) {
	pair<uint64_t, int64_t> int128_mul(int64_t _a, int64_t _b);

#define CHECK(a, b) BOOST_TEST(make_int128_t(int128_mul((a), (b))) == (int128_t)(a)*(int128_t)(b))

	CHECK(0, 0);
	CHECK(0, 1);
	CHECK(1, 0);
	CHECK(0, -1);
	CHECK(-1, 0);
	CHECK(-1, 1);
	CHECK(1, -1);
	CHECK(1, 1);
	CHECK(-1, -1);
	CHECK(123, 456);
	CHECK(123, -456);
	CHECK(-123, -456);
	CHECK(2147483647, 4294967296);
	CHECK(2147483647, -4294967296);
	CHECK(-2147483647, -4294967296);
	CHECK(12345678987654321, 98765432123456789);
	CHECK(12345678987654321, -98765432123456789);
	CHECK(-12345678987654321, -98765432123456789);
	using i64 = std::numeric_limits<int64_t>;
	CHECK(i64::max(), i64::max());
	CHECK(i64::max(), i64::min() + 1);
	CHECK(i64::min() + 1, i64::max());

	//BOOST_TEST(make_int128_t(int128_mul(i64::min(), i64::min())) == make_int128_t({ 0,0 })); // overflow test

	std::mt19937_64 mtg{ std::random_device{}() };
	std::uniform_int_distribution<int64_t> uid{ i64::min() + 1, i64::max() };
	for (int i = 0; i < 10000; i++) {
		auto a = uid(mtg);
		auto b = uid(mtg);
		CHECK(a, b);
	}
#undef CHECK
}



BOOST_AUTO_TEST_CASE(helper_function_int128_div_rem) {
	tuple<uint64_t, int64_t, int64_t> int128_div_rem(uint64_t low, int64_t high, int64_t divisor);

#define CHECK(_lo, _hi, _divisor) [](uint64_t lo, int64_t hi, int64_t divisor)\
	{\
		auto dividend = make_int128_t({ lo, hi });\
		auto quotient = dividend / divisor;\
		auto remainder = dividend % divisor;\
		if (hi < 0 && divisor < 0)\
		{\
			remainder = -remainder; /*-5 % -3 should be -2*/\
		}\
		auto [qlo, qhi, rem] = int128_div_rem(lo, hi, divisor);\
		BOOST_TEST(quotient == make_int128_t({ qlo, qhi }));\
		BOOST_TEST(remainder == rem);\
	}(_lo, _hi, _divisor)
	CHECK(0, 0, 1);
	CHECK(1, 0, 1);
	CHECK(1, 0, -1);
	CHECK(-1, -1, 1);
	CHECK(-1, -1, -1);
	CHECK(5, 0, 3);
	CHECK(5, 0, -3);
	CHECK(-5, -1, 3);
	CHECK(-5, -1, -3);
	using i64 = std::numeric_limits<int64_t>;
	using u64 = std::numeric_limits<uint64_t>;
	CHECK(u64::max(), i64::max(), 1);
	CHECK(u64::max(), i64::max(), -1);
	CHECK(u64::max(), i64::max(), i64::max());
	CHECK(u64::max(), i64::max(), i64::min());
	CHECK(1, 0, i64::max());
	CHECK(1, 0, i64::min());
	{ //overflow case
		auto[lo, hi, rem] = int128_div_rem(0, i64::min(), -1);
		BOOST_TEST(make_int128_t({ lo, hi }) == make_int128_t({ 0, i64::min() }));
		BOOST_TEST(rem == 0);
	}

	std::mt19937_64 mtg{ std::random_device{}() };
	std::uniform_int_distribution<int64_t> uid{ i64::min() + 1, i64::max() };
	std::uniform_int_distribution<uint64_t> uuid{};
	for (int i = 0; i < 10000; ) {
		auto lo = uuid(mtg);
		auto hi = uid(mtg);
		auto div = uid(mtg);
		if (div == 0) {
			continue;
		}
		CHECK(lo, hi, div);
		i++;
	}
#undef CHECK
}

BOOST_AUTO_TEST_CASE(helper_function_shifted_uint64_div) {
	tuple<uint64_t, uint64_t, uint64_t> shifted_uint64_div(uint64_t a, uint64_t b);

#define CHECK(_a, _b) [](uint64_t a, uint64_t b)\
	{\
		uint128_t ta = a;\
		ta <<= 32; \
		auto quotient = ta / b;\
		auto remainder = ta % b;\
		auto [qlo, qhi, rem] = shifted_uint64_div(a, b);\
		BOOST_TEST(quotient == make_uint128_t({ qlo, qhi }));\
		BOOST_TEST(remainder == rem);\
	}(_a, _b)
	CHECK(0, 1);
	CHECK(1, 1);
	CHECK(5, 3);
	using i64 = std::numeric_limits<int64_t>;
	using u64 = std::numeric_limits<uint64_t>;
	CHECK(i64::max(), 1);
	CHECK(i64::max(), i64::max());
	CHECK(0, i64::max());
	CHECK(1, i64::max());
	CHECK(0x7000'0000'0000'0000, 1);
	CHECK(0x1234'5678'1234'5678, 0x1234'5678'1234'5679);

	std::mt19937_64 mtg{ 0 };
	std::uniform_int_distribution<uint64_t> uid{ u64::min() + 1, (uint64_t)i64::max() };
	for (int i = 0; i < 10000; ) {
		auto a = uid(mtg);
		auto b = uid(mtg);
		CHECK(a, b);
		i++;
	}
#undef CHECK
}

#include "../Fix32/Fix32.hpp"

BOOST_AUTO_TEST_CASE(nan_case) {
	BOOST_TEST(Fix32::NaN != Fix32::NaN);
	BOOST_TEST(Fix32::NaN.is_nan());
	BOOST_TEST((-Fix32::NaN).is_nan());
	BOOST_TEST((+Fix32::NaN).is_nan());

	BOOST_TEST(!Fix32::INF.is_nan());
	BOOST_TEST(!(-Fix32::INF).is_nan());
	BOOST_TEST(!Fix32(1).is_nan());
	BOOST_TEST(!Fix32(-1).is_nan());
	BOOST_TEST(!Fix32(0).is_nan());

	BOOST_TEST((Fix32::NaN + Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NaN - Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NaN * Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NaN / Fix32(1)).is_nan());

	BOOST_TEST((Fix32(-1) + Fix32::NaN).is_nan());
	BOOST_TEST((Fix32(-1) - Fix32::NaN).is_nan());
	BOOST_TEST((Fix32(-1) * Fix32::NaN).is_nan());
	BOOST_TEST((Fix32(-1) / Fix32::NaN).is_nan());

	BOOST_TEST((Fix32::NaN + Fix32::NaN).is_nan());
	BOOST_TEST((Fix32::NaN - Fix32::NaN).is_nan());
	BOOST_TEST((Fix32::NaN * Fix32::NaN).is_nan());
	BOOST_TEST((Fix32::NaN / Fix32::NaN).is_nan());

	BOOST_TEST((Fix32(0) / Fix32(0)).is_nan());

	BOOST_TEST((Fix32::INF / -Fix32::INF).is_nan());
	BOOST_TEST((Fix32::INF / Fix32::INF).is_nan());
	BOOST_TEST((-Fix32::INF / Fix32::INF).is_nan());
	BOOST_TEST((-Fix32::INF / -Fix32::INF).is_nan());

	BOOST_TEST((Fix32::INF * Fix32(0)).is_nan());
	BOOST_TEST((-Fix32::INF * Fix32(0)).is_nan());
	BOOST_TEST((Fix32(0) * Fix32::INF).is_nan());
	BOOST_TEST((Fix32(0) * -Fix32::INF).is_nan());
}

BOOST_AUTO_TEST_CASE(add_sub_infinity_case) {
	BOOST_TEST(Fix32::INF + Fix32::INF == Fix32::INF);
	BOOST_TEST((Fix32::INF + -Fix32::INF).is_nan());
	BOOST_TEST((Fix32::INF - Fix32::INF).is_nan());
	BOOST_TEST(Fix32::INF - -Fix32::INF == Fix32::INF);

	BOOST_TEST((-Fix32::INF + Fix32::INF).is_nan());
	BOOST_TEST(-Fix32::INF + -Fix32::INF == -Fix32::INF);
	BOOST_TEST(-Fix32::INF - Fix32::INF == -Fix32::INF);
	BOOST_TEST((-Fix32::INF - -Fix32::INF).is_nan());

	BOOST_TEST(Fix32::INF + 5 == Fix32::INF);
	BOOST_TEST(Fix32::INF - 5 == Fix32::INF);
	BOOST_TEST(-Fix32::INF + 5 == -Fix32::INF);
	BOOST_TEST(-Fix32::INF - 5 == -Fix32::INF);

	BOOST_TEST(5 + Fix32::INF == Fix32::INF);
	BOOST_TEST(5 - Fix32::INF == -Fix32::INF);
	BOOST_TEST(5 + Fix32::INF == Fix32::INF);
	BOOST_TEST(5 - Fix32::INF == -Fix32::INF);
}

BOOST_AUTO_TEST_CASE(add_sub_overflow_case) {
	BOOST_TEST(Fix32(2147483647) != Fix32::INF);

	BOOST_TEST(Fix32(2147483647) + Fix32(1) == Fix32::INF);
	BOOST_TEST(Fix32(2147483647) - Fix32(-1) == Fix32::INF);
	BOOST_TEST(Fix32(2147483647) + Fix32(2147483647) == Fix32::INF);
	BOOST_TEST(Fix32(2147483647) - Fix32(-2147483647) == Fix32::INF);

	BOOST_TEST(Fix32(-2147483647) + Fix32(-1) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647) + Fix32(-2) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647) - Fix32(1) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647) - Fix32(2) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647) + Fix32(-2147483647) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647) - Fix32(2147483647) == -Fix32::INF);

	BOOST_TEST(Fix32(1073741824) + Fix32(1073741824) == Fix32::INF);
	BOOST_TEST(Fix32(1073741824) - Fix32(-1073741824) == Fix32::INF);
	BOOST_TEST(Fix32(-1073741824) + Fix32(-1073741824) == -Fix32::INF);
	BOOST_TEST(Fix32(-1073741824) + Fix32(-1073741825) == -Fix32::INF);
	BOOST_TEST(Fix32(-1073741824) - Fix32(1073741824) == -Fix32::INF);
	BOOST_TEST(Fix32(-1073741824) - Fix32(1073741825) == -Fix32::INF);


	BOOST_TEST(Fix32::MAX + Fix32::DELTA == Fix32::INF);
	BOOST_TEST(Fix32::MAX - -Fix32::DELTA == Fix32::INF);

	BOOST_TEST(-Fix32::MAX + -Fix32::DELTA == -Fix32::INF);
	BOOST_TEST(-Fix32::MAX - Fix32::DELTA == -Fix32::INF);
}

BOOST_AUTO_TEST_CASE(add_sub_normal_case) {
	BOOST_TEST(Fix32(1) + Fix32(1) == Fix32(2));
	BOOST_TEST(Fix32(1) + Fix32(-2) == Fix32(-1));
	BOOST_TEST(Fix32(1) - Fix32(1) == Fix32(0));
	BOOST_TEST(Fix32(1) - Fix32(2) == Fix32(-1));
	BOOST_TEST(Fix32(1) - Fix32(-2) == Fix32(3));
	BOOST_TEST(Fix32(-2) + Fix32(1) == Fix32(-1));
	BOOST_TEST(Fix32(-2) + Fix32(-1) == Fix32(-3));
	BOOST_TEST(Fix32(1073741823) + Fix32(1073741823) == Fix32(2147483646));
	BOOST_TEST(Fix32(2147483646) + Fix32(0) == Fix32(2147483646));
	BOOST_TEST(Fix32(2147483645) + Fix32(1) == Fix32(2147483646));
	BOOST_TEST(Fix32(2147483645) - Fix32(-1) == Fix32(2147483646));

	BOOST_TEST(Fix32(-1073741823) + Fix32(-1073741823) == Fix32(-2147483646));
	BOOST_TEST(Fix32(-1073741823) - Fix32(1073741823) == Fix32(-2147483646));
	BOOST_TEST(Fix32(-2147483646) + Fix32(0) == Fix32(-2147483646));
	BOOST_TEST(Fix32(-2147483645) + Fix32(-1) == Fix32(-2147483646));
	BOOST_TEST(Fix32(-2147483645) - Fix32(1) == Fix32(-2147483646));
}

BOOST_AUTO_TEST_CASE(constants) {
	BOOST_TEST(Fix32::MAX == -Fix32::MIN);
	BOOST_TEST(-Fix32::MAX == Fix32::MIN);
	BOOST_TEST(Fix32::MAX_INT == -Fix32::MIN_INT);
	BOOST_TEST(-Fix32::MAX_INT == Fix32::MIN_INT);

	BOOST_TEST(Fix32::MAX - Fix32::MAX_INT + Fix32::DELTA == Fix32::ONE - Fix32::DELTA);
	BOOST_TEST(Fix32::MIN - Fix32::MIN_INT - Fix32::DELTA == -Fix32::ONE + Fix32::DELTA);

	BOOST_TEST(Fix32(1 / 4294967296.0) == Fix32::DELTA);
}

BOOST_AUTO_TEST_CASE(mul_div_normal_case) {
	BOOST_TEST(Fix32(1) * Fix32(1) == Fix32(1));
	BOOST_TEST(Fix32(1) / Fix32(1) == Fix32(1));
	BOOST_TEST(Fix32(1) * Fix32(-2) == Fix32(-2));
	BOOST_TEST(Fix32(1) / Fix32(-2) == Fix32(-0.5));
	BOOST_TEST(Fix32(1) / Fix32(1) == Fix32(1));
	BOOST_TEST(Fix32(1) / Fix32(2) == Fix32(0.5));
	BOOST_TEST(Fix32(-1) / Fix32(2) == Fix32(-0.5));
	BOOST_TEST(Fix32(-2) * Fix32(1) == Fix32(-2));
	BOOST_TEST(Fix32(-2) * Fix32(-1) == Fix32(2));
	BOOST_TEST(Fix32(1073741823) * Fix32(2) == Fix32(2147483646));
	BOOST_TEST(Fix32(2147483646) * Fix32(0) == Fix32(0));
	BOOST_TEST(Fix32(2147483645) * Fix32(1) == Fix32(2147483645));
	BOOST_TEST(Fix32(1073741823) * Fix32(-2) == Fix32(-2147483646));
	BOOST_TEST(Fix32(-2147483646) / Fix32(-1) == Fix32(2147483646));
	BOOST_TEST(Fix32(2147483646) / Fix32(-1) == Fix32(-2147483646));
	BOOST_TEST(Fix32(2147483646) / Fix32(-2) == Fix32(-1073741823));
	BOOST_TEST(Fix32(2147483646) / Fix32(2) == Fix32(1073741823));
	BOOST_TEST(Fix32(-2147483646) / Fix32(-2) == Fix32(1073741823));
	BOOST_TEST(Fix32(-2147483646) / Fix32(2) == Fix32(-1073741823));

	BOOST_TEST(Fix32(32767) * Fix32(32767) == Fix32(1073676289));
	BOOST_TEST(Fix32(32767) * Fix32(-32767) == Fix32(-1073676289));
	BOOST_TEST(Fix32(-32767) * Fix32(-32767) == Fix32(1073676289));
	BOOST_TEST(Fix32(-32767) * Fix32(32767) == Fix32(-1073676289));

	BOOST_TEST(Fix32(1073676289) / Fix32(32767) == Fix32(32767));
	BOOST_TEST(Fix32(1073676289) / Fix32(-32767) == Fix32(-32767));
	BOOST_TEST(Fix32(-1073676289) / Fix32(-32767) == Fix32(32767));
	BOOST_TEST(Fix32(-1073676289) / Fix32(32767) == Fix32(-32767));

	BOOST_TEST(Fix32(1024) * Fix32(1024) == Fix32(1048576));
	BOOST_TEST(Fix32(1048576) / Fix32(1024) == Fix32(1024));

	BOOST_TEST(Fix32(1.5) * Fix32(2) == Fix32(3));
	BOOST_TEST(Fix32(1.5) / Fix32(2) == Fix32(0.75));

	BOOST_TEST(Fix32::MAX_INT * Fix32::DELTA * 2 == Fix32::ONE - Fix32::DELTA * 2);
}

BOOST_AUTO_TEST_CASE(mul_div_infinity_case) {
	BOOST_TEST(Fix32::INF * Fix32(2) == Fix32::INF);
	BOOST_TEST(Fix32::INF * Fix32(-2) == -Fix32::INF);
	BOOST_TEST(Fix32(2) * Fix32::INF == Fix32::INF);
	BOOST_TEST(Fix32(-2) * Fix32::INF == -Fix32::INF);
	BOOST_TEST(-Fix32::INF * Fix32(2) == -Fix32::INF);
	BOOST_TEST(-Fix32::INF * Fix32(-2) == Fix32::INF);
	BOOST_TEST(Fix32(2) * -Fix32::INF == -Fix32::INF);
	BOOST_TEST(Fix32(-2) * -Fix32::INF == Fix32::INF);
	BOOST_TEST(Fix32::INF / Fix32(2) == Fix32::INF);
	BOOST_TEST(Fix32::INF / Fix32(-2) == -Fix32::INF);
	BOOST_TEST(-Fix32::INF / Fix32(2) == -Fix32::INF);
	BOOST_TEST(-Fix32::INF / Fix32(-2) == Fix32::INF);

	BOOST_TEST(Fix32::INF / Fix32(0) == Fix32::INF);
	BOOST_TEST(-Fix32::INF / Fix32(0) == -Fix32::INF);


	BOOST_TEST(Fix32::INF * Fix32::INF == Fix32::INF);
	BOOST_TEST(-Fix32::INF * Fix32::INF == -Fix32::INF);
	BOOST_TEST(Fix32::INF * -Fix32::INF == -Fix32::INF);
	BOOST_TEST(-Fix32::INF * -Fix32::INF == Fix32::INF);

	BOOST_TEST(Fix32(2) / Fix32::INF == Fix32::ZERO);
	BOOST_TEST(Fix32(-2) / Fix32::INF == Fix32::ZERO);
	BOOST_TEST(Fix32(0) / Fix32::INF == Fix32::ZERO);
}

BOOST_AUTO_TEST_CASE(mul_div_overflow_case) {
	BOOST_TEST(Fix32::MAX * 2 == Fix32::INF);
	BOOST_TEST(Fix32::MAX * -2 == -Fix32::INF);
	BOOST_TEST(-Fix32::MAX * 2 == -Fix32::INF);
	BOOST_TEST(-Fix32::MAX * -2 == Fix32::INF);

	BOOST_TEST(-Fix32::MAX * -2 == Fix32::INF);

	BOOST_TEST(Fix32(2) * Fix32(1073741824) == Fix32::INF);
	BOOST_TEST(-Fix32(2) * Fix32(1073741824) == -Fix32::INF);
	BOOST_TEST(-Fix32(2) * -Fix32(1073741824) == Fix32::INF);
	BOOST_TEST(Fix32(2) * -Fix32(1073741824) == -Fix32::INF);
}

BOOST_AUTO_TEST_CASE(normal_construction) {
	BOOST_TEST(Fix32(1) == Fix32(1));
	BOOST_TEST(Fix32(1) == Fix32(1));
	BOOST_TEST(Fix32(2147483647) == Fix32(2147483647));
	BOOST_TEST(Fix32(-2147483647) == Fix32(-2147483647));


	BOOST_TEST(Fix32(0.5) == Fix32(1) / Fix32(2));
	BOOST_TEST(Fix32(1.5) == Fix32(1) + Fix32(0.5));
	BOOST_TEST(Fix32(2147483647.0) == Fix32(2147483647));
	BOOST_TEST(Fix32(-2147483647.0) == Fix32(-2147483647));


	BOOST_TEST(Fix32(0.5f) == Fix32(1) / Fix32(2));
	BOOST_TEST(Fix32(1.5f) == Fix32(1) + Fix32(0.5));
}

BOOST_AUTO_TEST_CASE(overflow_construction) {
	BOOST_TEST(Fix32(2147483648ll) == Fix32::INF);
	BOOST_TEST(Fix32(-2147483648ll) == -Fix32::INF);
	BOOST_TEST(Fix32(-2147483647 - 1) == -Fix32::INF);
	BOOST_TEST(Fix32(-10000000000000) == -Fix32::INF);
	BOOST_TEST(Fix32(10000000000000) == Fix32::INF);
	BOOST_TEST(Fix32(100000000000000ull) == Fix32::INF);

	BOOST_TEST(Fix32(2147483648.0) == Fix32::INF);
	BOOST_TEST(Fix32(-2147483648.0) == -Fix32::INF);
	BOOST_TEST(Fix32(-10000000000000.0) == -Fix32::INF);
	BOOST_TEST(Fix32(10000000000000.0) == Fix32::INF);

	BOOST_TEST(Fix32(1e100) == Fix32::INF);
	BOOST_TEST(Fix32(-1e100) == -Fix32::INF);

	BOOST_TEST(Fix32(2147483648.0f) == Fix32::INF);
	BOOST_TEST(Fix32(-2147483648.0f) == -Fix32::INF);
	BOOST_TEST(Fix32(-10000000000000.0f) == -Fix32::INF);
	BOOST_TEST(Fix32(10000000000000.0f) == Fix32::INF);

	BOOST_TEST(Fix32(1e20f) == Fix32::INF);
	BOOST_TEST(Fix32(-1e20f) == -Fix32::INF);

	//float cannot represent INT_MAX precisely
	BOOST_TEST(Fix32(2147483647.0f) == Fix32::INF);
	BOOST_TEST(Fix32(-2147483647.0f) == -Fix32::INF);
}

BOOST_AUTO_TEST_CASE(inf_nan_construction) {
	BOOST_TEST(Fix32(std::numeric_limits<float>::infinity()) == Fix32::INF);
	BOOST_TEST(Fix32(-std::numeric_limits<float>::infinity()) == -Fix32::INF);
	BOOST_TEST(Fix32(std::numeric_limits<float>::quiet_NaN()).is_nan());

	BOOST_TEST(Fix32(std::numeric_limits<double>::infinity()) == Fix32::INF);
	BOOST_TEST(Fix32(-std::numeric_limits<double>::infinity()) == -Fix32::INF);
	BOOST_TEST(Fix32(std::numeric_limits<double>::quiet_NaN()).is_nan());
}

BOOST_AUTO_TEST_CASE(comparison) {
	BOOST_TEST(Fix32(1) > Fix32(0));
	BOOST_TEST(Fix32::DELTA > Fix32(0));
	BOOST_TEST(Fix32(1.5) > Fix32(1.5)-Fix32::DELTA);

	BOOST_TEST(Fix32(1) < Fix32(2));
	BOOST_TEST(Fix32::DELTA < Fix32(1));
	BOOST_TEST(Fix32(1.5) + Fix32::DELTA > Fix32(1.5));

	BOOST_TEST(Fix32(1) >= Fix32(0));
	BOOST_TEST(Fix32::DELTA >= Fix32(0));
	BOOST_TEST(Fix32(1.5) >= Fix32(1.5) - Fix32::DELTA);

	BOOST_TEST(Fix32(1) <= Fix32(2));
	BOOST_TEST(Fix32::DELTA <= Fix32(1));
	BOOST_TEST(Fix32(1.5) + Fix32::DELTA >= Fix32(1.5));

	BOOST_TEST(Fix32::DELTA != Fix32::ZERO);
	BOOST_TEST(Fix32::DELTA == Fix32::from_raw(1));

	BOOST_TEST(Fix32::INF > Fix32::MAX);
	BOOST_TEST(-Fix32::INF < Fix32::MIN);

	BOOST_TEST(Fix32::NaN != Fix32::NaN);

	BOOST_TEST(Fix32::NaN > Fix32::ONE == false);
	BOOST_TEST(Fix32::NaN < Fix32::ONE == false);
	BOOST_TEST(Fix32::NaN >= Fix32::ONE == false);
	BOOST_TEST(Fix32::NaN <= Fix32::ONE == false);
	BOOST_TEST(Fix32::NaN == Fix32::ONE == false);
	BOOST_TEST(Fix32::NaN != Fix32::ONE == true);
}

BOOST_AUTO_TEST_CASE(fix32_to_real) {
	BOOST_TEST(Fix32(1).to_real<double>() == 1);
	BOOST_TEST(Fix32(1.5).to_real<double>() == 1.5);
	BOOST_TEST(Fix32(-1).to_real<double>() == -1);
	BOOST_TEST(Fix32(-1.5).to_real<double>() == -1.5);
	BOOST_TEST(Fix32(0).to_real<double>() == 0.0f);
	BOOST_TEST(Fix32::MAX.to_real<double>() == 2147483648.0 - Fix32::DELTA.to_real<double>());
	BOOST_TEST(Fix32::MAX_INT.to_real<double>() == 2147483647.0);
	BOOST_TEST(Fix32::MIN.to_real<double>() == -2147483648.0 + Fix32::DELTA.to_real<double>());
	BOOST_TEST(Fix32::MIN_INT.to_real<double>() == -2147483647.0);

	BOOST_TEST(Fix32::INF.to_real<double>() == std::numeric_limits<double>::infinity());
	BOOST_TEST((-Fix32::INF).to_real<double>() == -std::numeric_limits<double>::infinity());
	BOOST_TEST(Fix32::NaN.to_real<double>() != std::numeric_limits<double>::quiet_NaN());


	BOOST_TEST(Fix32(1).to_real<float>() == 1.0f);
	BOOST_TEST(Fix32(1.5).to_real<float>() == 1.5f);
	BOOST_TEST(Fix32(-1).to_real<float>() == -1.0f);
	BOOST_TEST(Fix32(-1.5).to_real<float>() == -1.5f);
	BOOST_TEST(Fix32(0).to_real<float>() == 0.0f);
	BOOST_TEST(Fix32::MAX.to_real<float>() == 2147483647.0f);
	BOOST_TEST(Fix32::MAX_INT.to_real<float>() == 2147483647.0f);
	BOOST_TEST(Fix32::MIN.to_real<float>() == -2147483647.0f);
	BOOST_TEST(Fix32::MIN_INT.to_real<float>() == -2147483647.0f);

	BOOST_TEST(Fix32::INF.to_real<float>() == std::numeric_limits<float>::infinity());
	BOOST_TEST((-Fix32::INF).to_real<float>() == -std::numeric_limits<float>::infinity());
	BOOST_TEST(Fix32::NaN.to_real<float>() != std::numeric_limits<float>::quiet_NaN());
}