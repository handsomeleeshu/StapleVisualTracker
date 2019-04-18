/*
 * stp_image.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "stp_image.h"
#include "math.h"

stp_ret STP_ImageInit(stp_image** p_image, stp_size size) {
    stp_image *image;
    STP_Assert(size.h > 0 && size.w > 0 && p_image);
    image = (stp_image *)STP_Calloc(1, sizeof(stp_image));
    if (!image) {
        return STP_ERR_NOMEM;
    }
    image->p_mem = STP_Calloc(1, STP_ALIGN_ON4(size.w + 1)*STP_ALIGN_ON4(size.h + 1)*sizeof(stp_int32));
    if (image->p_mem) {
        image->height = size.h;
        image->width = size.w;
        image->q = 15;
        image->data_type = STP_INT32; /* default image data type */
        *p_image = image;
        return STP_OK;
    } else {
        STP_Free(image);
        return STP_ERR_NOMEM;
    }
}

stp_ret STP_ImageDestroy(stp_image* image) {
    if (image) {
        if (image->p_mem) {
            STP_Free(image->p_mem);
        }
        STP_Free(image);
    }
    return STP_OK;
}

stp_ret STP_ImageSetDataType(stp_image* image, STP_DataType type) {
    STP_Assert(image);
    image->data_type = type;
    return STP_OK;
}

#if !STP_ImageConvertDataType_Optimized
stp_ret STP_ImageConvertDataType(stp_image *out_image, stp_image *in_image, STP_DataType dest_type) {
    stp_int32 i;
    float q = 1L << 15;

    STP_Assert(in_image && out_image);

    if (in_image->data_type != dest_type) {
        if (dest_type == STP_INT32) {
            stp_int32 *o = (stp_int32*)out_image->p_mem;
            float *in = (float*)in_image->p_mem;

            for (i = 0; i < STP_ALIGN_ON4(in_image->width)*STP_ALIGN_ON4(in_image->height); i += 4) {
                o[i] = (stp_int32)(in[i]*q);
                o[i + 1] = (stp_int32)(in[i + 1]*q);
                o[i + 2] = (stp_int32)(in[i + 2]*q);
                o[i + 3] = (stp_int32)(in[i + 3]*q);
            }
            out_image->q = in_image->q + 15;
            out_image->data_type = STP_INT32;
        } else {
            float *o = (float*)out_image->p_mem;
            stp_int32 *in = (stp_int32*)in_image->p_mem;

            for (i = 0; i < STP_ALIGN_ON4(in_image->width)*STP_ALIGN_ON4(in_image->height); i += 4) {
                o[i] = (float)in[i]/q;
                o[i + 1] = (float)in[i + 1]/q;
                o[i + 2] = (float)in[i + 2]/q;
                o[i + 3] = (float)in[i + 3]/q;
            }
            out_image->data_type = STP_FLOAT32;
            out_image->q = in_image->q - 15;
        }
    } else {
        return STP_ERR_INVALID_PARAM;
    }
    return STP_OK;
}
#endif

/* only this api support float data type image */
#if !STP_ImageCoefMix_Optimized
stp_ret STP_ImageCoefMix(stp_image *out_image, stp_image *in_image0, stp_image *in_image1, const float coef[2]) {
    stp_int32 i;
    float *in0, *in1, *out;
    float coefs[2];

    STP_Assert(in_image0 && in_image1 && out_image
            && in_image0->data_type == STP_FLOAT32
            && in_image1->data_type == STP_FLOAT32);

    coefs[0] = coef[0];
    coefs[1] = coef[1];
    in0 = (float*)in_image0->p_mem;
    in1 = (float*)in_image1->p_mem;
    out = (float*)out_image->p_mem;
    coefs[1] *= pow(2, in_image0->q - in_image1->q);

    for (i = 0; i < STP_ALIGN_ON4(in_image0->width)*STP_ALIGN_ON4(in_image0->height); i += 4) {
        out[i] = in0[i]*coefs[0] + in1[i]*coefs[1];
        out[i + 1] = in0[i + 1]*coefs[0] + in1[i + 1]*coefs[1];
        out[i + 2] = in0[i + 2]*coefs[0] + in1[i + 2]*coefs[1];
        out[i + 3] = in0[i + 3]*coefs[0] + in1[i + 3]*coefs[1];
    }

    out_image->data_type = STP_FLOAT32;
    out_image->q = in_image0->q;
    return STP_OK;
}
#endif

