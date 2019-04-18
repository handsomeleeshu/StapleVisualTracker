/*
 * image.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_IMAGE_H_
#define _STP_IMAGE_H_

#include "stp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STP_INT32,    /* 32bit interger */
    STP_FLOAT32   /* single-precision float */
} STP_DataType;

typedef struct {
    stp_int32 width;
    stp_int32 height;

    /* quantization of image coefficients */
    stp_int32 q;

    STP_DataType data_type;
    void *p_mem;
} stp_image;

stp_ret STP_ImageInit(stp_image** p_image, stp_size size);
stp_ret STP_ImageDestroy(stp_image* image);

stp_ret STP_ImageSetDataType(stp_image *image, STP_DataType type);
stp_ret STP_ImageConvertDataType(stp_image *out_image, stp_image *in_image, STP_DataType dest_type);

stp_ret STP_ImageResize(stp_image *out_image, const stp_image *in_image, const stp_rect_q *rect);

stp_ret STP_ImageClear(stp_image *image);

stp_ret STP_ImageMaxPoint(stp_image *image, stp_pos *pos);
/* out = x.+y */
stp_ret STP_ImageDotAdd(stp_image *out, const stp_image *x, const stp_image *y);
/* out = x.+y */
stp_ret STP_ImageDotAddScalar(stp_image *out, const stp_image *x, stp_int32_q lambda);
/* out = x.-y */
stp_ret STP_ImageDotSub(stp_image *out, const stp_image *x, const stp_image *y);
/* out = x.*y */
stp_ret STP_ImageDotMulScalar(stp_image *out, const stp_image *x, stp_int32_q s);
/* out = x.*y */
stp_ret STP_ImageDotMul(stp_image *out, const stp_image *x, const stp_image *y);
/* out = x./(y + lambda) */
stp_ret STP_ImageDotDiv(stp_image *out, const stp_image *x, const stp_image *y, stp_int32_q lambda);
/* out = 1./x */
stp_ret STP_ImageDotRecip(stp_image *out, const stp_image *x, stp_int32_q lambda);
/* out = 1./x */
stp_ret STP_ImageDotSquareRootRecipe(stp_image *out, const stp_image *x);
/* out = max(x, y);*/
stp_ret STP_ImageDotDominance(stp_image *out, const stp_image *x, const stp_image *y);
/* x = min(max, x);*/
stp_ret STP_ImageDotClipCeil(stp_image *out, const stp_image *x, stp_int32_q max);
/* x = max(min, x);*/
stp_ret STP_ImageDotClipFloor(stp_image *out, const stp_image *x, stp_int32_q min);
/* out = x >> shift */
stp_ret STP_ImageDotShift(stp_image *out, const stp_image *x, stp_int32 shift);
/* x and y axis are gradients of image in */
stp_ret STP_ImageGradient(stp_image *x, stp_image *y, const stp_image *in);
/* filter image x with image y */
stp_ret STP_ImageFilter(stp_image *out, const stp_image *x, const stp_image *filter);
/* only this api support float data type image */
stp_ret STP_ImageCoefMix(stp_image *out_image, stp_image* in_image0, stp_image* in_image1, const float coef[2]);

typedef struct {
    stp_image *real, *imag, *temp; /* temp for middle results */
    /* + or - */
    stp_int32 sign;
} stp_complex_image;

stp_ret STP_ComplexImageInit(stp_complex_image** p_image, stp_size size);
stp_ret STP_ComplexImageDestroy(stp_complex_image* image);

stp_ret STP_ComplexImageResize(stp_complex_image *out_image,
        const stp_complex_image *in_image, const stp_rect_q *rect);

/* real +/- image */
stp_ret STP_ComplexImageConj(stp_complex_image *image);

stp_ret STP_ComplexImageClear(stp_complex_image *image);
/* out = x.+y */
stp_ret STP_ComplexImageDotAddC(stp_complex_image *out, const stp_complex_image *x,
        const stp_complex_image *y);
/* out = x.-y */
stp_ret STP_ComplexImageDotSubC(stp_complex_image *out, const stp_complex_image *x,
        const stp_complex_image *y);
/* out = x.*y */
stp_ret STP_ComplexImageDotMulC(stp_complex_image *out, const stp_complex_image *x,
        const stp_complex_image *y);
/* out = x.*y */
stp_ret STP_ComplexImageDotMulScalar(stp_complex_image *out, const stp_complex_image *x, stp_int32_q s);
/* out = x'.*x */
stp_ret STP_ComplexImageDotEnergy(stp_image *out, const stp_complex_image *x);
/* out = x./(y + lambda) */
stp_ret STP_ComplexImageDotDiv(stp_complex_image *out, const stp_complex_image *x,
        const stp_image *y, stp_int32_q lambda);
stp_ret STP_ComplexImageDotShift(stp_complex_image *out, const stp_complex_image *x, stp_int32 shift);

stp_ret STP_ComplexImageSetDataType(stp_complex_image *image, STP_DataType type);
stp_ret STP_ComplexImageConvertDataType(stp_complex_image *out_image, stp_complex_image *in_image, STP_DataType dest_type);
stp_ret STP_ComplexImageCoefMix(stp_complex_image *out_image,
        stp_complex_image *in_image0, stp_complex_image *in_image1,
        const float coef[2]);

typedef struct {
    stp_image *images[3];
} stp_multichannel_image;

#ifdef __cplusplus
}
#endif

#endif /* _STP_IMAGE_H_ */
