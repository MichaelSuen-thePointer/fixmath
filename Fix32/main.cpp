#include "Fix32.hpp"
#include <iostream>

using std::cout;

int main()
{
	Fix32 r = -123456;
	r /=      100000;
	cout << r.to_float() << "\n";
	r /= -2;
	cout << r.to_float() << "\n";
	r *= -4;
	cout << r.to_float() << "\n";

	Fix32 half = -0.5f;
	Fix32 quater = -0.25f;

	r *= half;
	cout << r.to_float() << "\n";

	r /= quater;
	cout << r.to_float() << "\n";
}