#if !STP_ImageResize_optimized
stp_ret STP_ImageResize(stp_image *outImage, const stp_image *inImage, const stp_rect_q *rect_q) {
    stp_int32 i, j;
    stp_int32 h, w;
    stp_int32 h_in, w_in, h_out, w_out;
    stp_int32 out_x0, out_x1, out_y0, out_y1;
    stp_int32 x, y, stepx_q16, stepy_q16, startx_q16, starty_q16, curx_q16, cury_q16;
    stp_int32 coefx[STP_ALIGN_ON4(outImage->width)][2], coefy[STP_ALIGN_ON4(outImage->height)][2];
    stp_int32 *in, *out;
    register stp_int32 a, b, r;

    STP_Assert(outImage && inImage);

    h_in = inImage->height;
    w_in = inImage->width;
    h_out = outImage->height;
    w_out = outImage->width;

    if (rect_q) {
        stp_int32 in_x0, in_x1, in_y0, in_y1;

        x = (stp_int64)rect_q->pos.pos.x << 16 >> rect_q->pos.q; /* q16 */
        y = (stp_int64)rect_q->pos.pos.y << 16 >> rect_q->pos.q; /* q16 */
        h = (stp_int64)rect_q->size.size.h << 16 >> rect_q->size.q; /* q16 */
        w = (stp_int64)rect_q->size.size.w << 16 >> rect_q->size.q; /* q16 */
        startx_q16 = x - (w >> 1);
        starty_q16 = y - (h >> 1);

        if (startx_q16 < 0) {
            out_x0 = ((stp_int64)(-startx_q16)*w_out/w) + 1;
            startx_q16 = ((stp_int64)out_x0*w)/w_out + startx_q16;
        } else {
            out_x0 = 0;
        }

        in_x1 = x + (w >> 1);
        if (in_x1 > (w_in << 16)) {
            out_x1 = (stp_int64)w_out - ((stp_int64)in_x1 - ((stp_int64)w_in << 16))*w_out/w - 1;
        } else {
            out_x1 = w_out;
        }

        if (starty_q16 < 0) {
            out_y0 = ((stp_int64)(-starty_q16)*h_out/h) + 1;
            starty_q16 = ((stp_int64)out_y0*h)/h_out + starty_q16;
        } else {
            out_y0 = 0;
        }

        in_y1 = y + (h >> 1);
        if (in_y1 > (h_in << 16)) {
            out_y1 = (stp_int64)h_out - ((stp_int64)in_y1 - ((stp_int64)h_in << 16))*h_out/h - 1;
        } else {
            out_y1 = h_out;
        }

    } else {
        out_x0 = 0;
        out_x1 = w_out;
        out_y0 = 0;
        out_y1 = h_out;
        startx_q16 = 0;
        starty_q16 = 0;
        w = inImage->width << 16;
        h = inImage->height << 16;
    }

    in = (stp_int32*)inImage->p_mem;
    out = (stp_int32*)outImage->p_mem;
    stepx_q16 = w/w_out;
    stepy_q16 = h/h_out;

    curx_q16 = startx_q16;
    for (i = out_x0; i < out_x1; i++) {
        coefx[i][1] = (curx_q16 & 0xffff) << 15;
        coefx[i][0] = STP_FXP32(1.0f, 31) - coefx[i][1];
        curx_q16 += stepx_q16;
    }

    curx_q16 -= stepx_q16;
    while (curx_q16 > inImage->width << 16) {
        curx_q16 -= stepx_q16;
        coefx[--i][1] = 0;
    }

    cury_q16 = starty_q16;
    for (i = out_y0; i < out_y1; i++) {
        coefy[i][1] = (cury_q16 & 0xffff) << 15;
        coefy[i][0] = STP_FXP32(1.0f, 31) - coefy[i][1];
        cury_q16 += stepy_q16;
    }

    cury_q16 -= stepy_q16;
    while (cury_q16 > inImage->height << 16) {
        cury_q16 -= stepy_q16;
        coefy[--i][1] = 0;
    }

    r = 1L << 30;

    cury_q16 = starty_q16;
    for (i = out_y0; i < out_y1; i++) {
        y = cury_q16 >> 16;
        curx_q16 = startx_q16;
        for (j = out_x0; j < out_x1; j++) {
            x = curx_q16 >> 16;
            a = ((stp_int64)in[y*w_in + x]*coefy[i][0] + r) >> 31;
            b = ((stp_int64)in[y*w_in + x + 1]*coefy[i][0] + r) >> 31;
            a += ((stp_int64)in[(y + 1)*w_in + x]*coefy[i][1] + r) >> 31;
            b += ((stp_int64)in[(y + 1)*w_in + x + 1]*coefy[i][1] + r) >> 31;
            out[i*w_out + j] = ((stp_int64)a*coefx[j][0] + r) >> 31;
            out[i*w_out + j] += ((stp_int64)b*coefx[j][1] + r) >> 31;
            curx_q16 += stepx_q16;
        }
        cury_q16 += stepy_q16;
    }

    if ((out_x1 - out_x0)*(out_y1 - out_y0) < w_out*h_out) {
        for (i = out_y0; i < out_y1; i++) {
            for (j = out_x0; j > 0; j--) {
                out[i*w_out + j - 1] = out[i*w_out + j];
            }

            for (j = out_x1; j < w_out; j++) {
                out[i*w_out + j] = out[i*w_out + j - 1];
            }
        }

        for (i = out_y0; i > 0; i--) {
            for (j = 0; j < w_out; j++) {
                out[(i - 1)*w_out + j] = out[i*w_out + j];
            }
        }

        for (i = out_y1; i < h_out; i++) {
            for (j = 0; j < w_out; j++) {
                out[i*w_out + j] = out[(i - 1)*w_out + j];
            }
        }
    }

    outImage->q = inImage->q;

    return STP_OK;
}
#endif

