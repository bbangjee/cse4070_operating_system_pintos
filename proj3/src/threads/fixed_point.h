#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

// p is integer & q is fraction
// 32-bit) 1 bit => sign | 17 bits => integer(p) | 14 bits => fraction(q)

#define F (1 << 14) // 2^14로 1.0 의미

// int to fp
#define INT_TO_FP(n) ((n) * F)

#define FP_TO_INT_ZERO(x) ((x) / F)
#define FP_TO_INT_NEAREST(x) ((x) >= 0 ? ((x) + F / 2) / F : ((x) - F / 2) / F)
// x와 y 둘 다 fp일 때 더하기
#define FP_ADD_FP(x, y) ((x) + (y))
// x와 y 둘 다 fp일 때 빼기
#define FP_SUB_FP(x, y) ((x) - (y))
// x와 y 둘 다 fp일 때 곱하기
#define FP_MUL_FP(x, y) (((int64_t)(x)) * (y) / F)
// x와 y 둘 다 fp일 때 나누기
#define FP_DIV_FP(x, y) (((int64_t)(x)) * F / (y))
// x는 fp, n은 int일 때 더하기
#define FP_ADD_INT(x, n) ((x) + F * (n))
// x는 fp, n은 int일 때 빼기
#define FP_SUB_INT(x, n) ((x) - F * (n))
// x는 fp, n은 int일 때 곱하기
#define FP_MUL_INT(x, n) ((x) * (n))
// x는 fp, n은 int일 때 나누기
#define FP_DIV_INT(x, n) ((x) / (n))

#endif