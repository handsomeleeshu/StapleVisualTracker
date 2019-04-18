/*
 * staple_neon_opt.c

 *
 *  Created on: 2018-10-13
 *      Author: handsomelee
 */

#include "stp_image.h"
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
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vadd.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1"
    );

    out->q = x->q;

    return STP_OK;
}
#endif


#if STP_ImageDotMul_Optimized
stp_ret STP_ImageDotMul(stp_image *out, const stp_image *x, const stp_image *y) {
    stp_int32 i;
    stp_int32 q;
    stp_int32 *in0, *in1, *o;
    STP_Assert(out && x && y);

    in0 = (stp_int32*)x->p_mem;
    in1 = (stp_int32*)y->p_mem;
    o = (stp_int32*)out->p_mem;

    i = x->width*x->height;
    q = 15 - x->q - y->q;

    asm volatile (
        "vdup.s32 q4, %[q] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vmull.s32 q2, d0, d2 \n"
        "vmull.s32 q3, d1, d3 \n"
        "vrshl.s64 q0, q2, q4 \n"
        "vrshl.s64 q1, q3, q4 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {d0[0]}, [%[o]]! \n"
        "vst1.32 {d1[0]}, [%[o]]! \n"
        "vst1.32 {d2[0]}, [%[o]]! \n"
        "vst1.32 {d3[0]}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i), [q] "r" (q)
        : "memory", "q0", "q1", "q2", "q3", "q4"
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

    if (s.q < 31 && v < 1LL << s.q) {
        v = v << (31 - s.q);
        asm volatile (
            "vdup.s32 q1, %[v] \n"
            "1: \n"
            "vld1.32 {q0}, [%[in]]! \n"
            "vqrdmulh.s32 q0, q0, q1 \n"
            "subs %[i], %[i], #4 \n"
            "vst1.32 {q0}, [%[o]]! \n"
            "bgt 1b \n"
            :
            : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
            : "memory", "q0", "q1"
        );
        out->q = x->q;
    } else {
        q = 15 - x->q - s.q;
        asm volatile (
            "vdup.s32 q4, %[q] \n"
            "vdup.s32 q3, %[v] \n"
            "1: \n"
            "vld1.32 {q0}, [%[in]]! \n"
            "vmull.s32 q1, d1, d7 \n"
            "vmull.s32 q0, d0, d6 \n"
            "vrshl.s64 q1, q1, q4 \n"
            "vrshl.s64 q0, q0, q4 \n"
            "subs %[i], %[i], #4 \n"
            "vst1.32 {d0[0]}, [%[o]]! \n"
            "vst1.32 {d1[0]}, [%[o]]! \n"
            "vst1.32 {d2[0]}, [%[o]]! \n"
            "vst1.32 {d3[0]}, [%[o]]! \n"
            "bgt 1b \n"
            :
            : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i), [q] "r" (q)
            : "memory", "q0", "q1", "q2", "q3", "q4"
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
        "vdup.s32 q1, %[neg_inf] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vmax.s32 q1, q0 \n"
        "subs %[i], %[i], #4 \n"
        "bgt 1b \n"
        "vpmax.s32 d2, d2, d2 \n"
        "vpmax.s32 d3, d3, d2 \n"
        "vpmax.s32 d3, d3, d3 \n"
        "vmov.32 %[max], d3[0] \n"
        : [max] "+r" (max)
        : [in] "r" (in), [i] "r" (i), [neg_inf] "r" (neg_inf)
        : "memory", "q0", "q1"
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
        "vdup.s32 q2, %[v] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vld1.32 {q1}, [%[in]]! \n"
        "vadd.s32 q0, q2 \n"
        "vadd.s32 q1, q2 \n"
        "vcvt.f32.s32 q0, q0, #15 \n"
        "vcvt.f32.s32 q1, q1, #15 \n"
        "vrecpe.f32 q0, q0 \n"
        "vrecpe.f32 q1, q1 \n"
        "vcvt.s32.f32 q0, q0, #15 \n"
        "vcvt.s32.f32 q1, q1, #15 \n"
        "subs %[i], %[i], #8 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "vst1.32 {q1}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
        : "memory", "q0", "q1", "q2"
    );

    out->q = 30 - x->q;

    return STP_OK;
}
#endif

#if STP_ImageResize_optimized
stp_ret STP_ImageResize(stp_image *outImage, const stp_image *inImage, const stp_rect_q *rect_q) {
    stp_int32 i;
    stp_int32 h, w;
    stp_int32 h_in, w_in, h_out, w_out;
    stp_int32 x, y, stepx_q16, stepy_q16, startx_q16, starty_q16, curx_q16, cury_q16;
    stp_int32 coefx[2][STP_ALIGN_ON4(outImage->width)], coefy[2][STP_ALIGN_ON4(outImage->height)];
    stp_int32 *in, *out, *p_coefx[2], *p_coefy[2];

    STP_Assert(outImage && inImage);

    if (rect_q) {
        x = (stp_int64)rect_q->pos.pos.x << 16 >> rect_q->pos.q; /* q16 */
        y = (stp_int64)rect_q->pos.pos.y << 16 >> rect_q->pos.q; /* q16 */
        h = (stp_int64)rect_q->size.size.h << 16 >> rect_q->size.q; /* q16 */
        w = (stp_int64)rect_q->size.size.w << 16 >> rect_q->size.q; /* q16 */
        startx_q16 = x - (w >> 1);
        starty_q16 = y - (h >> 1);
        w = (stp_int64)rect_q->size.size.w << 16 >> rect_q->size.q;
        h =  (stp_int64)rect_q->size.size.h << 16 >> rect_q->size.q;
    } else {
        startx_q16 = 0;
        starty_q16 = 0;
        w = inImage->width << 16;
        h = inImage->height << 16;
    }

    h_in = inImage->height;
    w_in = inImage->width;
    h_out = outImage->height;
    w_out = outImage->width;
    in = (stp_int32*)inImage->p_mem;
    out = (stp_int32*)outImage->p_mem;
    stepx_q16 = w/w_out;
    stepy_q16 = h/h_out;

    curx_q16 = startx_q16;
    for (i = 0; i < w_out; i++) {
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
    for (i = 0; i < h_out; i++) {
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

    stp_uint32 params[11];
    params[0] = (stp_uint32)stepx_q16;
    params[1] = (stp_uint32)stepy_q16;
    params[2] = (stp_uint32)w_out;
    params[3] = (stp_uint32)h_out;
    params[4] = (stp_uint32)w_in*4;
    params[5] = (stp_uint32)p_coefx;
    params[6] = (stp_uint32)startx_q16;
    params[7] = (stp_uint32)in;
    params[8] = (stp_uint32)out;
    params[9] = (stp_uint32)p_coefy;
    params[10] = (stp_uint32)starty_q16;

    asm volatile (
        "push {r0-r12} \n"
        "mov r11, %[params] \n"
        "ldm r11, {r0-r10} \n"
        "vdup.32 q9, r0 \n"
        "vdup.32 q11, r1 \n"
        "vdup.32 q10, r10 \n"
        "vmov.32 d16[0], r6 \n"
        "add r6, r0 \n"
        "vmov.32 d16[1], r6 \n"
        "add r6, r0 \n"
        "vmov.32 d17[0], r6 \n"
        "add r6, r0 \n"
        "vmov.32 d17[1], r6 \n"
        "vshl.s32 q9, q9, #2 \n"
        "ldr r6, [r5, #4] \n"
        "ldr r5, [r5] \n"
        "ldr r10, [r9, #4] \n"
        "ldr r9, [r9] \n"

        "1: \n"
        "ldr r0, [r9] \n"
        "ldr r1, [r10] \n"
        "vdup.32 q6, r0 \n"
        "vdup.32 q7, r1 \n"
        "add r9, #4 \n"
        "add r10, #4 \n"
        "vpush {q8} \n"
        "push {r2} \n"
        "push {r5} \n"
        "push {r6} \n"

        "2: \n"
        "vdup.32 q3, r4 \n"
        "vshr.u32 q1, q10, #16 \n"
        "vshr.u32 q0, q8, #16 \n"
        "vmul.u32 q1, q1, q3 \n"
        "vshl.u32 q0, #2 \n"
        "vadd.u32 q0, q1 \n"
        "vmov r0, r1, d1 \n"
        "add r0, r7 \n"
        "add r1, r7 \n"
        "vld1.32 {d1}, [r0] \n"
        "vld1.32 {d3}, [r1] \n"
        "add r0, r4 \n"
        "add r1, r4 \n"
        "vld1.32 {d5}, [r0] \n"
        "vld1.32 {d7}, [r1] \n"

        "vld1.32 {q4}, [r5]! \n"
        "vld1.32 {q5}, [r6]! \n"

        "vmov r0, r1, d0 \n"
        "add r0, r7 \n"
        "add r1, r7 \n"
        "vld1.32 {d0}, [r0] \n"
        "vld1.32 {d2}, [r1] \n"
        "add r0, r4 \n"
        "add r1, r4 \n"
        "vld1.32 {d4}, [r0] \n"
        "vld1.32 {d6}, [r1] \n"
        "vtrn.32 q0, q1 \n"
        "vtrn.32 q2, q3 \n"

        "vqrdmulh.s32 q0, q0, q6 \n"
        "vqrdmulh.s32 q2, q2, q7 \n"
        "vqrdmulh.s32 q1, q1, q6 \n"
        "vqrdmulh.s32 q3, q3, q7 \n"
        "vadd.s32 q0, q2 \n"
        "vadd.s32 q1, q3 \n"
        "vqrdmulh.s32 q0, q0, q4 \n"
        "vqrdmulh.s32 q1, q1, q5 \n"
        "vadd.s32 q0, q0, q1 \n"
        "vadd.s32 q8, q8, q9 \n"
        "subs r2, r2, #4 \n"
        "vst1.32 {q0}, [r8]! \n"
        "bgt 2b \n"

        "vadd.s32 q10, q11 \n"
        "lsl r2, #2 \n"
        "add r8, r2 \n"
        "subs r3, r3, #1 \n"
        "pop {r6} \n"
        "pop {r5} \n"
        "pop {r2} \n"
        "vpop {q8} \n"
        "bgt 1b \n"
        "pop {r0-r12} \n"
        :
        : [params] "r" (params)
        : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11"
    );


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
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 q0, q1, q0 \n"
        "vshl.s32 q0, q0, #1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[p]]! \n"
        "bgt 1b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1"
    );

    p += in->width;
    in1 += in->width;
    i = (in->height - 2)*in->width;
    asm volatile (
        "2: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 q0, q1, q0 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[p]]! \n"
        "bgt 2b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1"
    );

    p = (stp_int32*)y->p_mem + (in->height - 1)*in->width;
    in0 = (stp_int32*)in->p_mem + (in->height - 2)*in->width;
    in1 = in0 + in->width;
    i = in->width;
    asm volatile (
        "3: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 q0, q1, q0 \n"
        "vshl.s32 q0, q0, #1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[p]]! \n"
        "bgt 3b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "push {%[j]} \n"
        "5: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 q0, q1, q0 \n"
        "subs %[j], %[j], #4 \n"
        "vst1.32 {q0}, [%[p]]! \n"
        "bgt 5b \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vsub.s32 d0, d2, d0 \n"
        "vst1.32 {d0}, [%[p]]! \n"
        "vld1.32 {d0}, [%[p]]! \n"
        "subs %[i], %[i], #1 \n"
        "pop {%[j]} \n"
        "bgt 4b \n"
        :
        : [p] "r" (p), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i), [j] "r" (j)
        : "memory", "q0", "q1"
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
        "vdup.s32 q1, %[ceil] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vmin.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [ceil] "r" (ceil), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "vdup.s32 q1, %[floor] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vmax.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [floor] "r" (floor), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "vdup.s32 q1, %[shift] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vrshl.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [shift] "r" (-shift), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "vdup.s32 q1, %[v] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in]]! \n"
        "vadd.s32 q0, q0, q1 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [v] "r" (v), [i] "r" (i)
        : "memory", "q0", "q1"
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
        "vld1.32 {q0}, [%[in]]! \n"
        "vld1.32 {q1}, [%[in]]! \n"
        "vcvt.f32.u32 q0, q0, #15 \n"
        "vcvt.f32.u32 q1, q1, #15 \n"
        "vld1.32 {q2}, [%[in]]! \n"
        "vld1.32 {q3}, [%[in]]! \n"
        "vrsqrte.f32 q0, q0 \n"
        "vrsqrte.f32 q1, q1 \n"
        "vcvt.f32.u32 q2, q2, #15 \n"
        "vcvt.f32.u32 q3, q3, #15 \n"
        "vrsqrte.f32 q2, q2 \n"
        "vrsqrte.f32 q3, q3 \n"
        "vcvt.u32.f32 q0, q0, #15 \n"
        "vcvt.u32.f32 q1, q1, #15 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "vst1.32 {q1}, [%[o]]! \n"
        "vcvt.u32.f32 q2, q2, #15 \n"
        "vcvt.u32.f32 q3, q3, #15 \n"
        "subs %[i], %[i], #16 \n"
        "vst1.32 {q2}, [%[o]]! \n"
        "vst1.32 {q3}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in] "r" (in), [i] "r" (i)
        : "memory", "q0", "q1", "q2", "q3"
    );

    out->q = 30 - x->q;

    return STP_OK;
}
#endif