stp_ret STP_ImageClear(stp_image *image) {
    STP_Assert(image);
    STP_Memclr(image->p_mem, 0, sizeof(stp_int32)*image->height*image->width);
    image->q = 15;
    return STP_OK;
}

#if !STP_ImageMaxPoint_Optimized
stp_ret STP_ImageMaxPoint(stp_image *image, stp_pos *pos) {
    stp_int32 i;
    stp_int32 max = 0x80000000L;
    stp_int32 *in;
    STP_Assert(image);

    in = (stp_int32*)image->p_mem;

    for (i = 0; i < image->width*image->height; i++) {
        max = STP_MAX(in[i], max);
    }

    for (i = 0; i < image->width*image->height; i++) {
        if (in[i] == max) {
            pos->x = i%image->width;
            pos->y = i/image->width;
            break;
        }
    }

    return STP_OK;
}
#endif

#if !STP_ImageDotAdd_Optimized
stp_ret STP_ImageDotAdd(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = in0[i] + in1[i];
        o[i + 1] = in0[i + 1] + in1[i + 1];
        o[i + 2] = in0[i + 2] + in1[i + 2];
        o[i + 3] = in0[i + 3] + in1[i + 3];
    }

    out->q = x->q;

    return STP_OK;
}
#endif

#if !STP_ImageDotAddScalar_Optimized
stp_ret STP_ImageDotAddScalar(stp_image *out, const stp_image *x, stp_int32_q lambda) {
    stp_int32 i, v;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    if (x->q >= lambda.q) {
        v = lambda.value << (x->q - lambda.q);
    } else {
        v = lambda.value >> (lambda.q - x->q);
    }

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = in[i] + v;
        o[i + 1] = in[i + 1] + v;
        o[i + 2] = in[i + 2] + v;
        o[i + 3] = in[i + 3] + v;
    }

    out->q = x->q;

    return STP_OK;
}
#endif

