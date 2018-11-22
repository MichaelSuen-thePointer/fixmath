#include <benchmark/benchmark.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <random>
#include <tuple>
#pragma comment(lib, "shlwapi.lib")
using std::pair;
using std::tuple;

pair<uint64_t, int64_t> int128_mul(int64_t _a, int64_t _b);

using namespace boost::multiprecision;
int128_t int128_mul2(int64_t a, int64_t b)
{
	return (int128_t)a * (int128_t)b;
}

std::mt19937_64 mtg{ std::random_device{}() };

void mul128_arguments_applier(benchmark::internal::Benchmark* b)
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

BENCHMARK(fix32_int128_mul)->Apply(mul128_arguments_applier);

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

BENCHMARK(boost_int128_mul)->Apply(mul128_arguments_applier);


tuple<uint64_t, int64_t, int64_t> int128_div_rem(uint64_t low, int64_t high, int64_t divisor);

pair<int128_t, int128_t> int128_div_rem2(int128_t a, int64_t b)
{
	return { a / b, a % b };
}

void div_rem128_arguments_applier(benchmark::internal::Benchmark* b)
{
	using i32 = std::numeric_limits<int32_t>;
	using i64 = std::numeric_limits<int64_t>;
	b->Args({ i64::min(), i64::max(), i64::min(), -1, 1, i64::max() }); //negative / positive
	b->Args({ i64::min(), i64::max(), i64::min(), -1, i64::min(), -1 }); //negative / negative
	b->Args({ i64::min(), i64::max(), 0, i64::max(), 1, i64::max() }); //positive / positive
	b->Args({ i64::min(), i64::max(), i64::min(), i64::max(), i64::min(), i64::max() }); //mix
	b->Args({ i64::min(), i64::max(), 0, 0, 1, i64::max() }); //64bit+ / 64bit+
	b->Args({ i64::min(), i64::max(), 0, 0, i64::min(), -1 }); //64bit+ / 64bit-
	b->Args({ i64::min(), i64::max(), -1, -1, 1, i64::max() }); //64bit- / 64bit+
	b->Args({ i64::min(), i64::max(), -1, -1, i64::min(), -1 }); //64bit- / 64bit-
	b->Args({ 0, 0, 0, i64::max(), 1, i64::max() }); //hi64bit+ / 64bit+
	b->Args({ 0, 0, 0, i64::max(), i64::min(), -1 }); //hi64bit+ / 64bit-
	b->Args({ 0, 0, i64::min(), -1, 1, i64::max() }); //hi64bit- / 64bit+
	b->Args({ 0, 0, i64::min(), -1, i64::min(), -1 }); //hi64bit- / 64bit-
}

int128_t make_int128_t(uint64_t lo, uint64_t hi)
{
	return (((int128_t)(int64_t)hi) << 64) | (int128_t)lo;
}

void fix32_int128_div_rem(benchmark::State& state)
{
	auto max = state.range(0);

	std::uniform_int_distribution<int64_t> uida{ state.range(0), state.range(1) };
	std::uniform_int_distribution<int64_t> uidb{ state.range(2), state.range(3) };
	std::uniform_int_distribution<int64_t> uidc{ state.range(4), state.range(5) };
	for (auto _ : state)
	{
		state.PauseTiming();
		auto a = uida(mtg);
		auto b = uidb(mtg);
		auto c = uidc(mtg);
		while (c == 0)
		{
			c = uidc(mtg);
		}
		state.ResumeTiming();
		benchmark::DoNotOptimize(int128_div_rem((uint64_t)a, b, c));
	}
}

BENCHMARK(fix32_int128_div_rem)->Apply(div_rem128_arguments_applier);

void boost_int128_div_rem(benchmark::State& state)
{
	auto max = state.range(0);

	std::uniform_int_distribution<int64_t> uida{ state.range(0), state.range(1) };
	std::uniform_int_distribution<int64_t> uidb{ state.range(2), state.range(3) };
	std::uniform_int_distribution<int64_t> uidc{ state.range(4), state.range(5) };
	for (auto _ : state)
	{
		state.PauseTiming();
		auto a = uida(mtg);
		auto b = uidb(mtg);
		auto c = uidc(mtg);
		while (c == 0)
		{
			c = uidc(mtg);
		}
		state.ResumeTiming();
		benchmark::DoNotOptimize(int128_div_rem2(make_int128_t((uint64_t)a, b), c));
	}
}

BENCHMARK(boost_int128_div_rem)->Apply(div_rem128_arguments_applier);

BENCHMARK_MAIN();