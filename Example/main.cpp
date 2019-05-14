#include "../Fix32/Fixmath.hpp"

int main()
{
	Fix32 last_num = 0;
	int last_count = 1;
	int scale = 1000;
	for (Fix32 a = 0; a <= Fix32::PI / 2; a += Fix32::DELTA * scale) {
		auto r = debug_sin(a);
		if (r.first != last_count) {
			for (Fix32 b = a - Fix32::DELTA * scale; b <= a; b += Fix32::DELTA) {
				auto r = debug_sin(b);
				if (r.first != last_count) {
					printf("%lld\t%d\n", last_num.to_raw(), last_count);
					printf("---\n");
					printf("%lld\t%d\n", b.to_raw(), r.first);
					scale *= 2;
				}
				last_num = b;
				last_count = r.first;
			}
		}
		last_num = a;
		last_count = r.first;
	}
	printf("%f", debug_sin(Fix32::PI / 2).second.to_real<double>());
}