#if !STP_ImageDotSub_Optimized
stp_ret STP_ImageDotSub(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = in0[i] - in1[i];
        o[i + 1] = in0[i + 1] - in1[i + 1];
        o[i + 2] = in0[i + 2] - in1[i + 2];
        o[i + 3] = in0[i + 3] - in1[i + 3];
    }

    out->q = x->q;

    return STP_OK;
}
#endif


#if !STP_ImageDotMul_Optimized
stp_ret STP_ImageDotMul(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 q, r = 0;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    q = x->q + y->q - 15;
    if (q >= 1)
        r = 1L << (q - 1);

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = ((stp_int64)in0[i]*in1[i] + r) >> q;
        o[i + 1] = ((stp_int64)in0[i + 1]*in1[i + 1] + r) >> q;
        o[i + 2] = ((stp_int64)in0[i + 2]*in1[i + 2] + r) >> q;
        o[i + 3] = ((stp_int64)in0[i + 3]*in1[i + 3] + r) >> q;
    }

    out->q = 15;

    return STP_OK;
}
#endif

#if !STP_ImageDotMulScalar_Optimized
stp_ret STP_ImageDotMulScalar(stp_image *out, const stp_image *x, stp_int32_q s) {
    stp_int32 i, v, q, r;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    v = s.value;

    if (s.q < 31 && v < 1LL << s.q) {
        v = v << (31 - s.q);
        r = 1L << 30;
        for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
            o[i] = ((stp_int64)in[i]*v + r) >> 31;
            o[i + 1] = ((stp_int64)in[i + 1]*v + r) >> 31;
            o[i + 2] = ((stp_int64)in[i + 2]*v + r) >> 31;
            o[i + 3] = ((stp_int64)in[i + 3]*v + r) >> 31;
        }
        out->q = x->q;
    } else {
        q = x->q + s.q - 15;
        r = 1L << (q - 1);
        for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
            o[i] = ((stp_int64)in[i]*v + r) >> q;
            o[i + 1] = ((stp_int64)in[i + 1]*v + r) >> q;
            o[i + 2] = ((stp_int64)in[i + 2]*v + r) >> q;
            o[i + 3] = ((stp_int64)in[i + 3]*v + r) >> q;
        }
        out->q = 15;
    }

    return STP_OK;
}
#endif

stp_ret STP_ImageDotDiv(stp_image *out, const stp_image *x, const stp_image *y, stp_int32_q lambda) {
    STP_Assert(out && x && y && out != x);
    STP_ImageDotRecip(out, y, lambda);
    STP_ImageDotMul(out, out, x);
    return STP_OK;
}

#if !STP_ImageDotRecip_Optimized
stp_ret STP_ImageDotRecip(stp_image *out, const stp_image *x, stp_int32_q lambda) {
    stp_int32 i, v;
    stp_int32 *in, *o;
    float q;

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    if (x->q >= lambda.q) {
        v = lambda.value << (x->q - lambda.q);
    } else {
        v = lambda.value >> (lambda.q - x->q);
    }
    q = (1LL << (x->q + 15));

    for (i = 0; i < x->height*x->width; i += 4) {
        o[i] = 1.0f/(float)(in[i] + v)*q;
        o[i + 1] = 1.0f/(float)(in[i + 1] + v)*q;
        o[i + 2] = 1.0f/(float)(in[i + 2] + v)*q;
        o[i + 3] = 1.0f/(float)(in[i + 3] + v)*q;
    }

    out->q = 15;

    return STP_OK;
}
#endif

