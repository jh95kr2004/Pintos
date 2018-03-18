#ifndef __FIXED_POINT_H
#define __FIXED_POINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#define f 16384

int sum_f_f(int, int);
int sum_f_i(int, int);
int sum_i_f(int, int);
int sum_i_i(int, int);

int sub_f_f(int, int);
int sub_f_i(int, int);
int sub_i_f(int, int);
int sub_i_i(int, int);

int mul_f_f(int, int);
int mul_f_i(int, int);
int mul_i_f(int, int);
int mul_i_i(int, int);

int div_f_f(int, int);
int div_f_i(int, int);
int div_i_f(int, int);
int div_i_i(int, int);

int itof(int);
int ftoi(int, int);

#endif
