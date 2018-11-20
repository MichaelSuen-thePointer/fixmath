#include <benchmark/benchmark.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <random>
#include <tuple>
#pragma comment(lib, "shlwapi.lib")
using std::pair;
pair<uint64_t, uint64_t> negate(uint64_t low, int64_t high)
{
	auto old_l = ~low;
	auto l = old_l + 1;
	auto h = ~(uint64_t)high;
	h += l < old_l;
	return { l, h };
}
pair<uint64_t, int64_t> int128_mul(int64_t _a, int64_t _b)
{
	int64_t a = abs(_a);
	int64_t b = abs(_b);
	using int_limits = std::numeric_limits<int32_t>;
	uint64_t rhi, rlo;
	if (a <= int_limits::max() && b <= int_limits::max())
	{
		rhi = 0;
		rlo = a * b;
	}
	else if ((a & 0xFFFFFFFFu) == 0 && (b & 0xFFFFFFFFu) == 0)
	{
		rhi = (a >> 32) * (b >> 32);
		rlo = 0;
	}
	else
	{
		uint64_t ahi = (uint64_t)a >> 32;
		uint64_t alo = (uint32_t)a;
		uint64_t bhi = (uint64_t)b >> 32;
		uint64_t blo = (uint32_t)b;

		auto alo_blo = alo * blo;
		auto alo_bhi = alo * bhi;
		auto ahi_blo = ahi * blo;
		auto ahi_bhi = ahi * bhi;

		auto ahi_blo_p_alo_bhi = ahi_blo;
		ahi_blo_p_alo_bhi += alo_bhi;
		auto carry1 = ahi_blo_p_alo_bhi < ahi_blo;

		rhi = ahi_bhi + ((uint64_t)carry1 << 32);

		rlo = alo_blo;
		rlo += ahi_blo_p_alo_bhi << 32;
		auto carry2 = rlo < alo_blo;

		rhi += carry2 + (ahi_blo_p_alo_bhi >> 32);
	}
	if (_a < 0 && _b < 0 || _a > 0 && _b > 0)
	{
		return { rlo, rhi };
	}
	return negate(rlo, rhi);
}
using namespace boost::multiprecision;

int128_t int128_mul2(int64_t a, int64_t b)
{
	return (int128_t)a * (int128_t)b;
}

std::mt19937_64 mtg{ std::random_device{}() };

void arguments_applier(benchmark::internal::Benchmark* b)
{
	using i32 = std::numeric_limits<int32_t>;
	using i64 = std::numeric_limits<int64_t>;
	b->Args({ 0, i32::max(), 0, i32::max() });
	b->Args({ i32::min(), 0, i32::min(), 0 });
	b->Args({ i32::min(), i32::max(), i32::min(), i32::max() });
	b->Args({ 0, i64::max(), 0, i64::max() });
	b->Args({ i64::min(), 0, i64::min(), 0 });
	b->Args({ i64::min(), i64::max(), i64::min(), i64::max() });
	b->Args({ i32::min(), i32::max(), i64::min(), i64::max() });
}

void fix32_int128_mul(benchmark::State& state)
{
	auto max = state.range(0);
	
	std::uniform_int_distribution<int64_t> uida{ state.range(0), state.range(1) };
	std::uniform_int_distribution<int64_t> uidb{ state.range(2), state.range(3) };
	for (auto _ : state)
	{
		state.PauseTiming();
		auto a = uida(mtg);
		auto b = uidb(mtg);
		state.ResumeTiming();
		benchmark::DoNotOptimize(int128_mul(a, b));
	}
}

BENCHMARK(fix32_int128_mul)->Apply(arguments_applier);

void boost_int128_mul(benchmark::State& state)
{
	std::uniform_int_distribution<int64_t> uida{ state.range(0), state.range(1) };
	std::uniform_int_distribution<int64_t> uidb{ state.range(2), state.range(3) };
	for (auto _ : state)
	{
		state.PauseTiming();
		auto a = uida(mtg);
		auto b = uidb(mtg);
		state.ResumeTiming();
		benchmark::DoNotOptimize(int128_mul2(a, b));
	}
}

BENCHMARK(boost_int128_mul)->Apply(arguments_applier);

BENCHMARK_MAIN();