#if !STP_ImageDotSquareRootRecipe_Optimized
stp_ret STP_ImageDotSquareRootRecipe(stp_image *out, const stp_image *x) {
    stp_int32 i;
    stp_int32 *in, *o;
    float q;

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    q = pow(2, ((float)x->q)/2 + 15);

    for (i = 0; i < x->height*x->width; i += 4) {
        o[i] = 1.0f/sqrt((double)(in[i]))*q;
        o[i + 1] = 1.0f/sqrt((double)(in[i + 1]))*q;
        o[i + 2] = 1.0f/sqrt((double)(in[i + 2]))*q;
        o[i + 3] = 1.0f/sqrt((double)(in[i + 3]))*q;
    }

    out->q = 15;

    return STP_OK;
}
#endif

#if !STP_ImageDotDominance_Optimized
#define STP_DOMINANCE(a, b) ((STP_MAX(STP_ABS(a), STP_ABS(b)) == STP_ABS(a)) ? a : b)
stp_ret STP_ImageDotDominance(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = STP_DOMINANCE(in0[i], in1[i]);
        o[i + 1] = STP_DOMINANCE(in0[i + 1], in1[i + 1]);
        o[i + 2] = STP_DOMINANCE(in0[i + 2], in1[i + 2]);
        o[i + 3] = STP_DOMINANCE(in0[i + 3], in1[i + 3]);
    }

    out->q = x->q;

    return STP_OK;
}
#endif


#if !STP_ImageDotClipCeil_Optimized
stp_ret STP_ImageDotClipCeil(stp_image *out, const stp_image *x, stp_int32_q max) {
    stp_int32 i, ceil;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    if (x->q >= max.q) {
        ceil = max.value << (x->q - max.q);
    } else {
        ceil = max.value >> (max.q - x->q);
    }

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = STP_MIN(in[i], ceil);
        o[i + 1] = STP_MIN(in[i + 1], ceil);
        o[i + 2] = STP_MIN(in[i + 2], ceil);
        o[i + 3] = STP_MIN(in[i + 3], ceil);
    }

    out->q = x->q;

    return STP_OK;
}
#endif

#if !STP_ImageDotClipFloor_Optimized
stp_ret STP_ImageDotClipFloor(stp_image *out, const stp_image *x, stp_int32_q min) {
    stp_int32 i, floor;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    if (x->q >= min.q) {
        floor = min.value << (x->q - min.q);
    } else {
        floor = min.value >> (min.q - x->q);
    }

    for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
        o[i] = STP_MAX(in[i], floor);
        o[i + 1] = STP_MAX(in[i + 1], floor);
        o[i + 2] = STP_MAX(in[i + 2], floor);
        o[i + 3] = STP_MAX(in[i + 3], floor);
    }

    out->q = x->q;

    return STP_OK;
}
#endif

#if !STP_ImageDotShift_Optimized
stp_ret STP_ImageDotShift(stp_image *out, const stp_image *x, stp_int32 shift) {
    stp_int32 i;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;

    if (shift > 0) {
        for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
            o[i] = in[i] >> shift;
            o[i + 1] = in[i + 1] >> shift;
            o[i + 2] = in[i + 2] >> shift;
            o[i + 3] = in[i + 3] >> shift;
        }
    } else {
        shift = -shift;
        for (i = 0; i < STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height); i += 4) {
            o[i] = in[i] << shift;
            o[i + 1] = in[i + 1] << shift;
            o[i + 2] = in[i + 2] << shift;
            o[i + 3] = in[i + 3] << shift;
        }
    }

    out->q = x->q;

    return STP_OK;
}
#endif

