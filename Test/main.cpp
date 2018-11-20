#define BOOST_TEST_MODULE Fix32

#include <boost/test/unit_test.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <tuple>
using std::pair;
using std::make_pair;
using namespace boost::unit_test;
using namespace boost::multiprecision;

BOOST_AUTO_TEST_CASE(helper_function_clzll)
{
	int clzll(uint64_t value);
	BOOST_TEST(clzll(0xFFFF'FFFF'FFFF'FFFF) == 0);
	BOOST_TEST(clzll(0x7FFF'FFFF'FFFF'FFFF) == 1);
	BOOST_TEST(clzll(0x1'FFFF'FFFF) == 31);
	BOOST_TEST(clzll(0xFFFF'FFFF) == 32);
	BOOST_TEST(clzll(0x7FFF'FFFF) == 33);
	BOOST_TEST(clzll(1) == 63);
	BOOST_TEST(clzll(0) == 64);
}

BOOST_AUTO_TEST_CASE(helper_function_negate)
{
	pair<uint64_t, uint64_t> negate(uint64_t low, int64_t high);
	auto make_int128_t = [](pair<uint64_t, uint64_t> v)
	{
		auto[lo, hi] = v;
		return (int128_t)(((uint128_t)hi << 64) | (uint128_t)lo);
	};
#define CHECK(lo, hi) BOOST_TEST(((uint128_t)-make_int128_t(make_pair((uint64_t)lo, (uint64_t)hi))) == (uint128_t)make_int128_t(negate(lo, hi)))
	CHECK(1, 0);
	CHECK(0, 0);
	CHECK(-1, -1);
	CHECK(-1, 0x7FFF'FFFF'FFFF'FFFF);
	CHECK(0x123456788765431, 0x876543212345678);
}