/*
 * main.c
 *
 *  Created on: Jul 11, 2019
 *      Author: Intern_2
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <xtensa/tie/xt_hifi2.h>

typedef struct {
	ae_f64 h;
	ae_f64 l;
}F64x2;

static inline ae_f32x2 int16ToF32x2(int16_t x, int16_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2((int32_t)x << 16, (int32_t)y << 16));
}

static inline ae_f32x2 int32ToF32x2(int32_t x, int32_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2(x, y));
}

static inline ae_f64 uint32ToF64(uint32_t x)
{
	return AE_MOVF64_FROMINT32X2(AE_MOVDA32X2(0, x));
}

static inline ae_f32x2 floatToF32x2(float h, float l)
{
	int32_t x;
	int32_t y;

	if (h >= 1)
	{
		x = INT32_MAX;
	}
	else if (h < -1)
	{
		x = INT32_MIN;
	}

	if (l >= 1)
	{
		y = INT32_MAX;
	}
	else if (h < -1)
	{
		y = INT32_MIN;
	}

	x = (int32_t)(h * (double)(1LL << 31));
	y = (int32_t)(l * (double)(1LL << 31));

	return int32ToF32x2(x, y);
}

static inline float Q31ToFloat_h(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_H(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

static inline float Q31ToFloat_l(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_L(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

static inline F64x2 putF64ToF64x2(ae_f64 h, ae_f64 l)
{
	F64x2 res;
	res.h = h;
	res.l = l;

	return res;
}

static inline ae_f32x2 roundF64x2ToF32x2(F64x2 x)
{
	return AE_ROUND32X2F64SSYM(x.h, x.l);
}

static inline ae_f16x4 roundF64x2ToF16x4(F64x2 x)
{
	return AE_ROUND16X4F32SSYM(0, roundF64x2ToF32x2(x));
}

static inline ae_f32x2 Add(ae_f32x2 x, ae_f32x2 y)
{
	return AE_ADD32S(x, y);
}

static inline ae_f32x2 Sub(ae_f32x2 x, ae_f32x2 y)
{
	return AE_SUB32S(x, y);
}

static inline F64x2 Mul(ae_f32x2 x, ae_f32x2 y)
{
	F64x2 res;
	res.h = AE_MULF32S_HH(x, y);
	res.l = AE_MULF32S_LL(x, y);

	return res;
}

static inline F64x2 Mac(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	F64x2 prod = Mul(x, y);
	acc.h = AE_ADD64S(acc.h, prod.h);
	acc.l = AE_ADD64S(acc.l, prod.l);

	return acc;
}

static inline F64x2 MSub(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	F64x2 prod = Mul(x, y);
	acc.h = AE_SUB64S(acc.h, prod.h);
	acc.l = AE_SUB64S(acc.l, prod.l);

	return acc;
}

int main()
{
	float x = 0.9;
	float y = 4;
	float z = 0.8;
	float k = 0;
	ae_f32x2 a = floatToF32x2(x, y);
	ae_f32x2 b = floatToF32x2(z, k);
	F64x2 acc;
//	acc.h = 435434596830345436LL;
//	acc.l = 9234589449396754LL;
//			putF64ToF64x2(
//			AE_MOVF64_FROMF32X2(floatToQ31(0, x)),
//			AE_MOVF64_FROMF32X2(floatToQ31(0, z))
//			);

	printf("x = %f\n", Q31ToFloat_h(a));
	printf("y = %f\n", Q31ToFloat_l(a));
	printf("z = %f\n", Q31ToFloat_h(b));
	printf("k = %f\n", Q31ToFloat_l(b));

	printf("Add1 = %f\n", Q31ToFloat_h(Add(a, b)));
	printf("Add2 = %f\n", Q31ToFloat_l(Add(a, b)));

	printf("Sub1 = %f\n", Q31ToFloat_h(Sub(a, b)));
	printf("Sub2 = %f\n", Q31ToFloat_l(Sub(a, b)));

	printf("Mul1 = %f\n", Q31ToFloat_h(roundF64x2ToF32x2(Mul(a, b))));
	printf("Mul2 = %f\n", Q31ToFloat_l(roundF64x2ToF32x2(Mul(a, b))));
	ae_f32x2 q = roundF64x2ToF32x2(Mul(a, b));
	ae_f16x4 w = roundF64x2ToF16x4(Mul(a, b));

//	printf("Mac1 = %f\n", Q31ToFloat_h(roundF64x2ToF32x2(Mac(acc, a, b))));
//	printf("Mac2 = %f\n", Q31ToFloat_l(roundF64x2ToF32x2(Mac(acc, a, b))));
//
//	printf("MSub1 = %f\n", Q31ToFloat_h(roundF64x2ToF32x2(MSub(acc, a, b))));
//	printf("MSub2 = %f\n", Q31ToFloat_l(roundF64x2ToF32x2(MSub(acc, a, b))));

	return 0;
}