#if !STP_ImageGradient_Optimized
stp_ret STP_ImageGradient(stp_image *x, stp_image *y, const stp_image *in) {
    stp_int32 i, j;
    stp_int32 *p, *in0, *in1;

    STP_Assert(in && x && y && !(in->width & 3) && !(in->height & 3));

    p = (stp_int32*)y->p_mem;
    in0 = (stp_int32*)in->p_mem;
    in1 = in0 + in->width;
    for (j = 0; j < in->width; j += 4) {
        p[j] = (in1[j] - in0[j]) << 1;
        p[j + 1] = (in1[j + 1] - in0[j + 1]) << 1;
        p[j + 2] = (in1[j + 2] - in0[j + 2]) << 1;
        p[j + 3] = (in1[j + 3] - in0[j + 3]) << 1;
    }

    p += in->width;
    in1 += in->width;
    for (i = 1; i < in->height - 1; i++) {
        for (j = 0; j < in->width; j += 4) {
            p[j] = in1[j] - in0[j];
            p[j + 1] = in1[j + 1] - in0[j + 1];
            p[j + 2] = in1[j + 2] - in0[j + 2];
            p[j + 3] = in1[j + 3] - in0[j + 3];
        }
        p += in->width; in1 += in->width; in0 += in->width;
    }

    p = (stp_int32*)y->p_mem + i*in->width;
    in1 = in0 + in->width;
    for (j = 0; j < in->width; j += 4) {
        p[j] = (in1[j] - in0[j]) << 1;
        p[j + 1] = (in1[j + 1] - in0[j + 1]) << 1;
        p[j + 2] = (in1[j + 2] - in0[j + 2]) << 1;
        p[j + 3] = (in1[j + 3] - in0[j + 3]) << 1;
    }

    p = (stp_int32*)x->p_mem;
    in0 = (stp_int32*)in->p_mem;
    for (j = 0; j < in->height; j++) {
        p[j*in->width] = (in0[j*in->width + 1] - in0[j*in->width]) << 1;
    }

    in1 = in0 + 2;
    p += 1;
    for (i = 0; i < in->height; i++) {
        for (j = 0; j < in->width - 4; j += 4) {
            p[j] = in1[j] - in0[j];
            p[j + 1] = in1[j + 1] - in0[j + 1];
            p[j + 2] = in1[j + 2] - in0[j + 2];
            p[j + 3] = in1[j + 3] - in0[j + 3];
        }
        p[j] = in1[j] - in0[j];
        p[j + 1] = in1[j + 1] - in0[j + 1];
        p += in->width; in1 += in->width; in0 += in->width;
    }

    p = (stp_int32*)x->p_mem + in->width - 1;
    in0 = (stp_int32*)in->p_mem + in->width - 2;
    for (j = 0; j < in->height; j++) {
        p[j*in->width] = (in0[j*in->width + 1] - in0[j*in->width]) << 1;
    }

    x->q = in->q;
    y->q = in->q;

    return STP_OK;
}
#endif

#if !STP_ImageFilter_Optimized
stp_ret STP_ImageFilter(stp_image *out, const stp_image *x, const stp_image *filter) {
    stp_int32 i, j, k, l, h, w, f_h, f_w, t_w;
    stp_int32 q, v;
    stp_int32 *in0, *in1, *o;
    stp_size size;
    stp_image *temp;
    STP_Assert(out && x && filter);

    size.h = x->height + filter->height;
    size.w = x->width + filter->width;

    if (out->width != size.w || out->height != size.h) {
        STP_ImageInit(&temp, size);
        o = (stp_int32*)temp->p_mem;
    } else {
        o = (stp_int32*)out->p_mem;
    }

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)filter->p_mem;
    h = x->height;
    w = x->width;
    f_h = filter->height;
    f_w = filter->width;
    t_w = size.w;
    q = x->q + filter->q - 15;

    for (k = 0; k < f_h; k++) {
        for (l = 0; l < f_w; l++) {
            v = in1[k*f_w + l];
            for (i = 0; i < h; i++) {
                for (j = 0; j < w; j++) {
                    o[(i+k)*t_w + (j+l)] += (stp_int64)in0[i*w + j]*v >> q;
                }
            }
        }
    }

    if (out->width != size.w || out->height != size.h) {
        in0 = (stp_int32 *) temp->p_mem;
        h = out->height;
        w = out->width;
        f_h = temp->height - h;
        f_w = temp->width - w;
        in0 += (f_h >> 1) * t_w + (f_w >> 1);
        o = (stp_int32 *) out->p_mem;
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                o[i * w + j] = in0[i * t_w + j];
            }
        }

        STP_ImageDestroy(temp);
    }

    out->q = 15;

    return STP_OK;
}
#endif

