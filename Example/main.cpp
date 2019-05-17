#include "../Fix32/Fixmath.hpp"

int main()
{
	debug_sin(Fix32::from_raw(6676243429));
	printf("\n");
	sin(Fix32::from_raw(6676243429));
	Fix32 last_num = 0;
	int last_count = 1;
	Fix32 last_result = 0;
	int scale = 500;
	for (Fix32 a = Fix32::from_raw(13); a <= Fix32::PI / 2; a += Fix32::DELTA * scale) {
		auto r = debug_sin(a);
		if (r.second.to_raw() != sin(a).to_raw()) {
			printf("ERROR>>>> debug_sin(%lld)=%lld sin=%lld\n", a.to_raw(), r.second.to_raw(), sin(a).to_raw());
		}
	}
	printf("%lld", debug_sin(Fix32::PI / 2).second.to_raw());
}