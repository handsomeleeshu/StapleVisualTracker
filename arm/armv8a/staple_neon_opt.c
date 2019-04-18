/*
 * staple_neon_opt.c

 *
 *  Created on: 2018-10-13
 *      Author: handsomelee
 */

#include "stp_image.h"
#include "staple.h"
#include "math.h"

#if STP_ImageDotAdd_Optimized
stp_ret STP_ImageDotAdd(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    i = x->width*x->height;

    asm volatile (
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    out->q = x->q;

    return STP_OK;
}
#endif

#if STP_ImageDotSub_Optimized
stp_ret STP_ImageDotSub(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    i = x->width*x->height;

    asm volatile (
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    out->q = x->q;

    return STP_OK;
}
#endif


#if STP_ImageDotMul_Optimized
stp_ret STP_ImageDotMul(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int64 q;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    i = x->width*x->height;
    q = 15 - x->q - y->q;

    asm volatile (
        "dup v4.2d, %[q] \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "smull v2.2d, v0.2s, v1.2s \n"
        "smull2 v3.2d, v0.4s, v1.4s \n"
        "srshl v0.2d, v2.2d, v4.2d \n"
        "srshl v1.2d, v3.2d, v4.2d \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.s}[0], [%[o]], #4 \n"
        "st1 {v0.s}[2], [%[o]], #4 \n"
        "st1 {v1.s}[0], [%[o]], #4 \n"
        "st1 {v1.s}[2], [%[o]], #4 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i), [q] "r" (q)
        : "memory", "v0", "v1", "v2", "v3", "v4"
    );

    out->q = 15;

    return STP_OK;
}
#endif

#if STP_ImageDotMulScalar_Optimized
stp_ret STP_ImageDotMulScalar(stp_image *out, const stp_image *x, stp_int32_q s) {
    stp_int32 i, v, q;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    v = s.value;
    i = x->width*x->height;

    if (s.q < 31 && v < (1LL << s.q)) {
        v = v << (31 - s.q);
        asm volatile (
            "dup v1.2d, %[v] \n"
            "trn1 v1.4s, v1.4s, v1.4s \n"
            "1: \n"
            "ld1 {v0.4s}, [%[in]], #16 \n"
            "sqrdmulh v0.4s, v0.4s, v1.4s \n"
            "subs %[i], %[i], #4 \n"
            "st1 {v0.4s}, [%[o]], #16 \n"
            "bgt 1b \n"
            :
            : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
            : "memory", "v0", "v1"
        );
        out->q = x->q;
    } else {
        q = 15 - x->q - s.q;
        asm volatile (
            "dup v3.2d, %[v] \n"
            "dup v4.2d, %[q] \n"
            "trn1 v3.4s, v3.4s, v3.4s \n"
            "1: \n"
            "ld1 {v0.4s}, [%[in]], #16 \n"
            "smull v0.2d, v0.2s, v3.2s \n"
            "smull2 v1.2d, v0.4s, v3.4s \n"
            "srshl v0.2d, v0.2d, v4.2d \n"
            "srshl v1.2d, v1.2d, v4.2d \n"
            "subs %[i], %[i], #4 \n"
            "st1 {v0.s}[0], [%[o]], #4 \n"
            "st1 {v0.s}[2], [%[o]], #4 \n"
            "st1 {v1.s}[0], [%[o]], #4 \n"
            "st1 {v1.s}[2], [%[o]], #4 \n"
            "bgt 1b \n"
            :
            : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i), [q] "r" (q)
            : "memory", "v0", "v1", "v2", "v3", "v4"
        );
        out->q = 15;
    }

    return STP_OK;
}
#endif

#if STP_ImageMaxPoint_Optimized
stp_ret STP_ImageMaxPoint(stp_image *image, stp_pos *pos) {
    volatile stp_int32 i;
    volatile stp_int32 neg_inf = 0x80000000L;
    volatile stp_int32 max = 0x80000000L;
    volatile stp_int32 *in;

    STP_Assert(image);

    in = (stp_int32*)image->p_mem;
    i = image->width*image->height;

    in[i] = 0x80000000;
    in[i + 1] = 0x80000000;
    in[i + 2] = 0x80000000;
    in[i + 3] = 0x80000000;

    asm volatile (
        "dup v1.2d, %[neg_inf] \n"
        "trn1 v1.4s, v1.4s, v1.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "smax v1.4s, v1.4s, v0.4s \n"
        "subs %[i], %[i], #4 \n"
        "bgt 1b \n"
        "smaxp v1.4s, v1.4s, v1.4s \n"
        "smaxp v1.4s, v1.4s, v1.4s \n"
        "smov %[max], v1.s[0] \n"
        : [max] "+r" (max)
        : [in] "r" (in), [i] "r" (i), [neg_inf] "r" (neg_inf)
        : "v0", "v1"
    );

    in = (stp_int32*)image->p_mem;
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

#if STP_ImageDotRecip_Optimized
stp_ret STP_ImageDotRecip(stp_image *out, const stp_image *x, stp_int32_q lambda) {
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

    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "dup v2.2d, %[v] \n"
        "trn1 v2.4s, v2.4s, v2.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "ld1 {v1.4s}, [%[in]], #16 \n"
        "add v0.4s, v0.4s, v2.4s \n"
        "add v1.4s, v1.4s, v2.4s \n"
        "scvtf v0.4s, v0.4s, #15 \n"
        "scvtf v1.4s, v1.4s, #15 \n"
        "frecpe v0.4s, v0.4s \n"
        "frecpe v1.4s, v1.4s \n"
        "fcvtzs v0.4s, v0.4s, #15 \n"
        "fcvtzs v1.4s, v1.4s, #15 \n"
        "subs %[i], %[i], #8 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "st1 {v1.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
        : "memory", "v0", "v1", "v2"
    );

    out->q = 30 - x->q;

    return STP_OK;
}
#endif

#if STP_ImageResize_optimized
stp_ret STP_ImageResize(stp_image *outImage, const stp_image *inImage, const stp_rect_q *rect_q) {
    stp_int32 i, j;
    stp_int32 h, w;
    stp_int32 h_in, w_in, h_out, w_out;
    stp_int32 out_x0, out_x1, out_y0, out_y1;
    stp_int32 x, y, stepx_q16, stepy_q16, startx_q16, starty_q16, curx_q16, cury_q16;
    stp_int32 coefx[2][STP_ALIGN_ON4(outImage->width)], coefy[2][STP_ALIGN_ON4(outImage->height)];
    stp_int32 *in, *out, *p_coefx[2], *p_coefy[2];

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
        coefx[1][i] = (curx_q16 & 0xffff) << 15;
        coefx[0][i] = STP_FXP32(1.0f, 31) - coefx[1][i];
        curx_q16 += stepx_q16;
    }

    curx_q16 -= stepx_q16;
    while (curx_q16 > inImage->width << 16) {
        curx_q16 -= stepx_q16;
        coefx[1][--i] = 0;
    }

    cury_q16 = starty_q16;
    for (i = out_y0; i < out_y1; i++) {
        coefy[1][i] = (cury_q16 & 0xffff) << 15;
        coefy[0][i] = STP_FXP32(1.0f, 31) - coefy[1][i];
        cury_q16 += stepy_q16;
    }

    cury_q16 -= stepy_q16;
    while (cury_q16 > inImage->height << 16) {
        cury_q16 -= stepy_q16;
        coefy[1][--i] = 0;
    }

    p_coefx[0] = coefx[0];
    p_coefx[1] = coefx[1];
    p_coefy[0] = coefy[0];
    p_coefy[1] = coefy[1];

    stp_uint64 params[12];
    params[0] = (stp_uint64)stepx_q16;
    params[1] = (stp_uint64)stepy_q16;
    params[2] = (stp_uint64)(out_x1 - out_x0);
    params[3] = (stp_uint64)(out_y1 - out_y0);
    params[4] = (stp_uint64)w_in*4;
    params[5] = (stp_uint64)p_coefx;
    params[6] = (stp_uint64)startx_q16;
    params[7] = (stp_uint64)in;
    params[8] = (stp_uint64)(out + out_y0*w_out + out_x0);
    params[9] = (stp_uint64)p_coefy;
    params[10] = (stp_uint64)starty_q16;
    params[11] = (stp_uint64)((w_out + out_x0 - out_x1) << 2);

    asm volatile (
        "stp x0, x1, [sp, #-16]! \n"
        "stp x2, x3, [sp, #-16]! \n"
        "stp x4, x5, [sp, #-16]! \n"
        "stp x6, x7, [sp, #-16]! \n"
        "stp x8, x9, [sp, #-16]! \n"
        "stp x10, x11, [sp, #-16]! \n"
        "stp x12, x13, [sp, #-16]! \n"
        "mov x12, %[params] \n"
        "ldr x0, [x12] \n"
        "ldr x1, [x12, #8] \n"
        "ldr x2, [x12, #16] \n"
        "ldr x3, [x12, #24] \n"
        "ldr x4, [x12, #32] \n"
        "ldr x5, [x12, #40] \n"
        "ldr x6, [x12, #48] \n"
        "ldr x7, [x12, #56] \n"
        "ldr x8, [x12, #64] \n"
        "ldr x9, [x12, #72] \n"
        "ldr x10, [x12, #80] \n"
        "ldr x11, [x12, #88] \n"

        "dup v9.4s, w0 \n"
        "dup v11.4s, w1 \n"
        "dup v10.4s, w10 \n"
        "mov v8.s[0], w6 \n"
        "add w6, w6, w0 \n"
        "mov v8.s[1], w6 \n"
        "add w6, w6, w0 \n"
        "mov v8.s[2], w6 \n"
        "add w6, w6, w0 \n"
        "mov v8.s[3], w6 \n"
        "shl v9.4s, v9.4s, #2 \n"
        "ldr x6, [x5, #8] \n"
        "ldr x5, [x5] \n"
        "ldr x10, [x9, #8] \n"
        "ldr x9, [x9] \n"

        "1: \n"
        "ldr w0, [x9], #4 \n"
        "ldr w1, [x10], #4 \n"
        "dup v6.4s, w0 \n"
        "dup v7.4s, w1 \n"
        "str q8, [sp, #-16]! \n"
        "str x2, [sp, #-16]! \n"
        "str x5, [sp, #-16]! \n"
        "str x6, [sp, #-16]! \n"

        "2: \n"
        "dup v3.4s, w4 \n"
        "ushr v1.4s, v10.4s, #16 \n"
        "ushr v0.4s, v8.4s, #16 \n"
        "mul v1.4s, v1.4s, v3.4s \n"
        "shl v0.4s, v0.4s, #2 \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "smov x0, v0.s[2] \n"
        "smov x1, v0.s[3] \n"
        "add x0, x0, x7 \n"
        "add x1, x1, x7 \n"
        "ld1 {v0.d}[1], [x0] \n"
        "ld1 {v1.d}[1], [x1] \n"
        "add x0, x0, x4 \n"
        "add x1, x1, x4 \n"
        "ld1 {v2.d}[1], [x0] \n"
        "ld1 {v3.d}[1], [x1] \n"

        "ld1 {v4.4s}, [x5], #16 \n"
        "ld1 {v5.4s}, [x6], #16 \n"

        "smov x0, v0.s[0] \n"
        "smov x1, v0.s[1] \n"
        "add x0, x0, x7 \n"
        "add x1, x1, x7 \n"
        "ld1 {v0.d}[0], [x0] \n"
        "ld1 {v1.d}[0], [x1] \n"
        "add x0, x0, x4 \n"
        "add x1, x1, x4 \n"
        "ld1 {v2.d}[0], [x0] \n"
        "ld1 {v3.d}[0], [x1] \n"
        "trn2 v12.4s, v0.4s, v1.4s \n"
        "trn2 v13.4s, v2.4s, v3.4s \n"
        "trn1 v0.4s, v0.4s, v1.4s \n"
        "trn1 v2.4s, v2.4s, v3.4s \n"

        "sqrdmulh v0.4s, v0.4s, v6.4s \n"
        "sqrdmulh v2.4s, v2.4s, v7.4s \n"
        "sqrdmulh v1.4s, v12.4s, v6.4s \n"
        "sqrdmulh v3.4s, v13.4s, v7.4s \n"
        "add v0.4s, v0.4s, v2.4s \n"
        "add v1.4s, v1.4s, v3.4s \n"
        "sqrdmulh v0.4s, v0.4s, v4.4s \n"
        "sqrdmulh v1.4s, v1.4s, v5.4s \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "add v8.4s, v8.4s, v9.4s \n"
        "subs x2, x2, #4 \n"
        "st1 {v0.4s}, [x8], #16 \n"
        "bgt 2b \n"

        "add v10.4s, v10.4s, v11.4s \n"
        "lsl x2, x2, #2 \n"
        "add x8, x8, x2 \n"
        "subs x3, x3, #1 \n"
        "add x8, x8, x11 \n"
        "ldr x6, [sp], #16 \n"
        "ldr x5, [sp], #16 \n"
        "ldr x2, [sp], #16 \n"
        "ld1 {v8.4s}, [sp], #16 \n"
        "bgt 1b \n"

        "ldp x12, x13, [sp], #16 \n"
        "ldp x10, x11, [sp], #16 \n"
        "ldp x8, x9, [sp], #16 \n"
        "ldp x6, x7, [sp], #16 \n"
        "ldp x4, x5, [sp], #16 \n"
        "ldp x2, x3, [sp], #16 \n"
        "ldp x0, x1, [sp], #16 \n"
        :
        : [params] "r" (params)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13"
    );

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

#if STP_ImageGradient_Optimized
stp_ret STP_ImageGradient(stp_image *x, stp_image *y, const stp_image *in) {
    stp_int32 i, j;
    stp_int32 *p, *in0, *in1;

    STP_Assert(in && x && y && !(in->width & 3) && !(in->height & 3));

    p = (stp_int32*)y->p_mem;
    in0 = (stp_int32*)in->p_mem;
    in1 = in0 + in->width;
    i = in->width;
    asm volatile (
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.4s, v1.4s, v0.4s \n"
        "shl v0.4s, v0.4s, #1 \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[p]], #16 \n"
        "bgt 1b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    p += in->width;
    in1 += in->width;
    i = (in->height - 2)*in->width;
    asm volatile (
        "2: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.4s, v1.4s, v0.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[p]], #16 \n"
        "bgt 2b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    p = (stp_int32*)y->p_mem + (in->height - 1)*in->width;
    in0 = (stp_int32*)in->p_mem + (in->height - 2)*in->width;
    in1 = in0 + in->width;
    i = in->width;
    asm volatile (
        "3: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.4s, v1.4s, v0.4s \n"
        "shl v0.4s, v0.4s, #1 \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[p]], #16 \n"
        "bgt 3b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    p = (stp_int32*)x->p_mem;
    in0 = (stp_int32*)in->p_mem;
    for (j = 0; j < in->height; j++) {
        p[j*in->width] = (in0[j*in->width + 1] - in0[j*in->width]) << 1;
    }

    in1 = in0 + 2;
    p += 1;
    i = in->height;
    j = in->width - 4;
    asm volatile (
        "4: \n"
        "str %[j], [sp, #-16]! \n"
        "5: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.4s, v1.4s, v0.4s \n"
        "subs %[j], %[j], #4 \n"
        "st1 {v0.4s}, [%[p]], #16 \n"
        "bgt 5b \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "sub v0.2s, v1.2s, v0.2s \n"
        "st1 {v0.d}[0], [%[p]], #8 \n"
        "ld1 {v0.d}[0], [%[p]], #8 \n"
        "subs %[i], %[i], #1 \n"
        "ldr %[j], [sp], #16 \n"
        "bgt 4b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i), [j] "r" (j)
        : "memory", "v0", "v1"
    );

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

#if STP_ImageDotClipCeil_Optimized
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
    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "dup v1.2d, %[ceil] \n"
        "trn1 v1.4s, v1.4s, v1.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "smin v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [ceil] "r" (ceil), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    out->q = x->q;

    return STP_OK;
}
#endif

#if STP_ImageDotClipFloor_Optimized
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
    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "dup v1.2d, %[floor] \n"
        "trn1 v1.4s, v1.4s, v1.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "smax v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [floor] "r" (floor), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    out->q = x->q;

    return STP_OK;
}
#endif

#if STP_ImageDotShift_Optimized
stp_ret STP_ImageDotShift(stp_image *out, const stp_image *x, stp_int32 shift) {
    stp_int32 i;
    stp_int32 *in, *o;
    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;
    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "dup v1.2d, %[shift] \n"
        "trn1 v1.4s, v1.4s, v1.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "srshl v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [shift] "r" (-shift), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    return STP_OK;
}
#endif

#if STP_ImageDotAddScalar_Optimized
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
    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "dup v1.2d, %[v] \n"
        "trn1 v1.4s, v1.4s, v1.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
        : "memory", "v0", "v1"
    );

    return STP_OK;
}
#endif

#if STP_ImageDotSquareRootRecipe_Optimized
stp_ret STP_ImageDotSquareRootRecipe(stp_image *out, const stp_image *x) {
    stp_int32 i, q;
    stp_int32 *in, *o;

    STP_Assert(out && x);

    in = (stp_int32*)x->p_mem;
    o = (stp_int32*)out->p_mem;

    i = STP_ALIGN_ON4(x->width)*STP_ALIGN_ON4(x->height);

    asm volatile (
        "1: \n"
        "ld1 {v0.4s}, [%[in]], #16 \n"
        "ld1 {v1.4s}, [%[in]], #16 \n"
        "scvtf v0.4s, v0.4s, #15 \n"
        "scvtf v1.4s, v1.4s, #15 \n"
        "ld1 {v2.4s}, [%[in]], #16 \n"
        "ld1 {v3.4s}, [%[in]], #16 \n"
        "frsqrte v0.4s, v0.4s \n"
        "frsqrte v1.4s, v1.4s \n"
        "scvtf v2.4s, v2.4s, #15 \n"
        "scvtf v3.4s, v3.4s, #15 \n"
        "frsqrte v2.4s, v2.4s \n"
        "frsqrte v3.4s, v3.4s \n"
        "fcvtzs v0.4s, v0.4s, #15 \n"
        "fcvtzs v1.4s, v1.4s, #15 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "st1 {v1.4s}, [%[o]], #16 \n"
        "fcvtzs v2.4s, v2.4s, #15 \n"
        "fcvtzs v3.4s, v3.4s, #15 \n"
        "subs %[i], %[i], #16 \n"
        "st1 {v2.4s}, [%[o]], #16 \n"
        "st1 {v3.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [i] "r" (i)
        : "memory", "v0", "v1", "v2", "v3"
    );

    out->q = 30 - x->q;

    return STP_OK;
}
#endif

#if STP_ImageDotDominance_Optimized
stp_ret STP_ImageDotDominance(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y && x->q == y->q);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    i = x->width*x->height;

    asm volatile (
        "movi v4.4s, #1 \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "abs v2.4s, v0.4s \n"
        "abs v3.4s, v1.4s \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "smax v2.4s, v2.4s, v3.4s \n"
        "cmlt v0.4s, v0.4s, #0 \n"
        "add v0.4s, v0.4s, v0.4s \n"
        "add v0.4s, v0.4s, v4.4s \n"
        "mul v0.4s, v0.4s, v2.4s \n"
        "subs %[i], %[i], #4 \n"
        "st1 {v0.4s}, [%[o]], #16 \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "v0", "v1", "v2", "v3", "v4"
    );

    out->q = x->q;

    return STP_OK;
}
#endif


#if STP_ComplexImageDotMulC_Optimized
stp_ret STP_ComplexImageDotMulC(stp_complex_image *out,
        const stp_complex_image *x, const stp_complex_image *y) {
    stp_int32 i;
    stp_int64 q;
    stp_int32 *in0, *in1, *in2, *in3, *o0, *o1;
    STP_Assert(out && x && y);

    in0 = (stp_int32*)x->real->p_mem;
    in1 = (stp_int32*)x->imag->p_mem;
    in2 = (stp_int32*)y->real->p_mem;
    in3 = (stp_int32*)y->imag->p_mem;
    o0 = (stp_int32*)out->real->p_mem;
    o1 = (stp_int32*)out->imag->p_mem;

    i = x->real->width*x->real->height;
    q = 15 - x->real->q - y->real->q;

    if (x->sign == y->sign) {
        asm volatile (
            "dup v12.2d, %[q] \n"

            "1: \n"
            "ld2 {v0.4s, v1.4s}, [%[in0]], #32 \n"
            "ld2 {v2.4s, v3.4s}, [%[in2]], #32 \n"
            "smull v8.2d, v0.2s, v2.2s \n"
            "smull v9.2d, v1.2s, v3.2s \n"
            "ld2 {v6.4s, v7.4s}, [%[in3]], #32 \n"
            "ld2 {v4.4s, v5.4s}, [%[in1]], #32 \n"
            "smull v10.2d, v0.2s, v6.2s \n"
            "smull v11.2d, v1.2s, v7.2s \n"
            "smlsl v8.2d, v4.2s, v6.2s \n"
            "smlsl v9.2d, v5.2s, v7.2s \n"
            "smlal v10.2d, v2.2s, v4.2s \n"
            "smlal v11.2d, v3.2s, v5.2s \n"
            "srshl v8.2d, v8.2d, v12.2d \n"
            "srshl v9.2d, v9.2d, v12.2d \n"
            "srshl v10.2d, v10.2d, v12.2d \n"
            "srshl v11.2d, v11.2d, v12.2d \n"
            "trn1 v8.4s, v8.4s, v9.4s \n"
            "trn1 v10.4s, v10.4s, v11.4s \n"
            "st1 {v8.4s}, [%[o0]], #16 \n"
            "st1 {v10.4s}, [%[o1]], #16 \n"

            "smull2 v8.2d, v0.4s, v2.4s \n"
            "smull2 v9.2d, v1.4s, v3.4s \n"
            "smull2 v10.2d, v0.4s, v6.4s \n"
            "smull2 v11.2d, v1.4s, v7.4s \n"
            "smlsl2 v8.2d, v4.4s, v6.4s \n"
            "smlsl2 v9.2d, v5.4s, v7.4s \n"
            "smlal2 v10.2d, v2.4s, v4.4s \n"
            "smlal2 v11.2d, v3.4s, v5.4s \n"
            "srshl v8.2d, v8.2d, v12.2d \n"
            "srshl v9.2d, v9.2d, v12.2d \n"
            "srshl v10.2d, v10.2d, v12.2d \n"
            "srshl v11.2d, v11.2d, v12.2d \n"
            "trn1 v8.4s, v8.4s, v9.4s \n"
            "trn1 v10.4s, v10.4s, v11.4s \n"
            "subs %[i], %[i], #8 \n"
            "st1 {v8.4s}, [%[o0]], #16 \n"
            "st1 {v10.4s}, [%[o1]], #16 \n"
            "bgt 1b \n"
            :
            : [o0] "r" (o0), [o1] "r" (o1), [in0] "r" (in0), [in1] "r" (in1), [in2] "r" (in2), [in3] "r" (in3), [i] "r" (i), [q] "r" (q)
            : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12"
        );
    } else {
        asm volatile (
            "dup v12.2d, %[q] \n"

            "1: \n"
            "ld2 {v0.4s, v1.4s}, [%[in0]], #32 \n"
            "ld2 {v2.4s, v3.4s}, [%[in2]], #32 \n"
            "smull v8.2d, v0.2s, v2.2s \n"
            "smull v9.2d, v1.2s, v3.2s \n"
            "ld2 {v6.4s, v7.4s}, [%[in3]], #32 \n"
            "ld2 {v4.4s, v5.4s}, [%[in1]], #32 \n"
            "smull v10.2d, v0.2s, v6.2s \n"
            "smull v11.2d, v1.2s, v7.2s \n"
            "smlal v8.2d, v4.2s, v6.2s \n"
            "smlal v9.2d, v5.2s, v7.2s \n"
            "smlsl v10.2d, v2.2s, v4.2s \n"
            "smlsl v11.2d, v3.2s, v5.2s \n"
            "srshl v8.2d, v8.2d, v12.2d \n"
            "srshl v9.2d, v9.2d, v12.2d \n"
            "srshl v10.2d, v10.2d, v12.2d \n"
            "srshl v11.2d, v11.2d, v12.2d \n"
            "trn1 v8.4s, v8.4s, v9.4s \n"
            "trn1 v10.4s, v10.4s, v11.4s \n"
            "st1 {v8.4s}, [%[o0]], #16 \n"
            "st1 {v10.4s}, [%[o1]], #16 \n"

            "smull2 v8.2d, v0.4s, v2.4s \n"
            "smull2 v9.2d, v1.4s, v3.4s \n"
            "smull2 v10.2d, v0.4s, v6.4s \n"
            "smull2 v11.2d, v1.4s, v7.4s \n"
            "smlal2 v8.2d, v4.4s, v6.4s \n"
            "smlal2 v9.2d, v5.4s, v7.4s \n"
            "smlsl2 v10.2d, v2.4s, v4.4s \n"
            "smlsl2 v11.2d, v3.4s, v5.4s \n"
            "srshl v8.2d, v8.2d, v12.2d \n"
            "srshl v9.2d, v9.2d, v12.2d \n"
            "srshl v10.2d, v10.2d, v12.2d \n"
            "srshl v11.2d, v11.2d, v12.2d \n"
            "trn1 v8.4s, v8.4s, v9.4s \n"
            "trn1 v10.4s, v10.4s, v11.4s \n"
            "subs %[i], %[i], #8 \n"
            "st1 {v8.4s}, [%[o0]], #16 \n"
            "st1 {v10.4s}, [%[o1]], #16 \n"
            "bgt 1b \n"
            :
            : [o0] "r" (o0), [o1] "r" (o1), [in0] "r" (in0), [in1] "r" (in1), [in2] "r" (in2), [in3] "r" (in3), [i] "r" (i), [q] "r" (q)
            : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12"
        );
    }
    out->real->q = 15;
    out->imag->q = 15;

    if (x->sign == y->sign) {
        out->sign = x->sign;
    } else {
        out->sign = -x->sign;
    }

    return STP_OK;
}
#endif

#if STP_FhogGradHist_Optimized
stp_ret STP_FhogGradHist(stp_image *x_grad, stp_image *y_grad,
        const stp_image *angle_table, const stp_image *norm_table,
        stp_image *out_hogs[], stp_int32 cell_w) {
    stp_int32 *x, *y, *o_table, *m_table, **p, *_p[20], table_w;
    stp_int32 i, j, h, w, c_w, h_w, c_s, c_mask, o, m;
    stp_int32 o0, o1, c0, c1, k0, k1;

    x = (stp_int32*)x_grad->p_mem;
    y = (stp_int32*)y_grad->p_mem;
    o_table = (stp_int32*)angle_table->p_mem;
    m_table = (stp_int32*)norm_table->p_mem;
    c_w = cell_w;
    h = x_grad->height;
    w = x_grad->width;
    table_w = norm_table->width;

    STP_Assert(c_w == 4); /* only support cell size 4 accelerating now */
    if (c_w == 4) c_s = 2, c_mask = 0x3;
    h_w = w >> c_s; /* hog width */
    c_w >>= 1;

    for (i = 0; i < 18; i++) {
        STP_ImageClear(out_hogs[i]);
        _p[i + 1] = (stp_int32*)out_hogs[i]->p_mem;
        out_hogs[i]->q = norm_table->q + 5 + 4*c_s + 1;
    }
    _p[0] = _p[18];
    _p[19] = _p[1];

    p = &_p[1];

    /* tri-linear interpolation unnormalized gradient histograms of the surround pixels */
    for (i = 0; i < h - c_w; i++) {
        for (j = 0; j < w - c_w; j++) {
            if ((i >= 2 && i < h - cell_w) && j == 2) {
                j = w - cell_w - 1;
                continue;
            }

            if (i < 2 || j < 2) {
                o = o_table[y[i*w + j]*table_w + x[i*w + j]];
                m = m_table[y[i*w + j]*table_w + x[i*w + j]];
                o0 = o >> 5;
                o1 = o0 + 1;
                c1 = o & 0x1f;
                c0 = 32 - c1;
                k0 = (i + c_w) & c_mask;
                k1 = (j + c_w) & c_mask;
                k0*=k1;
                p[o0][((i + c_w) >> c_s)*h_w + ((j + c_w) >> c_s)] += m*c0*k0; /* q6*q5*(q6 or q4) */
                p[o1][((i + c_w) >> c_s)*h_w + ((j + c_w) >> c_s)] += m*c1*k0;
            }

            if (i >= h - cell_w || j < 2) {
                o = o_table[y[(i + c_w)*w + j]*table_w + x[(i + c_w)*w + j]];
                m = m_table[y[(i + c_w)*w + j]*table_w + x[(i + c_w)*w + j]];
                o0 = o >> 5;
                o1 = o0 + 1;
                c1 = o & 0x1f;
                c0 = 32 - c1;
                k0 = (1 << c_s) - (i & c_mask);
                k1 = (j + c_w) & c_mask;
                k0*=k1;
                p[o0][(i >> c_s)*h_w + ((j + c_w) >> c_s)] += m*c0*k0; /* q6*q5*(q6 or q4) */
                p[o1][(i >> c_s)*h_w + ((j + c_w) >> c_s)] += m*c1*k0;
            }

            if (i < 2 || j >= w - cell_w) {
                o = o_table[y[i*w + j + c_w]*table_w + x[i*w + j + c_w]];
                m = m_table[y[i*w + j + c_w]*table_w + x[i*w + j + c_w]];
                o0 = o >> 5;
                o1 = o0 + 1;
                c1 = o & 0x1f;
                c0 = 32 - c1;
                k0 = (i + c_w) & c_mask;
                k1 = (1 << c_s) - (j & c_mask);
                k0*=k1;
                p[o0][((i + c_w) >> c_s)*h_w + (j >> c_s)] += m*c0*k0; /* q6*q5*(q6 or q4) */
                p[o1][((i + c_w) >> c_s)*h_w + (j >> c_s)] += m*c1*k0;
            }

            if (i >= h - cell_w || j >= w - cell_w) {
                o = o_table[y[(i + c_w)*w + j + c_w]*table_w + x[(i + c_w)*w + j + c_w]];
                m = m_table[y[(i + c_w)*w + j + c_w]*table_w + x[(i + c_w)*w + j + c_w]];
                o0 = o >> 5;
                o1 = o0 + 1;
                c1 = o & 0x1f;
                c0 = 32 - c1;
                k0 = (1 << c_s) - (i & c_mask);
                k1 = (1 << c_s) - (j & c_mask);
                k0*=k1;
                p[o0][(i >> c_s)*h_w + (j >> c_s)] += m*c0*k0; /* q6*q5*(q6 or q4) */
                p[o1][(i >> c_s)*h_w + (j >> c_s)] += m*c1*k0;
            }
        }
    }

    /* tri-linear interpolation unnormalized gradient histograms of the centeral pixels */
    i = h - cell_w;
    j = w - cell_w;
    x = x + c_w*w + c_w;
    y = y + c_w*w + c_w;

    stp_uint64 params[7];
    params[0] = (stp_uint64)j;
    params[1] = (stp_uint64)i;
    params[2] = (stp_uint64)p;
    params[3] = (stp_uint64)y;
    params[4] = (stp_uint64)x;
    params[5] = (stp_uint64)m_table;
    params[6] = (stp_uint64)o_table;

    asm volatile (
        "stp x0, x1, [sp, #-16]! \n"
        "stp x2, x3, [sp, #-16]! \n"
        "stp x4, x5, [sp, #-16]! \n"
        "stp x6, x7, [sp, #-16]! \n"
        "stp x8, x9, [sp, #-16]! \n"
        "stp x10, x11, [sp, #-16]! \n"
        "stp x12, x13, [sp, #-16]! \n"
        "mov x7, %[params] \n"
        "ldr x6, [x7] \n"
        "ldr x5, [x7, #8] \n"
        "ldr x4, [x7, #16] \n"
        "ldr x3, [x7, #24] \n"
        "ldr x2, [x7, #32] \n"
        "ldr x1, [x7, #40] \n"
        "ldr x0, [x7, #48] \n"
        "movi v5.4s, #4 \n"
        "mov w7, #4 \n"
        "mov v6.s[0], w7 \n"
        "mov w7, #3 \n"
        "mov v6.s[1], w7 \n"
        "mov w7, #2 \n"
        "mov v6.s[2], w7 \n"
        "mov w7, #1 \n"
        "mov v6.s[3], w7 \n"
        "sub v7.4s, v5.4s, v6.4s \n"
        "dup v10.2d, x6 \n"
        "mov x13, x6 \n"

        "movi v8.4s, #4 \n"
        "movi v9.4s, #0 \n"

        "1: \n"
        "str x5, [sp, #-16]! \n"
        "mov x6, #4 \n"

        "2: \n"
        "str x6, [sp, #-16]! \n"
        "mov x7, v10.d[0] \n"

        "3: \n"
        "str x7, [sp, #-16]! \n"

        "ld1 {v0.4s}, [x3], #16 \n"
        "ld1 {v1.4s}, [x2], #16 \n"
        "shl v0.4s, v0.4s, #10 \n"
        "shl v1.4s, v1.4s, #2 \n"
        "sub v3.16b, v3.16b, v3.16b \n"
        "add v0.4s, v0.4s, v1.4s \n"
        "sub v0.4s, v3.4s, v0.4s \n"
        "smov x5, v0.s[0] \n"
        "smov x6, v0.s[1] \n"
        "smov x7, v0.s[2] \n"
        "smov x8, v0.s[3] \n"
        "sub x1, x1, x5 \n"
        "sub x0, x0, x5 \n"
        "ld1 {v0.s}[0], [x1], x5 \n"
        "ld1 {v1.s}[0], [x0], x5 \n"
        "sub x1, x1, x6 \n"
        "sub x0, x0, x6 \n"
        "ld1 {v0.s}[1], [x1], x6 \n"
        "ld1 {v1.s}[1], [x0], x6 \n"
        "sub x1, x1, x7 \n"
        "sub x0, x0, x7 \n"
        "ld1 {v0.s}[2], [x1], x7 \n"
        "ld1 {v1.s}[2], [x0], x7 \n"
        "sub x1, x1, x8 \n"
        "sub x0, x0, x8 \n"
        "ld1 {v0.s}[3], [x1], x8 \n"
        "ld1 {v1.s}[3], [x0], x8 \n"


        "movi v5.4s, #0x1f \n"
        "and v3.16b, v1.16b, v5.16b \n"
        "movi v5.4s, #8 \n"
        "sshr v1.4s, v1.4s, #2 \n"
        "add v1.4s, v1.4s, v5.4s \n"
        "movi v5.4s, #32 \n"
        "bic v1.4s, #0x7 \n"
        "sub v4.4s, v5.4s, v3.4s \n"
        "smov x5, v1.s[0] \n"
        "smov x6, v1.s[1] \n"
        "smov x7, v1.s[2] \n"
        "smov x8, v1.s[3] \n"
        "ldr x9, [x4, x5] \n"
        "ldr x10, [x4, x6] \n"
        "ldr x11, [x4, x7] \n"
        "ldr x12, [x4, x8] \n"

        "mul v1.4s, v0.4s, v4.4s \n"
        "mul v0.4s, v0.4s, v3.4s \n"

        "sub x5, x5, #8 \n"
        "sub x6, x6, #8 \n"
        "sub x7, x7, #8 \n"
        "sub x8, x8, #8 \n"
        "ldr x5, [x4, x5] \n"
        "ldr x6, [x4, x6] \n"
        "ldr x7, [x4, x7] \n"
        "ldr x8, [x4, x8] \n"

        "mul v4.4s, v1.4s, v6.4s \n"
        "mul v3.4s, v0.4s, v6.4s \n"
        "mul v4.4s, v4.4s, v8.4s \n"
        "mul v3.4s, v3.4s, v8.4s \n"
        "ld1 {v12.s}[0], [x5] \n"
        "ld1 {v12.s}[1], [x9] \n"
        "trn1 v14.4s, v4.4s, v3.4s \n"
        "trn2 v15.4s, v4.4s, v3.4s \n"
        "mov d4, v14.d[1] \n"
        "mov d3, v15.d[1] \n"
        "add v12.2s, v12.2s, v14.2s \n"
        "st1 {v12.s}[0], [x5], #4 \n"
        "st1 {v12.s}[1], [x9], #4 \n"
        "ld1 {v13.s}[0], [x6] \n"
        "ld1 {v13.s}[1], [x10] \n"
        "add v13.2s, v13.2s, v15.2s \n"
        "st1 {v13.s}[0], [x6], #4 \n"
        "st1 {v13.s}[1], [x10], #4 \n"
        "ld1 {v12.s}[0], [x7] \n"
        "ld1 {v12.s}[1], [x11] \n"
        "add v12.2s, v12.2s, v4.2s \n"
        "st1 {v12.s}[0], [x7], #4 \n"
        "st1 {v12.s}[1], [x11], #4 \n"
        "ld1 {v13.s}[0], [x8] \n"
        "ld1 {v13.s}[1], [x12] \n"
        "add v13.2s, v13.2s, v3.2s \n"
        "st1 {v13.s}[0], [x8], #4 \n"
        "st1 {v13.s}[1], [x12], #4 \n"

        "mul v4.4s, v1.4s, v7.4s \n"
        "mul v3.4s, v0.4s, v7.4s \n"
        "mul v4.4s, v4.4s, v8.4s \n"
        "mul v3.4s, v3.4s, v8.4s \n"
        "ld1 {v12.s}[0], [x5] \n"
        "ld1 {v12.s}[1], [x9] \n"
        "trn1 v14.4s, v4.4s, v3.4s \n"
        "trn2 v15.4s, v4.4s, v3.4s \n"
        "mov d4, v14.d[1] \n"
        "mov d3, v15.d[1] \n"
        "add v12.2s, v12.2s, v14.2s \n"
        "st1 {v12.s}[0], [x5] \n"
        "st1 {v12.s}[1], [x9] \n"
        "ld1 {v13.s}[0], [x6] \n"
        "ld1 {v13.s}[1], [x10] \n"
        "add v13.2s, v13.2s, v15.2s \n"
        "st1 {v13.s}[0], [x6] \n"
        "st1 {v13.s}[1], [x10] \n"
        "ld1 {v12.s}[0], [x7] \n"
        "ld1 {v12.s}[1], [x11] \n"
        "add v12.2s, v12.2s, v4.2s \n"
        "st1 {v12.s}[0], [x7] \n"
        "st1 {v12.s}[1], [x11] \n"
        "ld1 {v13.s}[0], [x8] \n"
        "ld1 {v13.s}[1], [x12] \n"
        "add v13.2s, v13.2s, v3.2s \n"
        "st1 {v13.s}[0], [x8] \n"
        "st1 {v13.s}[1], [x12] \n"

        "add x5, x5, x13 \n"
        "add x6, x6, x13 \n"
        "add x7, x7, x13 \n"
        "add x8, x8, x13 \n"
        "add x9, x9, x13 \n"
        "add x10, x10, x13 \n"
        "add x11, x11, x13 \n"
        "add x12, x12, x13 \n"

        "mul v4.4s, v1.4s, v6.4s \n"
        "mul v3.4s, v0.4s, v6.4s \n"
        "mul v4.4s, v4.4s, v9.4s \n"
        "mul v3.4s, v3.4s, v9.4s \n"
        "ld1 {v12.s}[0], [x5] \n"
        "ld1 {v12.s}[1], [x9] \n"
        "trn1 v14.4s, v4.4s, v3.4s \n"
        "trn2 v15.4s, v4.4s, v3.4s \n"
        "mov d4, v14.d[1] \n"
        "mov d3, v15.d[1] \n"
        "add v12.2s, v12.2s, v14.2s \n"
        "st1 {v12.s}[0], [x5], #4 \n"
        "st1 {v12.s}[1], [x9], #4 \n"
        "ld1 {v13.s}[0], [x6] \n"
        "ld1 {v13.s}[1], [x10] \n"
        "add v13.2s, v13.2s, v15.2s \n"
        "st1 {v13.s}[0], [x6], #4 \n"
        "st1 {v13.s}[1], [x10], #4 \n"
        "ld1 {v12.s}[0], [x7] \n"
        "ld1 {v12.s}[1], [x11] \n"
        "add v12.2s, v12.2s, v4.2s \n"
        "st1 {v12.s}[0], [x7], #4 \n"
        "st1 {v12.s}[1], [x11], #4 \n"
        "ld1 {v13.s}[0], [x8] \n"
        "ld1 {v13.s}[1], [x12] \n"
        "add v13.2s, v13.2s, v3.2s \n"
        "st1 {v13.s}[0], [x8], #4 \n"
        "st1 {v13.s}[1], [x12], #4 \n"

        "mul v4.4s, v1.4s, v7.4s \n"
        "mul v3.4s, v0.4s, v7.4s \n"
        "mul v4.4s, v4.4s, v9.4s \n"
        "mul v3.4s, v3.4s, v9.4s \n"
        "ld1 {v12.s}[0], [x5] \n"
        "ld1 {v12.s}[1], [x9] \n"
        "trn1 v14.4s, v4.4s, v3.4s \n"
        "trn2 v15.4s, v4.4s, v3.4s \n"
        "mov d4, v14.d[1] \n"
        "mov d3, v15.d[1] \n"
        "add v12.2s, v12.2s, v14.2s \n"
        "st1 {v12.s}[0], [x5] \n"
        "st1 {v12.s}[1], [x9] \n"
        "ld1 {v13.s}[0], [x6] \n"
        "ld1 {v13.s}[1], [x10] \n"
        "add v13.2s, v13.2s, v15.2s \n"
        "st1 {v13.s}[0], [x6] \n"
        "st1 {v13.s}[1], [x10] \n"
        "ld1 {v12.s}[0], [x7] \n"
        "ld1 {v12.s}[1], [x11] \n"
        "add v12.2s, v12.2s, v4.2s \n"
        "st1 {v12.s}[0], [x7] \n"
        "st1 {v12.s}[1], [x11] \n"
        "ld1 {v13.s}[0], [x8] \n"
        "ld1 {v13.s}[1], [x12] \n"
        "add v13.2s, v13.2s, v3.2s \n"
        "st1 {v13.s}[0], [x8] \n"
        "st1 {v13.s}[1], [x12] \n"

        "mov x5, #4 \n"
        "dup v5.2d, x5 \n"
        "sub x4, x4, #8 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #80 \n"
        "add v0.2d, v0.2d, v5.2d \n"
        "add v1.2d, v1.2d, v5.2d \n"
        "add v2.2d, v2.2d, v5.2d \n"
        "add v3.2d, v3.2d, v5.2d \n"
        "add v4.2d, v4.2d, v5.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #80 \n"
        "add v0.2d, v0.2d, v5.2d \n"
        "add v1.2d, v1.2d, v5.2d \n"
        "add v2.2d, v2.2d, v5.2d \n"
        "add v3.2d, v3.2d, v5.2d \n"
        "add v4.2d, v4.2d, v5.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #152 \n"

        "ldr x7, [sp], #16 \n"
        "subs x7, x7, #4 \n"
        "bgt 3b \n"

        "sub x4, x4, #8 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #80 \n"
        "sub v0.2d, v0.2d, v10.2d \n"
        "sub v1.2d, v1.2d, v10.2d \n"
        "sub v2.2d, v2.2d, v10.2d \n"
        "sub v3.2d, v3.2d, v10.2d \n"
        "sub v4.2d, v4.2d, v10.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #80 \n"
        "sub v0.2d, v0.2d, v10.2d \n"
        "sub v1.2d, v1.2d, v10.2d \n"
        "sub v2.2d, v2.2d, v10.2d \n"
        "sub v3.2d, v3.2d, v10.2d \n"
        "sub v4.2d, v4.2d, v10.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #152 \n"

        "ld1 {v0.4s}, [x3], #16 \n"
        "ld1 {v1.4s}, [x2], #16 \n"
        "movi v5.4s, #1 \n"
        "add v9.4s, v9.4s, v5.4s \n"
        "movi v5.4s, #4 \n"
        "bic v9.4s, #0x0c \n"
        "sub v8.4s, v5.4s, v9.4s \n"

        "ldr x6, [sp], #16 \n"
        "subs x6, x6, #1 \n"
        "bgt 2b \n"

        "mov x5, #4 \n"
        "dup v5.2d, x5 \n"
        "sub x4, x4, #8 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "add v5.2d, v5.2d, v10.2d \n"
        "sub x4, x4, #80 \n"
        "add v0.2d, v0.2d, v5.2d \n"
        "add v1.2d, v1.2d, v5.2d \n"
        "add v2.2d, v2.2d, v5.2d \n"
        "add v3.2d, v3.2d, v5.2d \n"
        "add v4.2d, v4.2d, v5.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "ld1 {v0.2d}, [x4], #16 \n"
        "ld1 {v1.2d}, [x4], #16 \n"
        "ld1 {v2.2d}, [x4], #16 \n"
        "ld1 {v3.2d}, [x4], #16 \n"
        "ld1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #80 \n"
        "add v0.2d, v0.2d, v5.2d \n"
        "add v1.2d, v1.2d, v5.2d \n"
        "add v2.2d, v2.2d, v5.2d \n"
        "add v3.2d, v3.2d, v5.2d \n"
        "add v4.2d, v4.2d, v5.2d \n"
        "st1 {v0.2d}, [x4], #16 \n"
        "st1 {v1.2d}, [x4], #16 \n"
        "st1 {v2.2d}, [x4], #16 \n"
        "st1 {v3.2d}, [x4], #16 \n"
        "st1 {v4.2d}, [x4], #16 \n"
        "sub x4, x4, #152 \n"

        "ldr x5, [sp], #16 \n"
        "subs x5, x5, #4 \n"
        "bgt 1b \n"

        "ldp x12, x13, [sp], #16 \n"
        "ldp x10, x11, [sp], #16 \n"
        "ldp x8, x9, [sp], #16 \n"
        "ldp x6, x7, [sp], #16 \n"
        "ldp x4, x5, [sp], #16 \n"
        "ldp x2, x3, [sp], #16 \n"
        "ldp x0, x1, [sp], #16 \n"
        :
        : [params] "r" (params)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v12", "v13", "v14", "v15"
    );

    return STP_OK;
}
#endif

#if STP_FFT2DLayerCore_Optimized
stp_ret STP_FFT2DLayerCore(stp_complex_image *out, stp_int32 block_num, stp_complex_image *temp,
        stp_complex_image *x, const stp_complex_image *table, stp_int32 layer_n) {
    stp_int32 i, j, k, l, m, n, ds;
    stp_int32 *in, *i0, *i1, *i2, *i3;
    stp_int32 *o, *o0, *o1, *o2, *o3;
    stp_int32 wo, wi, ho, hi;
    stp_int32 in_step0, in_step1, in_step2;
    stp_int32 out_step0, out_step1, out_step2, out_step3;
    stp_int32 out_off0, out_off1, out_off2, out_off3;
    stp_int32 a, b, c, d;
    volatile stp_int32 s;

    STP_Assert(out && x && table);

    STP_ComplexImageDotMulC(temp, x, table);

    wi = x->real->width;
    hi = x->real->height;
    s = (1 << (layer_n++));
    ds = s << 1;
    in_step0 = wi - s;
    in_step1 = ds - s*wi;
    in_step2 = ds*wi - wi;
    m = wi >> layer_n;

    wo = out->real->width;
    ho = out->real->height;

    if (wo > ho) {
        /* last step of 2d fft */
        out_step0 = 2;
        out_step1 = ho;
        out_step2 = 0;
        out_step3 = 0;
        out_off0 = block_num;
        out_off1 = (wo >> 1) + out_off0;
        out_off2 = s*wo + out_off0;
        out_off3 = (wo >> 1) + out_off2;
    } else if (wo < ho) {
        /* last step of 2d fft */
        out_step0 = 1;
        out_step1 = wo*3 >> 1;
        out_step2 = 0;
        out_step3 = 0;
        out_off0 = wo*block_num;
        out_off1 = (wo >> 1) + out_off0;
        out_off2 = ds*wo + out_off0;
        out_off3 = (wo >> 1) + out_off2;
    } else {
        out_step0 = 1;
        out_step1 = in_step0;
        out_step2 = in_step1;
        out_step3 = in_step2;
        out_off0 = 0;
        out_off1 = out_off0 + s;
        out_off2 = s*wo;
        out_off3 = out_off2 + s;
        block_num = 0;
    }

    for (n = 0; n < 2; n++) {
        if (n == 0) in = (stp_int32*)temp->real->p_mem, o = (stp_int32*)out->real->p_mem;
        else in = (stp_int32*)temp->imag->p_mem, o = (stp_int32*)out->imag->p_mem;

        i0 = in;
        i1 = i0 + s;
        i2 = i0 + s*wi;
        i3 = i2 + s;
        o0 = o + out_off0;
        o1 = o + out_off1;
        o2 = o + out_off2;
        o3 = o + out_off3;

        if (layer_n > 2 && wo == ho) {
            for (i = 0; i < m; i++) {
                for (j = 0; j < m; j++) {
                    for (k = 0; k < s; k++) {
                        asm volatile (
                            "1: \n"
                            "ld1 {v0.4s}, [%[i0]], #16 \n"
                            "ld1 {v1.4s}, [%[i1]], #16 \n"
                            "ld1 {v2.4s}, [%[i2]], #16 \n"
                            "ld1 {v3.4s}, [%[i3]], #16 \n"
                            "add v4.4s, v0.4s, v1.4s \n"
                            "sub v5.4s, v0.4s, v1.4s \n"
                            "add v0.4s, v2.4s, v3.4s \n"
                            "sub v1.4s, v2.4s, v3.4s \n"
                            "add v2.4s, v4.4s, v0.4s \n"
                            "add v3.4s, v5.4s, v1.4s \n"
                            "st1 {v2.4s}, [%[o0]], #16 \n"
                            "st1 {v3.4s}, [%[o1]], #16 \n"
                            "sub v0.4s, v4.4s, v0.4s \n"
                            "sub v1.4s, v5.4s, v1.4s \n"
                            "subs %[s], %[s], #4 \n"
                            "st1 {v0.4s}, [%[o2]], #16 \n"
                            "st1 {v1.4s}, [%[o3]], #16 \n"
                            "bgt 1b \n"
                            : [i0] "+r" (i0), [i1] "+r" (i1), [i2] "+r" (i2), [i3] "+r" (i3), [o0] "+r" (o0), [o1] "+r" (o1), [o2] "+r" (o2), [o3] "+r" (o3)
                            : [s] "r" (s)
                            : "memory", "v0", "v1", "v2", "v3", "v4", "v5"
                        );
                        i0 += in_step0, i1 += in_step0, i2 += in_step0, i3 += in_step0;
                        o0 += out_step1, o1 += out_step1, o2 += out_step1, o3 += out_step1;
                    }
                    i0 += in_step1, i1 += in_step1, i2 += in_step1, i3 += in_step1;
                    o0 += out_step2, o1 += out_step2, o2 += out_step2, o3 += out_step2;
                }
                i0 += in_step2, i1 += in_step2, i2 += in_step2, i3 += in_step2;
                o0 += out_step3, o1 += out_step3, o2 += out_step3, o3 += out_step3;
            }
        } else {
            for (i = 0; i < m; i++) {
                for (j = 0; j < m; j++) {
                    for (k = 0; k < s; k++) {
                        for (l = 0; l < s; l++) {
                            a = *i0 + *i1;
                            b = *i0++ - *i1++;
                            c = *i2 + *i3;
                            d = *i2++ - *i3++;
                            *o0 = a + c;
                            *o1 = b + d;
                            *o2 = a - c;
                            *o3 = b - d;
                            o0 += out_step0, o1 += out_step0, o2 += out_step0, o3 += out_step0;
                        }
                        i0 += in_step0, i1 += in_step0, i2 += in_step0, i3 += in_step0;
                        o0 += out_step1, o1 += out_step1, o2 += out_step1, o3 += out_step1;
                    }
                    i0 += in_step1, i1 += in_step1, i2 += in_step1, i3 += in_step1;
                    o0 += out_step2, o1 += out_step2, o2 += out_step2, o3 += out_step2;
                }
                i0 += in_step2, i1 += in_step2, i2 += in_step2, i3 += in_step2;
                o0 += out_step3, o1 += out_step3, o2 += out_step3, o3 += out_step3;
            }
        }
    }

    out->real->q = temp->real->q;
    out->imag->q = temp->imag->q;
    out->sign = temp->sign;

    return STP_OK;
}
#endif

#if STP_ImageConvertDataType_Optimized
stp_ret STP_ImageConvertDataType(stp_image* out_image, stp_image* in_image, STP_DataType dest_type) {
    stp_int32 i;
    STP_Assert(in_image && out_image);
    i = STP_ALIGN_ON4(in_image->width)*STP_ALIGN_ON4(in_image->height);

    if (in_image->data_type != dest_type) {
        if (dest_type == STP_INT32) {
            stp_int32 *o = (stp_int32*)out_image->p_mem;
            float *in = (float*)in_image->p_mem;
            asm volatile (
                "1: \n"
                "ld1 {v0.4s}, [%[in]], #16 \n"
                "ld1 {v1.4s}, [%[in]], #16 \n"
                "fcvtzs v0.4s, v0.4s, #15 \n"
                "fcvtzs v1.4s, v1.4s, #15 \n"
                "subs %[i], %[i], #8 \n"
                "st1 {v0.4s}, [%[o]], #16 \n"
                "st1 {v1.4s}, [%[o]], #16 \n"
                "bgt 1b \n"
                :
                : [o] "r" (o), [in] "r" (in), [i] "r" (i)
                : "memory", "v0", "v1"
            );
            out_image->q = in_image->q + 15;
            out_image->data_type = STP_INT32;
        } else {
            float *o = (float*)out_image->p_mem;
            stp_int32 *in = (stp_int32*)in_image->p_mem;

            asm volatile (
                "1: \n"
                "ld1 {v0.4s}, [%[in]], #16 \n"
                "ld1 {v1.4s}, [%[in]], #16 \n"
                "scvtf v0.4s, v0.4s, #15 \n"
                "scvtf v1.4s, v1.4s, #15 \n"
                "subs %[i], %[i], #8 \n"
                "st1 {v0.4s}, [%[o]], #16 \n"
                "st1 {v1.4s}, [%[o]], #16 \n"
                "bgt 1b \n"
                :
                : [o] "r" (o), [in] "r" (in), [i] "r" (i)
                : "memory", "v0", "v1"
            );
            out_image->data_type = STP_FLOAT32;
            out_image->q = in_image->q - 15;
        }
    } else {
        return STP_ERR_INVALID_PARAM;
    }
    return STP_OK;
}
#endif

#if STP_ImageCoefMix_Optimized
stp_ret STP_ImageCoefMix(stp_image *out_image, stp_image* in_image0, stp_image* in_image1, const float coef[2]) {
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

    i = STP_ALIGN_ON4(in_image0->width)*STP_ALIGN_ON4(in_image0->height);

    asm volatile (
        "dup v4.2d, %[c0] \n"
        "trn1 v4.4s, v4.4s, v4.4s \n"
        "dup v5.2d, %[c1] \n"
        "trn1 v5.4s, v5.4s, v5.4s \n"
        "1: \n"
        "ld1 {v0.4s}, [%[in0]], #16 \n"
        "ld1 {v1.4s}, [%[in1]], #16 \n"
        "fmul v0.4s, v0.4s, v4.4s \n"
        "ld1 {v2.4s}, [%[in0]], #16 \n"
        "ld1 {v3.4s}, [%[in1]], #16 \n"
        "fmul v2.4s, v2.4s, v4.4s \n"
        "fmla v0.4s, v1.4s, v5.4s \n"
        "fmla v2.4s, v3.4s, v5.4s \n"
        "subs %[i], %[i], #8 \n"
        "st1 {v0.4s}, [%[out]], #16 \n"
        "st1 {v2.4s}, [%[out]], #16 \n"
        "bgt 1b \n"
        :
        : [out] "r" (out), [in0] "r" (in0), [in1] "r" (in1), [c0] "r" (coefs[0]), [c1] "r" (coefs[1]), [i] "r" (i)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5"
    );

    out_image->data_type = STP_FLOAT32;
    out_image->q = in_image0->q;
    return STP_OK;
}
#endif

#if STP_FFT1D32PointRealInCore_Optimized
stp_ret STP_FFT1D32PointRealInCore(stp_complex_image *out, stp_complex_image* temp,
        stp_complex_image* table[], stp_image *x) {
    stp_uint64 params[10];
    params[0] = (stp_uint64)out->real->p_mem;
    params[1] = (stp_uint64)out->imag->p_mem;
    params[2] = (stp_uint64)table[4]->real->p_mem;
    params[3] = (stp_uint64)table[4]->imag->p_mem;
    params[4] = (stp_uint64)table[3]->real->p_mem;
    params[5] = (stp_uint64)table[3]->imag->p_mem;
    params[6] = (stp_uint64)table[2]->real->p_mem;
    params[7] = (stp_uint64)table[2]->imag->p_mem;
    params[8] = (stp_uint64)x->p_mem;
    params[9] = (stp_uint64)temp;

    asm volatile (
        "stp x0, x1, [sp, #-16]! \n"
        "stp x2, x3, [sp, #-16]! \n"
        "stp x4, x5, [sp, #-16]! \n"
        "stp x6, x7, [sp, #-16]! \n"
        "stp x8, x9, [sp, #-16]! \n"
        "mov x9, %[params] \n"
        "ldr x0, [x9] \n"
        "ldr x1, [x9, #8] \n"
        "ldr x2, [x9, #16] \n"
        "ldr x3, [x9, #24] \n"
        "ldr x4, [x9, #32] \n"
        "ldr x5, [x9, #40] \n"
        "ldr x6, [x9, #48] \n"
        "ldr x7, [x9, #56] \n"
        "ldr x8, [x9, #64] \n"

        "1: \n"

        "ld1 {v0.4s, v1.4s, v2.4s, v3.4s}, [x8], #64 \n"
        "ld1 {v4.4s, v5.4s, v6.4s, v7.4s}, [x8], #64 \n"

        "sub v8.4s, v0.4s, v4.4s \n"
        "sub v9.4s, v1.4s, v5.4s \n"
        "sub v10.4s, v2.4s, v6.4s \n"
        "sub v11.4s, v3.4s, v7.4s \n"

        "add v0.4s, v0.4s, v4.4s \n"
        "add v1.4s, v1.4s, v5.4s \n"
        "add v2.4s, v2.4s, v6.4s \n"
        "add v3.4s, v3.4s, v7.4s \n"

        "ld1 {v16.4s, v17.4s, v18.4s, v19.4s}, [x2], #64 \n"
        "ld1 {v20.4s, v21.4s, v22.4s, v23.4s}, [x3], #64 \n"

        "sqrdmulh v4.4s, v8.4s, v16.4s \n"
        "sqrdmulh v5.4s, v9.4s, v17.4s \n"
        "sqrdmulh v6.4s, v10.4s, v18.4s \n"
        "sqrdmulh v7.4s, v11.4s, v19.4s \n"

        "sqrdmulh v12.4s, v8.4s, v20.4s \n"
        "sqrdmulh v13.4s, v9.4s, v21.4s \n"
        "sqrdmulh v14.4s, v10.4s, v22.4s \n"
        "sqrdmulh v15.4s, v11.4s, v23.4s \n"


        "2: \n"

        "sub v16.4s, v0.4s, v2.4s \n"
        "sub v17.4s, v1.4s, v3.4s \n"
        "sub v18.4s, v4.4s, v6.4s \n"
        "sub v19.4s, v5.4s, v7.4s \n"
        "sub v20.4s, v12.4s, v14.4s \n"
        "sub v21.4s, v13.4s, v15.4s \n"

        "ld1 {v22.4s, v23.4s}, [x4], #32 \n"
        "ld1 {v24.4s, v25.4s}, [x5], #32 \n"

        "add v0.4s, v0.4s, v2.4s \n"
        "add v1.4s, v1.4s, v3.4s \n"
        "add v4.4s, v4.4s, v6.4s \n"
        "add v5.4s, v5.4s, v7.4s \n"
        "add v12.4s, v12.4s, v14.4s \n"
        "add v13.4s, v13.4s, v15.4s \n"

        "sqrdmulh v2.4s, v16.4s, v22.4s \n"
        "sqrdmulh v3.4s, v17.4s, v23.4s \n"
        "sqrdmulh v6.4s, v18.4s, v22.4s \n"
        "sqrdmulh v7.4s, v19.4s, v23.4s \n"
        "sqrdmulh v10.4s, v16.4s, v24.4s \n"
        "sqrdmulh v11.4s, v17.4s, v25.4s \n"
        "sqrdmulh v14.4s, v18.4s, v24.4s \n"
        "sqrdmulh v15.4s, v19.4s, v25.4s \n"

        "sqrdmulh v24.4s, v20.4s, v24.4s \n"
        "sqrdmulh v25.4s, v21.4s, v25.4s \n"
        "sqrdmulh v26.4s, v20.4s, v22.4s \n"
        "sqrdmulh v27.4s, v21.4s, v23.4s \n"

        "sub v6.4s, v6.4s, v24.4s \n"
        "sub v7.4s, v7.4s, v25.4s \n"
        "add v14.4s, v14.4s, v26.4s \n"
        "add v15.4s, v15.4s, v27.4s \n"

        "sub v8.4s, v8.4s, v8.4s \n"
        "sub v9.4s, v9.4s, v9.4s \n"

        "3: \n"

        "sub v16.4s, v0.4s, v1.4s \n"
        "sub v17.4s, v2.4s, v3.4s \n"
        "sub v18.4s, v4.4s, v5.4s \n"
        "sub v19.4s, v6.4s, v7.4s \n"
        "sub v20.4s, v8.4s, v9.4s \n"
        "sub v21.4s, v10.4s, v11.4s \n"
        "sub v22.4s, v12.4s, v13.4s \n"
        "sub v23.4s, v14.4s, v15.4s \n"

        "add v0.4s, v0.4s, v1.4s \n"
        "add v2.4s, v2.4s, v3.4s \n"
        "add v4.4s, v4.4s, v5.4s \n"
        "add v6.4s, v6.4s, v7.4s \n"
        "add v8.4s, v8.4s, v9.4s \n"
        "add v10.4s, v10.4s, v11.4s \n"
        "add v12.4s, v12.4s, v13.4s \n"
        "add v14.4s, v14.4s, v15.4s \n"

        "ld1 {v24.4s}, [x6], #16 \n"
        "ld1 {v25.4s}, [x7], #16 \n"

        "sqrdmulh v1.4s, v16.4s, v24.4s \n"
        "sqrdmulh v3.4s, v17.4s, v24.4s \n"
        "sqrdmulh v5.4s, v18.4s, v24.4s \n"
        "sqrdmulh v7.4s, v19.4s, v24.4s \n"
        "sqrdmulh v9.4s, v16.4s, v25.4s \n"
        "sqrdmulh v11.4s, v17.4s, v25.4s \n"
        "sqrdmulh v13.4s, v18.4s, v25.4s \n"
        "sqrdmulh v15.4s, v19.4s, v25.4s \n"

        "sqrdmulh v16.4s, v20.4s, v25.4s \n"
        "sqrdmulh v17.4s, v21.4s, v25.4s \n"
        "sqrdmulh v18.4s, v22.4s, v25.4s \n"
        "sqrdmulh v19.4s, v23.4s, v25.4s \n"
        "sqrdmulh v20.4s, v20.4s, v24.4s \n"
        "sqrdmulh v21.4s, v21.4s, v24.4s \n"
        "sqrdmulh v22.4s, v22.4s, v24.4s \n"
        "sqrdmulh v23.4s, v23.4s, v24.4s \n"

        "sub v1.4s, v1.4s, v16.4s \n"
        "sub v3.4s, v3.4s, v17.4s \n"
        "sub v5.4s, v5.4s, v18.4s \n"
        "sub v7.4s, v7.4s, v19.4s \n"
        "add v9.4s, v9.4s, v20.4s \n"
        "add v11.4s, v11.4s, v21.4s \n"
        "add v13.4s, v13.4s, v22.4s \n"
        "add v15.4s, v15.4s, v23.4s \n"


        "4: \n"

        "st1 {v0.4s, v1.4s, v2.4s, v3.4s}, [x0], #64 \n"
        "st1 {v4.4s, v5.4s, v6.4s, v7.4s}, [x0], #64 \n"
        "st1 {v8.4s, v9.4s, v10.4s, v11.4s}, [x1], #64 \n"
        "st1 {v12.4s, v13.4s, v14.4s, v15.4s}, [x1], #64 \n"

        "sub x0, x0, #128 \n"
        "sub x1, x1, #128 \n"

        "ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [x0], #64 \n"
        "ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [x1], #64 \n"
        "ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [x0], #64 \n"
        "ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [x1], #64 \n"

        "sub x0, x0, #128 \n"
        "sub x1, x1, #128 \n"

        "add v16.4s, v0.4s, v1.4s \n"
        "sub v17.4s, v0.4s, v1.4s \n"
        "add v18.4s, v0.4s, v9.4s \n"
        "sub v19.4s, v0.4s, v9.4s \n"
        "add v24.4s, v8.4s, v9.4s \n"
        "sub v25.4s, v8.4s, v9.4s \n"
        "sub v26.4s, v8.4s, v1.4s \n"
        "add v27.4s, v8.4s, v1.4s \n"

        "add v16.4s, v16.4s, v2.4s \n"
        "add v17.4s, v17.4s, v2.4s \n"
        "sub v18.4s, v18.4s, v2.4s \n"
        "sub v19.4s, v19.4s, v2.4s \n"
        "add v24.4s, v24.4s, v10.4s \n"
        "add v25.4s, v25.4s, v10.4s \n"
        "sub v26.4s, v26.4s, v10.4s \n"
        "sub v27.4s, v27.4s, v10.4s \n"

        "add v16.4s, v16.4s, v3.4s \n"
        "sub v17.4s, v17.4s, v3.4s \n"
        "sub v18.4s, v18.4s, v11.4s \n"
        "add v19.4s, v19.4s, v11.4s \n"
        "add v24.4s, v24.4s, v11.4s \n"
        "sub v25.4s, v25.4s, v11.4s \n"
        "add v26.4s, v26.4s, v3.4s \n"
        "sub v27.4s, v27.4s, v3.4s \n"

        "add v20.4s, v4.4s, v5.4s \n"
        "sub v21.4s, v4.4s, v5.4s \n"
        "add v22.4s, v4.4s, v13.4s \n"
        "sub v23.4s, v4.4s, v13.4s \n"
        "add v28.4s, v12.4s, v13.4s \n"
        "sub v29.4s, v12.4s, v13.4s \n"
        "sub v30.4s, v12.4s, v5.4s \n"
        "add v31.4s, v12.4s, v5.4s \n"

        "add v20.4s, v20.4s, v6.4s \n"
        "add v21.4s, v21.4s, v6.4s \n"
        "sub v22.4s, v22.4s, v6.4s \n"
        "sub v23.4s, v23.4s, v6.4s \n"
        "add v28.4s, v28.4s, v14.4s \n"
        "add v29.4s, v29.4s, v14.4s \n"
        "sub v30.4s, v30.4s, v14.4s \n"
        "sub v31.4s, v31.4s, v14.4s \n"

        "add v20.4s, v20.4s, v7.4s \n"
        "sub v21.4s, v21.4s, v7.4s \n"
        "sub v22.4s, v22.4s, v15.4s \n"
        "add v23.4s, v23.4s, v15.4s \n"
        "add v28.4s, v28.4s, v15.4s \n"
        "sub v29.4s, v29.4s, v15.4s \n"
        "add v30.4s, v30.4s, v7.4s \n"
        "sub v31.4s, v31.4s, v7.4s \n"

        "st1 {v16.4s, v17.4s, v18.4s, v19.4s}, [x0], #64 \n"
        "st1 {v24.4s, v25.4s, v26.4s, v27.4s}, [x1], #64 \n"
        "st1 {v20.4s, v21.4s, v22.4s, v23.4s}, [x0], #64 \n"
        "st1 {v28.4s, v29.4s, v30.4s, v31.4s}, [x1], #64 \n"

        "sub x0, x0, #128 \n"
        "sub x1, x1, #128 \n"

        "ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [x0], #64 \n"
        "ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [x1], #64 \n"
        "ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [x0], #64 \n"
        "ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [x1], #64 \n"

        "sub x0, x0, #128 \n"
        "sub x1, x1, #128 \n"

        "st1 {v0.4s, v1.4s, v2.4s, v3.4s}, [x0], #64 \n"
        "st1 {v8.4s, v9.4s, v10.4s, v11.4s}, [x1], #64 \n"
        "st1 {v4.4s, v5.4s, v6.4s, v7.4s}, [x0], #64 \n"
        "st1 {v12.4s, v13.4s, v14.4s, v15.4s}, [x1], #64 \n"

        "ldp x8, x9, [sp], #16 \n"
        "ldp x6, x7, [sp], #16 \n"
        "ldp x4, x5, [sp], #16 \n"
        "ldp x2, x3, [sp], #16 \n"
        "ldp x0, x1, [sp], #16 \n"
        :
        : [params] "r" (params)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7" \
          , "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15" \
          , "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23" \
          , "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
    );

    out->real->q = x->q;
    out->imag->q = x->q;
    out->sign = 1;

    return STP_OK;
}
#endif


