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

#define CHECK(a, b) BOOST_TEST(make_int128_t(int128_mul(a, b)) == (int128_t)a*(int128_t)b)

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
	CHECK(i64::max(), i64::min());
	CHECK(i64::min(), i64::max());

	BOOST_TEST(make_int128_t(int128_mul(i64::min(), i64::min())) == make_int128_t({ 0,0 })); // overflow test

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
}

#include "../Fix32/Fix32.hpp"

BOOST_AUTO_TEST_CASE(nan_case) {
	BOOST_TEST(Fix32::NOT_A_NUMBER != Fix32::NOT_A_NUMBER);
	BOOST_TEST(Fix32::NOT_A_NUMBER.is_nan());

	BOOST_TEST(!Fix32::POSITIVE_INFINITY.is_nan());
	BOOST_TEST(!Fix32::NEGATIVE_INFINITY.is_nan());
	BOOST_TEST(!Fix32(1).is_nan());
	BOOST_TEST(!Fix32(-1).is_nan());
	BOOST_TEST(!Fix32(0).is_nan());

	BOOST_TEST((Fix32::NOT_A_NUMBER + Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER - Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER * Fix32(1)).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER / Fix32(1)).is_nan());

	BOOST_TEST((Fix32(-1) + Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32(-1) - Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32(-1) * Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32(-1) / Fix32::NOT_A_NUMBER).is_nan());

	BOOST_TEST((Fix32::NOT_A_NUMBER + Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER - Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER * Fix32::NOT_A_NUMBER).is_nan());
	BOOST_TEST((Fix32::NOT_A_NUMBER / Fix32::NOT_A_NUMBER).is_nan());

	BOOST_TEST((Fix32(0) / Fix32(0)).is_nan());

	BOOST_TEST((Fix32::POSITIVE_INFINITY + Fix32::NEGATIVE_INFINITY).is_nan());
	BOOST_TEST((Fix32::NEGATIVE_INFINITY + Fix32::POSITIVE_INFINITY).is_nan());

	BOOST_TEST((Fix32::POSITIVE_INFINITY / Fix32::NEGATIVE_INFINITY).is_nan());
	BOOST_TEST((Fix32::POSITIVE_INFINITY / Fix32::POSITIVE_INFINITY).is_nan());
	BOOST_TEST((Fix32::NEGATIVE_INFINITY / Fix32::POSITIVE_INFINITY).is_nan());
	BOOST_TEST((Fix32::NEGATIVE_INFINITY / Fix32::NEGATIVE_INFINITY).is_nan());

	BOOST_TEST((Fix32::POSITIVE_INFINITY * Fix32(0)).is_nan());
	BOOST_TEST((Fix32::NEGATIVE_INFINITY * Fix32(0)).is_nan());
	BOOST_TEST((Fix32(0) * Fix32::POSITIVE_INFINITY).is_nan());
	BOOST_TEST((Fix32(0) * Fix32::NEGATIVE_INFINITY).is_nan());
}

BOOST_AUTO_TEST_CASE(normal_integer_add_sub) {
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