stp_ret STP_ComplexImageInit(stp_complex_image** p_image, stp_size size) {
    stp_ret ret = STP_OK;
    stp_complex_image *image;

    STP_Assert(size.h > 0 && size.w > 0 && p_image);

    image = (stp_complex_image *)STP_Calloc(1, sizeof(stp_complex_image));
    if (!image) {
        return STP_ERR_NOMEM;
    }

    ret = STP_ImageInit(&image->real, size);
    if (ret != STP_OK) {
        return ret;
    }
    ret = STP_ImageInit(&image->imag, size);
    if (ret != STP_OK) {
        return ret;
    }
    ret = STP_ImageInit(&image->temp, size);
    if (ret != STP_OK) {
        return ret;
    }

    image->sign = 1;

    *p_image = image;
    return STP_OK;
}

stp_ret STP_ComplexImageDestroy(stp_complex_image* image) {
    if (image) {
        if (image->real) {
            STP_ImageDestroy(image->real);
        }
        if (image->imag) {
            STP_ImageDestroy(image->imag);
        }
        if (image->temp) {
            STP_ImageDestroy(image->temp);
        }
        STP_Free(image);
    }
    return STP_OK;
}

stp_ret STP_ComplexImageResize(stp_complex_image *out_image,
        const stp_complex_image *in_image, const stp_rect_q *rect) {
    stp_ret ret = STP_OK;
    STP_Assert(out_image && in_image);
    ret = STP_ImageResize(out_image->real, in_image->real, rect);
    if (ret != STP_OK) {
        return ret;
    }
    ret = STP_ImageResize(out_image->imag, in_image->imag, rect);
    return ret;
}

stp_ret STP_ComplexImageConj(stp_complex_image *image) {
    STP_Assert(image);
    image->sign *= (-1);
    return STP_OK;
}

stp_ret STP_ComplexImageClear(stp_complex_image *image) {
    STP_Assert(image);
    image->sign = 1;
    STP_ImageClear(image->real);
    STP_ImageClear(image->imag);
    return STP_OK;
}

stp_ret STP_ComplexImageDotAddC(stp_complex_image *out,
        const stp_complex_image *x, const stp_complex_image *y) {
    stp_ret ret = STP_OK;
    STP_Assert(out && x && y);

    ret = STP_ImageDotAdd(out->real, x->real, y->real);
    if (ret != STP_OK) {
        return ret;
    }

    if (x->sign == y->sign) {
        ret = STP_ImageDotAdd(out->imag, x->imag, y->imag);
    } else {
        ret = STP_ImageDotSub(out->imag, x->imag, y->imag);
    }

    out->sign = x->sign;

    return ret;
}

stp_ret STP_ComplexImageDotSubC(stp_complex_image *out,
        const stp_complex_image *x, const stp_complex_image *y) {
    stp_ret ret = STP_OK;
    STP_Assert(out && x && y);

    ret = STP_ImageDotSub(out->real, x->real, y->real);
    if (ret != STP_OK) {
        return ret;
    }
    if (x->sign == y->sign) {
        ret = STP_ImageDotSub(out->imag, x->imag, y->imag);
    } else {
        ret = STP_ImageDotAdd(out->imag, x->imag, y->imag);
    }

    out->sign = x->sign;

    return ret;
}

