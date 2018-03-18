#include <fixed-point.h>
#include <ctype.h>
#include <inttypes.h>
#include <round.h>
#include <stdint.h>
#include <string.h>

int sum_f_f(int x, int y) {
	return x + y;
}

int sum_f_i(int x, int n) {
	return x + n * f;
}

int sum_i_f(int n, int x) {
	return n * f + x;
}

int sum_i_i(int a, int b) {
	return sum_f_f(itof(a), itof(b));
}

int sub_f_f(int x, int y) {
	return x - y;
}

int sub_f_i(int x, int n) {
	return x - n * f;
}

int sub_i_f(int n, int x) {
	return n * f - x;
}

int sub_i_i(int a, int b) {
	return sub_f_f(itof(a), itof(b));
}

int mul_f_f(int x, int y) {
	return ((int64_t)x) * y / f;
}

int mul_f_i(int x, int n) {
	return x * n;
}

int mul_i_f(int n, int x) {
	return n * x;
}

int mul_i_i(int a, int b) {
	return mul_f_f(itof(a), itof(b));
}

int div_f_f(int x, int y) {
	return ((int64_t)x) * f / y;
}

int div_f_i(int x, int n) {
	return x / n;
}

int div_i_f(int n, int x) {
	return div_f_f(itof(n), x);
}

int div_i_i(int a, int b) {
	return div_f_f(itof(a), itof(b));
}

int itof(int n) {
	return n * f;
}

int ftoi(int x, int mode) {
	if(mode == 0) return x / f;
	else if(mode == 1) {
		if(x < 0) return (x - f / 2) / f;
		return (x + f / 2) / f;
	}
}