#if STP_ComplexImageDotMulC_Optimized
stp_ret STP_ComplexImageDotMulC(stp_complex_image *out,
        const stp_complex_image *x, const stp_complex_image *y) {
    stp_int32 i;
    stp_int32 q;
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
            "vdup.s32 q12, %[q] \n"

            "1: \n"
            "vld2.s32 {q0, q1}, [%[in0]]! \n"
            "vld2.s32 {q2, q3}, [%[in2]]! \n"
            "vmull.s32 q8, d0, d4 \n"
            "vmull.s32 q9, d2, d6 \n"
            "vld2.s32 {q6, q7}, [%[in3]]! \n"
            "vld2.s32 {q4, q5}, [%[in1]]! \n"
            "vmull.s32 q10, d0, d12 \n"
            "vmull.s32 q11, d2, d14 \n"
            "vmlsl.s32 q8, d8, d12 \n"
            "vmlsl.s32 q9, d10, d14 \n"
            "vmlal.s32 q10, d4, d8 \n"
            "vmlal.s32 q11, d6, d10 \n"
            "vrshl.s64 q8, q8, q12 \n"
            "vrshl.s64 q9, q9, q12 \n"
            "vrshl.s64 q10, q10, q12 \n"
            "vrshl.s64 q11, q11, q12 \n"
            "vtrn.32 q8, q9 \n"
            "vtrn.32 q10, q11 \n"
            "vst1.32 {q8}, [%[o0]]! \n"
            "vst1.32 {q10}, [%[o1]]! \n"

            "vmull.s32 q8, d1, d5 \n"
            "vmull.s32 q9, d3, d7 \n"
            "vmull.s32 q10, d1, d13 \n"
            "vmull.s32 q11, d3, d15 \n"
            "vmlsl.s32 q8, d9, d13 \n"
            "vmlsl.s32 q9, d11, d15 \n"
            "vmlal.s32 q10, d5, d9 \n"
            "vmlal.s32 q11, d7, d11 \n"
            "vrshl.s64 q8, q8, q12 \n"
            "vrshl.s64 q9, q9, q12 \n"
            "vrshl.s64 q10, q10, q12 \n"
            "vrshl.s64 q11, q11, q12 \n"
            "vtrn.32 q8, q9 \n"
            "vtrn.32 q10, q11 \n"
            "subs %[i], %[i], #8 \n"
            "vst1.32 {q8}, [%[o0]]! \n"
            "vst1.32 {q10}, [%[o1]]! \n"
            "bgt 1b \n"
            :
            : [o0] "r" (o0), [o1] "r" (o1), [in0] "r" (in0), [in1] "r" (in1), [in2] "r" (in2), [in3] "r" (in3), [i] "r" (i), [q] "r" (q)
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12"
        );
    } else {
        asm volatile (
            "vdup.s32 q12, %[q] \n"

            "1: \n"
            "vld2.s32 {q0, q1}, [%[in0]]! \n"
            "vld2.s32 {q2, q3}, [%[in2]]! \n"
            "vmull.s32 q8, d0, d4 \n"
            "vmull.s32 q9, d2, d6 \n"
            "vld2.s32 {q6, q7}, [%[in3]]! \n"
            "vld2.s32 {q4, q5}, [%[in1]]! \n"
            "vmull.s32 q10, d0, d12 \n"
            "vmull.s32 q11, d2, d14 \n"
            "vmlal.s32 q8, d8, d12 \n"
            "vmlal.s32 q9, d10, d14 \n"
            "vmlsl.s32 q10, d4, d8 \n"
            "vmlsl.s32 q11, d6, d10 \n"
            "vrshl.s64 q8, q8, q12 \n"
            "vrshl.s64 q9, q9, q12 \n"
            "vrshl.s64 q10, q10, q12 \n"
            "vrshl.s64 q11, q11, q12 \n"
            "vtrn.32 q8, q9 \n"
            "vtrn.32 q10, q11 \n"
            "vst1.32 {q8}, [%[o0]]! \n"
            "vst1.32 {q10}, [%[o1]]! \n"

            "vmull.s32 q8, d1, d5 \n"
            "vmull.s32 q9, d3, d7 \n"
            "vmull.s32 q10, d1, d13 \n"
            "vmull.s32 q11, d3, d15 \n"
            "vmlal.s32 q8, d9, d13 \n"
            "vmlal.s32 q9, d11, d15 \n"
            "vmlsl.s32 q10, d5, d9 \n"
            "vmlsl.s32 q11, d7, d11 \n"
            "vrshl.s64 q8, q8, q12 \n"
            "vrshl.s64 q9, q9, q12 \n"
            "vrshl.s64 q10, q10, q12 \n"
            "vrshl.s64 q11, q11, q12 \n"
            "vtrn.32 q8, q9 \n"
            "vtrn.32 q10, q11 \n"
            "subs %[i], %[i], #8 \n"
            "vst1.32 {q8}, [%[o0]]! \n"
            "vst1.32 {q10}, [%[o1]]! \n"
            "bgt 1b \n"
            :
            : [o0] "r" (o0), [o1] "r" (o1), [in0] "r" (in0), [in1] "r" (in1), [in2] "r" (in2), [in3] "r" (in3), [i] "r" (i), [q] "r" (q)
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12"
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
        "vmov.s32 q4, #1 \n"
        "1: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vabs.s32 q2, q0 \n"
        "vabs.s32 q3, q1 \n"
        "vadd.s32 q0, q1 \n"
        "vmax.s32 q2, q3 \n"
        "vclt.s32 q0, #0 \n"
        "vadd.s32 q0, q0 \n"
        "vadd.s32 q0, q4 \n"
        "vmul.s32 q0, q0, q2 \n"
        "subs %[i], %[i], #4 \n"
        "vst1.32 {q0}, [%[o]]! \n"
        "bgt 1b \n"
        :
        : [o] "r" (o), [in0] "r" (in0), [in1] "r" (in1), [i] "r" (i)
        : "memory", "q0", "q1", "q2", "q3", "q4"
    );

    out->q = x->q;

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

    stp_uint32 params[7];
    params[0] = (stp_uint32)j;
    params[1] = (stp_uint32)i;
    params[2] = (stp_uint32)p;
    params[3] = (stp_uint32)y;
    params[4] = (stp_uint32)x;
    params[5] = (stp_uint32)m_table;
    params[6] = (stp_uint32)o_table;

    asm volatile (
        "push {r0-r12} \n"
        "mov r7, %[params] \n"
        "ldr r6, [r7] \n"
        "ldr r5, [r7, #4] \n"
        "ldr r4, [r7, #8] \n"
        "ldr r3, [r7, #12] \n"
        "ldr r2, [r7, #16] \n"
        "ldr r1, [r7, #20] \n"
        "ldr r0, [r7, #24] \n"
        "vmov.s32 q5, #4 \n"
        "mov r7, #4 \n"
        "vmov.s32 d12[0], r7 \n"
        "mov r7, #3 \n"
        "vmov.s32 d12[1], r7 \n"
        "mov r7, #2 \n"
        "vmov.s32 d13[0], r7 \n"
        "mov r7, #1 \n"
        "vmov.s32 d13[1], r7 \n"
        "vsub.s32 q7, q5, q6 \n"
        "vdup.s32 q10, r6 \n"

        "vmov.s32 q8, #4 \n"
        "vmov.s32 q9, #0 \n"

        "1: \n"
        "push {r5} \n"
        "mov r6, #4 \n"

        "2: \n"
        "push {r6} \n"
        "vmov.32 r7, d20[0] \n"

        "3: \n"
        "push {r7} \n"

        "vld1.32 {q0}, [r3]! \n"
        "vld1.32 {q1}, [r2]! \n"
        "vshl.u32 q0, q0, #10 \n"
        "vshl.u32 q1, q1, #2 \n"
        "vadd.u32 q0, q1 \n"
        "vmov r5, r6, d0 \n"
        "vmov r7, r8, d1 \n"
        "add r1, r5 \n"
        "add r0, r5 \n"
        "vld1.32 {d0[0]}, [r1] \n"
        "vld1.32 {d2[0]}, [r0] \n"
        "sub r1, r5 \n"
        "sub r0, r5 \n"
        "add r1, r6 \n"
        "add r0, r6 \n"
        "vld1.32 {d0[1]}, [r1] \n"
        "vld1.32 {d2[1]}, [r0] \n"
        "sub r1, r6 \n"
        "sub r0, r6 \n"
        "add r1, r7 \n"
        "add r0, r7 \n"
        "vld1.32 {d1[0]}, [r1] \n"
        "vld1.32 {d3[0]}, [r0] \n"
        "sub r1, r7 \n"
        "sub r0, r7 \n"
        "add r1, r8 \n"
        "add r0, r8 \n"
        "vld1.32 {d1[1]}, [r1] \n"
        "vld1.32 {d3[1]}, [r0] \n"
        "sub r1, r8 \n"
        "sub r0, r8 \n"


        "vmov.s32 q5, #0x1f \n"
        "vand.i32 q3, q1, q5 \n"
        "vmov.s32 q5, #4 \n"
        "vshr.s32 q1, q1, #3 \n"
        "vadd.s32 q1, q5 \n"
        "vmov.s32 q5, #32 \n"
        "vbic.i32 q1, #0x3 \n"
        "vsub.s32 q4, q5, q3 \n"
        "vmov r5, r6, d2 \n"
        "vmov r7, r8, d3 \n"
        "ldr r9, [r4, r5] \n"
        "ldr r10, [r4, r6] \n"
        "ldr r11, [r4, r7] \n"
        "ldr r12, [r4, r8] \n"

        "vmul.s32 q1, q0, q4 \n"
        "vmul.s32 q0, q0, q3 \n"

        "sub r5, #4 \n"
        "sub r6, #4 \n"
        "sub r7, #4 \n"
        "sub r8, #4 \n"
        "ldr r5, [r4, r5] \n"
        "ldr r6, [r4, r6] \n"
        "ldr r7, [r4, r7] \n"
        "ldr r8, [r4, r8] \n"

        "vmul.s32 q4, q1, q6 \n"
        "vmul.s32 q3, q0, q6 \n"
        "vmul.s32 q4, q4, q8 \n"
        "vmul.s32 q3, q3, q8 \n"
        "vld1.32 {d4[0]}, [r5] \n"
        "vld1.32 {d4[1]}, [r9] \n"
        "vtrn.32 q4, q3 \n"
        "vadd.s32 d4, d8 \n"
        "vst1.32 {d4[0]}, [r5]! \n"
        "vst1.32 {d4[1]}, [r9]! \n"
        "vld1.32 {d5[0]}, [r6] \n"
        "vld1.32 {d5[1]}, [r10] \n"
        "vadd.s32 d5, d6 \n"
        "vst1.32 {d5[0]}, [r6]! \n"
        "vst1.32 {d5[1]}, [r10]! \n"
        "vld1.32 {d4[0]}, [r7] \n"
        "vld1.32 {d4[1]}, [r11] \n"
        "vadd.s32 d4, d9 \n"
        "vst1.32 {d4[0]}, [r7]! \n"
        "vst1.32 {d4[1]}, [r11]! \n"
        "vld1.32 {d5[0]}, [r8] \n"
        "vld1.32 {d5[1]}, [r12] \n"
        "vadd.s32 d5, d7 \n"
        "vst1.32 {d5[0]}, [r8]! \n"
        "vst1.32 {d5[1]}, [r12]! \n"

        "vmul.s32 q4, q1, q7 \n"
        "vmul.s32 q3, q0, q7 \n"
        "vmul.s32 q4, q4, q8 \n"
        "vmul.s32 q3, q3, q8 \n"
        "vld1.32 {d4[0]}, [r5] \n"
        "vld1.32 {d4[1]}, [r9] \n"
        "vtrn.32 q4, q3 \n"
        "vadd.s32 d4, d8 \n"
        "vst1.32 {d4[0]}, [r5] \n"
        "vst1.32 {d4[1]}, [r9] \n"
        "vld1.32 {d5[0]}, [r6] \n"
        "vld1.32 {d5[1]}, [r10] \n"
        "vadd.s32 d5, d6 \n"
        "vst1.32 {d5[0]}, [r6] \n"
        "vst1.32 {d5[1]}, [r10] \n"
        "vld1.32 {d4[0]}, [r7] \n"
        "vld1.32 {d4[1]}, [r11] \n"
        "vadd.s32 d4, d9 \n"
        "vst1.32 {d4[0]}, [r7] \n"
        "vst1.32 {d4[1]}, [r11] \n"
        "vld1.32 {d5[0]}, [r8] \n"
        "vld1.32 {d5[1]}, [r12] \n"
        "vadd.s32 d5, d7 \n"
        "vst1.32 {d5[0]}, [r8] \n"
        "vst1.32 {d5[1]}, [r12] \n"


        "vmov d4, r5, r6 \n"
        "vmov d5, r7, r8 \n"
        "vadd.u32 q2, q10 \n"
        "vmov r5, r6, d4 \n"
        "vmov r7, r8, d5 \n"
        "vmov d4, r9, r10 \n"
        "vmov d5, r11, r12 \n"
        "vadd.u32 q2, q10 \n"
        "vmov r9, r10, d4 \n"
        "vmov r11, r12, d5 \n"


        "vmul.s32 q4, q1, q6 \n"
        "vmul.s32 q3, q0, q6 \n"
        "vmul.s32 q4, q4, q9 \n"
        "vmul.s32 q3, q3, q9 \n"
        "vld1.32 {d4[0]}, [r5] \n"
        "vld1.32 {d4[1]}, [r9] \n"
        "vtrn.32 q4, q3 \n"
        "vadd.s32 d4, d8 \n"
        "vst1.32 {d4[0]}, [r5]! \n"
        "vst1.32 {d4[1]}, [r9]! \n"
        "vld1.32 {d5[0]}, [r6] \n"
        "vld1.32 {d5[1]}, [r10] \n"
        "vadd.s32 d5, d6 \n"
        "vst1.32 {d5[0]}, [r6]! \n"
        "vst1.32 {d5[1]}, [r10]! \n"
        "vld1.32 {d4[0]}, [r7] \n"
        "vld1.32 {d4[1]}, [r11] \n"
        "vadd.s32 d4, d9 \n"
        "vst1.32 {d4[0]}, [r7]! \n"
        "vst1.32 {d4[1]}, [r11]! \n"
        "vld1.32 {d5[0]}, [r8] \n"
        "vld1.32 {d5[1]}, [r12] \n"
        "vadd.s32 d5, d7 \n"
        "vst1.32 {d5[0]}, [r8]! \n"
        "vst1.32 {d5[1]}, [r12]! \n"

        "vmul.s32 q4, q1, q7 \n"
        "vmul.s32 q3, q0, q7 \n"
        "vmul.s32 q4, q4, q9 \n"
        "vmul.s32 q3, q3, q9 \n"
        "vld1.32 {d4[0]}, [r5] \n"
        "vld1.32 {d4[1]}, [r9] \n"
        "vtrn.32 q4, q3 \n"
        "vadd.s32 d4, d8 \n"
        "vst1.32 {d4[0]}, [r5] \n"
        "vst1.32 {d4[1]}, [r9] \n"
        "vld1.32 {d5[0]}, [r6] \n"
        "vld1.32 {d5[1]}, [r10] \n"
        "vadd.s32 d5, d6 \n"
        "vst1.32 {d5[0]}, [r6] \n"
        "vst1.32 {d5[1]}, [r10] \n"
        "vld1.32 {d4[0]}, [r7] \n"
        "vld1.32 {d4[1]}, [r11] \n"
        "vadd.s32 d4, d9 \n"
        "vst1.32 {d4[0]}, [r7] \n"
        "vst1.32 {d4[1]}, [r11] \n"
        "vld1.32 {d5[0]}, [r8] \n"
        "vld1.32 {d5[1]}, [r12] \n"
        "vadd.s32 d5, d7 \n"
        "vst1.32 {d5[0]}, [r8] \n"
        "vst1.32 {d5[1]}, [r12] \n"


        "sub r4, #4 \n"
        "vmov.s32 q5, #4 \n"
        "vld1.32 {q0}, [r4]! \n"
        "vld1.32 {q1}, [r4]! \n"
        "vld1.32 {q2}, [r4]! \n"
        "vld1.32 {q3}, [r4]! \n"
        "vld1.32 {q4}, [r4]! \n"

        "sub r4, #80 \n"
        "vadd.u32 q0, q5 \n"
        "vadd.u32 q1, q5 \n"
        "vadd.u32 q2, q5 \n"
        "vadd.u32 q3, q5 \n"
        "vadd.u32 q4, q5 \n"
        "vst1.32 {q0}, [r4]! \n"
        "vst1.32 {q1}, [r4]! \n"
        "vst1.32 {q2}, [r4]! \n"
        "vst1.32 {q3}, [r4]! \n"
        "vst1.32 {q4}, [r4]! \n"
        "sub r4, #76 \n"
        "pop {r7} \n"
        "subs r7, #4 \n"
        "bgt 3b \n"

        "sub r4, #4 \n"
        "vld1.32 {q0}, [r4]! \n"
        "vld1.32 {q1}, [r4]! \n"
        "vld1.32 {q2}, [r4]! \n"
        "vld1.32 {q3}, [r4]! \n"
        "vld1.32 {q4}, [r4]! \n"

        "sub r4, #80 \n"
        "vsub.u32 q0, q10 \n"
        "vsub.u32 q1, q10 \n"
        "vsub.u32 q2, q10 \n"
        "vsub.u32 q3, q10 \n"
        "vsub.u32 q4, q10 \n"
        "vst1.32 {q0}, [r4]! \n"
        "vst1.32 {q1}, [r4]! \n"
        "vst1.32 {q2}, [r4]! \n"
        "vst1.32 {q3}, [r4]! \n"
        "vst1.32 {q4}, [r4]! \n"
        "sub r4, #76 \n"

        "vld1.32 {q0}, [r3]! \n"
        "vld1.32 {q1}, [r2]! \n"
        "vmov.s32 q5, #1 \n"
        "vadd.s32 q9, q5 \n"
        "vmov.s32 q5, #4 \n"
        "vbic.i32 q9, #0x0c \n"
        "vsub.s32 q8, q5, q9 \n"

        "pop {r6} \n"
        "subs r6, #1 \n"
        "bgt 2b \n"

        "sub r4, #4 \n"
        "vmov.s32 q5, #4 \n"
        "vld1.32 {q0}, [r4]! \n"
        "vld1.32 {q1}, [r4]! \n"
        "vld1.32 {q2}, [r4]! \n"
        "vld1.32 {q3}, [r4]! \n"
        "vld1.32 {q4}, [r4]! \n"
        "vadd.s32 q5, q10 \n"

        "sub r4, #80 \n"
        "vadd.u32 q0, q5 \n"
        "vadd.u32 q1, q5 \n"
        "vadd.u32 q2, q5 \n"
        "vadd.u32 q3, q5 \n"
        "vadd.u32 q4, q5 \n"
        "vst1.32 {q0}, [r4]! \n"
        "vst1.32 {q1}, [r4]! \n"
        "vst1.32 {q2}, [r4]! \n"
        "vst1.32 {q3}, [r4]! \n"
        "vst1.32 {q4}, [r4]! \n"
        "sub r4, #76 \n"

        "pop {r5} \n"
        "subs r5, #4 \n"
        "bgt 1b \n"

        "pop {r0-r12} \n"
        :
        : [params] "r" (params)
        : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10"
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
                            "vld1.32 {q0}, [%[i0]]! \n"
                            "vld1.32 {q1}, [%[i1]]! \n"
                            "vld1.32 {q2}, [%[i2]]! \n"
                            "vld1.32 {q3}, [%[i3]]! \n"
                            "vadd.s32 q4, q0, q1 \n"
                            "vsub.s32 q5, q0, q1 \n"
                            "vadd.s32 q0, q2, q3 \n"
                            "vsub.s32 q1, q2, q3 \n"
                            "vadd.s32 q2, q4, q0 \n"
                            "vadd.s32 q3, q5, q1 \n"
                            "vst1.32 {q2}, [%[o0]]! \n"
                            "vst1.32 {q3}, [%[o1]]! \n"
                            "vsub.s32 q0, q4, q0 \n"
                            "vsub.s32 q1, q5, q1 \n"
                            "subs %[s], %[s], #4 \n"
                            "vst1.32 {q0}, [%[o2]]! \n"
                            "vst1.32 {q1}, [%[o3]]! \n"
                            "bgt 1b \n"
                            : [i0] "+r" (i0), [i1] "+r" (i1), [i2] "+r" (i2), [i3] "+r" (i3), [o0] "+r" (o0), [o1] "+r" (o1), [o2] "+r" (o2), [o3] "+r" (o3)
                            : [s] "r" (s)
                            : "memory", "q0", "q1", "q2", "q3", "q4", "q5"
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
                "vld1.32 {q0}, [%[in]]! \n"
                "vld1.32 {q1}, [%[in]]! \n"
                "vcvt.s32.f32 q0, q0, #15 \n"
                "vcvt.s32.f32 q1, q1, #15 \n"
                "subs %[i], %[i], #8 \n"
                "vst1.32 {q0}, [%[o]]! \n"
                "vst1.32 {q1}, [%[o]]! \n"
                "bgt 1b \n"
                :
                : [o] "r" (o), [in] "r" (in), [i] "r" (i)
                : "memory", "q0", "q1"
            );
            out_image->q = in_image->q + 15;
            out_image->data_type = STP_INT32;
        } else {
            float *o = (float*)out_image->p_mem;
            stp_int32 *in = (stp_int32*)in_image->p_mem;

            asm volatile (
                "1: \n"
                "vld1.32 {q0}, [%[in]]! \n"
                "vld1.32 {q1}, [%[in]]! \n"
                "vcvt.f32.s32 q0, q0, #15 \n"
                "vcvt.f32.s32 q1, q1, #15 \n"
                "subs %[i], %[i], #8 \n"
                "vst1.32 {q0}, [%[o]]! \n"
                "vst1.32 {q1}, [%[o]]! \n"
                "bgt 1b \n"
                :
                : [o] "r" (o), [in] "r" (in), [i] "r" (i)
                : "memory", "q0", "q1"
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
        "vdup.s32 q4, %[c0] \n"
        "vdup.s32 q5, %[c1] \n"
        "1: \n"
        "vld1.32 {q0}, [%[in0]]! \n"
        "vld1.32 {q1}, [%[in1]]! \n"
        "vmul.f32 q0, q4 \n"
        "vld1.32 {q2}, [%[in0]]! \n"
        "vld1.32 {q3}, [%[in1]]! \n"
        "vmul.f32 q2, q4 \n"
        "vmla.f32 q0, q1, q5 \n"
        "vmla.f32 q2, q3, q5 \n"
        "subs %[i], %[i], #8 \n"
        "vst1.32 {q0}, [%[out]]! \n"
        "vst1.32 {q2}, [%[out]]! \n"
        "bgt 1b \n"
        :
        : [out] "r" (out), [in0] "r" (in0), [in1] "r" (in1), [c0] "r" (coefs[0]), [c1] "r" (coefs[1]), [i] "r" (i)
        : "memory", "q0", "q1", "q2", "q3", "q4", "q5"
    );

    out_image->data_type = STP_FLOAT32;
    out_image->q = in_image0->q;
    return STP_OK;
}
#endif

