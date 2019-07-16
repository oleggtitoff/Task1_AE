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

#define ALWAYS_INLINE static inline __attribute__((always_inline))

#define DIV_PRECISION 500

typedef struct {
	ae_f64 h;
	ae_f64 l;
}F64x2;

ALWAYS_INLINE ae_f32x2 int16ToF32x2(int16_t x, int16_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2((int32_t)x << 16, (int32_t)y << 16));
}

ALWAYS_INLINE ae_f32x2 int32ToF32x2(int32_t x, int32_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2(x, y));
}

ALWAYS_INLINE ae_f64 uint32ToF64(uint32_t x)
{
	return AE_MOVF64_FROMINT32X2(AE_MOVDA32X2(0, x));
}

ALWAYS_INLINE ae_f32x2 floatToF32x2(float h, float l)
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

ALWAYS_INLINE float Q31ToFloat_h(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_H(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

ALWAYS_INLINE float Q31ToFloat_l(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_L(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

ALWAYS_INLINE F64x2 putF64ToF64x2(ae_f64 h, ae_f64 l)
{
	F64x2 res;
	res.h = h;
	res.l = l;

	return res;
}

ALWAYS_INLINE ae_f64 LeftShiftA(ae_f64 x, uint8_t shift)
{
	return AE_SLAA64S(x, shift);
}

ALWAYS_INLINE ae_f64 RightShiftA(ae_f64 x, uint8_t shift)
{
	return AE_SRAA64(x, shift);
}

ALWAYS_INLINE ae_f32x2 RightShiftA_32x2(ae_f32x2 x, uint8_t shift)
{
	return AE_SRAA32S(x, shift);
}

ALWAYS_INLINE ae_f64 RightShiftL(ae_f64 x, uint8_t shift)
{
	return AE_SRLA64(x, shift);
}

ALWAYS_INLINE ae_f32x2 roundF64x2ToF32x2(F64x2 x)
{
	return AE_ROUND32X2F64SSYM(x.h, x.l);
}

ALWAYS_INLINE ae_f16x4 roundF64x2ToF16x4(F64x2 x)
{
	return AE_ROUND16X4F32SSYM(0, roundF64x2ToF32x2(x));
}

ALWAYS_INLINE ae_f32x2 Add(ae_f32x2 x, ae_f32x2 y)
{
	return AE_ADD32S(x, y);
}

ALWAYS_INLINE ae_f32x2 Sub(ae_f32x2 x, ae_f32x2 y)
{
	return AE_SUB32S(x, y);
}

ALWAYS_INLINE ae_f32x2 Mul(ae_f32x2 x, ae_f32x2 y)
{
	return RightShiftA_32x2(AE_MULFP32X2RS(x, y), 1);
}

ALWAYS_INLINE F64x2 Mul64(ae_f32x2 x, ae_f32x2 y)
{
	F64x2 res;
	res.h = AE_MULF32S_HH(x, y);
	res.l = AE_MULF32S_LL(x, y);
	return res;
}

ALWAYS_INLINE F64x2 Mac(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	AE_MULAF32S_LL(acc.l, x, y);
	AE_MULAF32S_HH(acc.h, x, y);
	return acc;
}

ALWAYS_INLINE F64x2 MSub(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	AE_MULSF32S_LL(acc.l, x, y);
	AE_MULSF32S_HH(acc.h, x, y);
	return acc;
}

//ALWAYS_INLINE
ae_f32x2 Div(ae_f32x2 x, ae_f32x2 y)
{
	ae_f32x2 precision = AE_MOVDA32X2(DIV_PRECISION, DIV_PRECISION);

	ae_f32x2 sign = AE_MOVDA32X2(1, 1);
	AE_MOVT32X2(sign, AE_NEG32(sign), AE_LT32(AE_XOR32(x, y), AE_ZERO32()));

	xtbool2 tmp1, tmp2, tmp3;
	xtbool2 y0 = AE_EQ32(y, AE_ZERO32());
	xtbool2 x0 = AE_EQ32(x, AE_ZERO32());
	ae_f32x2 low = AE_ZERO32();
	ae_f32x2 high = AE_MOVDA32X2(INT32_MAX, INT32_MAX);
	ae_f32x2 mid;
	ae_f32x2 midY;
	xtbool2 ifLess;
	xtbool2 ifBigger;

	xtbool2 isDone = xtbool_join_xtbool2(
								XT_ORB(xtbool2_extract_0(y0), xtbool2_extract_0(x0)),
								XT_ORB(xtbool2_extract_1(y0), xtbool2_extract_1(x0))
								);

	AE_MOVT32X2(mid, AE_MOVDA32X2(INT32_MAX, INT32_MAX), y0);
	AE_MOVT32X2(mid, AE_ZERO32(), x0);

	if ((int8_t)isDone == 3)
	{
		return mid;
	}

	x = AE_ABS32S(x);
	y = AE_ABS32S(y);

	while (1)
	{
		AE_MOVF32X2(mid, Add(low, RightShiftA_32x2(Sub(high, low), 1)), isDone);
		midY = Mul(mid, y);

		tmp1 = AE_LE32(AE_ABS32(Sub(midY, x)), precision);
		tmp2 = AE_EQ32(midY, AE_MOVDA32X2(INT32_MAX, INT32_MAX));
		tmp3 = AE_EQ32(midY, AE_MOVDA32X2(INT32_MIN, INT32_MIN));

		isDone = xtbool_join_xtbool2(
							XT_ORB(
									xtbool2_extract_0(isDone),
									XT_AND(
											XT_AND(
												xtbool2_extract_0(tmp1),
												XT_XOR(xtbool2_extract_0(tmp2), 1)
												),
											XT_XOR(xtbool2_extract_0(tmp3), 1)
											)
									),
							XT_ORB(
									xtbool2_extract_0(isDone),
									XT_AND(
											XT_AND(
												xtbool2_extract_1(tmp1),
												XT_XOR(xtbool2_extract_1(tmp2), 1)
												),
											XT_XOR(xtbool2_extract_1(tmp3), 1)
											)
									)
							);

		if ((int8_t)isDone == 3)
		{
			return Mul(mid, sign);
		}

		ifLess = AE_LT32(midY, x);
		ifBigger = AE_LE32(x, midY);

		AE_MOVT32X2(low, mid, ifLess);
		AE_MOVT32X2(high, mid, ifBigger);
	}
}

int main()
{
	float x1 = 0.5999994;
	float y1 = -1.002456;
	float x2 = 0.5999994;
	float y2 = -1.002456;

//	printf("x = %d\n", floatToFixed32(x1));
//	printf("y = %d\n", floatToFixed32(y1));
//	printf("x = %f\n", fixed32ToFloat(floatToFixed32(x1)));
//	printf("y = %f\n", fixed32ToFloat(floatToFixed32(y1)));

	ae_f32x2 z = floatToF32x2(x1, x2);
	ae_f32x2 c = floatToF32x2(y1, y2);

	//printf("Div fixed = %d\n", Div(z, c));
	printf("Div float = %f\n", Q31ToFloat_h(Div(z, c)));
	printf("Div float = %f\n", Q31ToFloat_l(Div(z, c)));

	return 0;
}