#if !STP_ComplexImageDotMulC_Optimized
stp_ret STP_ComplexImageDotMulC(stp_complex_image *out,
        const stp_complex_image *x, const stp_complex_image *y) {
    STP_Assert(out && x && y && out != x && out != y);

    STP_ImageDotMul(out->real, x->real, y->real);
    STP_ImageDotMul(out->temp, x->imag, y->imag);
    if (x->sign == y->sign) {
        STP_ImageDotSub(out->real, out->real, out->temp);
    } else {
        STP_ImageDotAdd(out->real, out->real, out->temp);
    }

    STP_ImageDotMul(out->imag, x->real, y->imag);
    STP_ImageDotMul(out->temp, y->real, x->imag);
    if (x->sign == y->sign) {
        STP_ImageDotAdd(out->imag, out->imag, out->temp);
        out->sign = x->sign;
    } else {
        STP_ImageDotSub(out->imag, out->imag, out->temp);
        out->sign = -x->sign;
    }

    return STP_OK;
}
#endif

stp_ret STP_ComplexImageDotMulScalar(stp_complex_image *out, const stp_complex_image *x, stp_int32_q s) {
    stp_ret ret = STP_OK;
    STP_Assert(out && x);

    ret = STP_ImageDotMulScalar(out->real, x->real, s);
    if (ret != STP_OK) {
        return ret;
    }
    ret = STP_ImageDotMulScalar(out->imag, x->imag, s);

    out->sign = x->sign;

    return ret;
}

stp_ret STP_ComplexImageDotEnergy(stp_image *out, const stp_complex_image *x) {
    STP_Assert(out && x);

    STP_ImageDotMul(x->temp, x->imag, x->imag);
    STP_ImageDotMul(out, x->real, x->real);
    STP_ImageDotAdd(out, out, x->temp);

    return STP_OK;
}

stp_ret STP_ComplexImageDotShift(stp_complex_image *out, const stp_complex_image *x, stp_int32 shift) {
    STP_Assert(out && x);
    STP_ImageDotShift(out->real, x->real, shift);
    STP_ImageDotShift(out->imag, x->imag, shift);
    out->sign = x->sign;
    return STP_OK;
}

stp_ret STP_ComplexImageDotDiv(stp_complex_image *out,
        const stp_complex_image *x, const stp_image *y, stp_int32_q lambda) {
    STP_Assert(out && x && y);

    STP_ImageDotRecip(out->temp, y, lambda);
    STP_ImageDotMul(out->real, x->real, out->temp);
    STP_ImageDotMul(out->imag, x->imag, out->temp);

    out->sign = x->sign;

    return STP_OK;
}

stp_ret STP_ComplexImageSetDataType(stp_complex_image* image, STP_DataType type) {
    STP_ImageSetDataType(image->real, type);
    STP_ImageSetDataType(image->imag, type);
    return STP_OK;
}

stp_ret STP_ComplexImageConvertDataType(stp_complex_image* out_image, stp_complex_image* in_image, STP_DataType dest_type) {
    STP_Assert(out_image && in_image);
    STP_ImageConvertDataType(out_image->real, in_image->real, dest_type);
    STP_ImageConvertDataType(out_image->imag, in_image->imag, dest_type);
    out_image->sign = in_image->sign;
    return STP_OK;
}

stp_ret STP_ComplexImageCoefMix(stp_complex_image *out_image,
        stp_complex_image* in_image0, stp_complex_image* in_image1,
        const float coef[2]) {
    float coefs[2];
    coefs[0] = coef[0];
    coefs[1] = coef[1];
    STP_Assert(out_image && in_image0 && in_image1);
    STP_ImageCoefMix(out_image->real, in_image0->real, in_image1->real, coefs);
    coefs[0] = coef[0];
    coefs[1] = coef[1];
    if (in_image1->sign != in_image0->sign) {
        coefs[1] *= -1;
    }
    STP_ImageCoefMix(out_image->imag, in_image0->imag, in_image1->imag, coefs);
    out_image->sign = in_image0->sign;
    return STP_OK;
}


