/*
 * stp_fft.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "stp_fft.h"
#include "math.h"

#if !STP_FFT1D32PointRealInCore_Optimized
static inline stp_ret STP_FFT1D32PointRealInCore(stp_complex_image *out, stp_complex_image* temp,
        stp_complex_image* table[], stp_image *x) {
    stp_int32 i;
    stp_int32 *p0, *p1, *o0, *o1, temp0[4], temp1[4];
    stp_int32 *t0, *t1;

    /* first layer */
    o0 = (stp_int32*)temp->real->p_mem;
    o1 = (stp_int32*)temp->imag->p_mem;
    p0 = (stp_int32*)x->p_mem;
    t0 = (stp_int32*)table[4]->real->p_mem;
    t1 = (stp_int32*)table[4]->imag->p_mem;
    for (i = 0; i < 16; i += 4) {
        o0[i] = p0[i] + p0[i + 16];
        o0[i + 1] = p0[i + 1] + p0[i + 17];
        o0[i + 2] = p0[i + 2] + p0[i + 18];
        o0[i + 3] = p0[i + 3] + p0[i + 19];

        o0[i + 16] = p0[i] - p0[i + 16];
        o0[i + 17] = p0[i + 1] - p0[i + 17];
        o0[i + 18] = p0[i + 2] - p0[i + 18];
        o0[i + 19] = p0[i + 3] - p0[i + 19];

        o1[i + 16] = (stp_int64)o0[i + 16]*t1[i] >> 31;
        o1[i + 17] = (stp_int64)o0[i + 17]*t1[i + 1] >> 31;
        o1[i + 18] = (stp_int64)o0[i + 18]*t1[i + 2] >> 31;
        o1[i + 19] = (stp_int64)o0[i + 19]*t1[i + 3] >> 31;

        o0[i + 16] = (stp_int64)o0[i + 16]*t0[i] >> 31;
        o0[i + 17] = (stp_int64)o0[i + 17]*t0[i + 1] >> 31;
        o0[i + 18] = (stp_int64)o0[i + 18]*t0[i + 2] >> 31;
        o0[i + 19] = (stp_int64)o0[i + 19]*t0[i + 3] >> 31;
    }

    /* second layer */
    p0 = (stp_int32*)out->real->p_mem;
    p1 = (stp_int32*)out->imag->p_mem;
    t0 = (stp_int32*)table[3]->real->p_mem;
    t1 = (stp_int32*)table[3]->imag->p_mem;
    for (i = 0; i < 8; i += 4) {
        p0[i] = o0[i] + o0[i + 8];
        p0[i + 1] = o0[i + 1] + o0[i + 9];
        p0[i + 2] = o0[i + 2] + o0[i + 10];
        p0[i + 3] = o0[i + 3] + o0[i + 11];

        p0[i + 8] = o0[i] - o0[i + 8];
        p0[i + 9] = o0[i + 1] - o0[i + 9];
        p0[i + 10] = o0[i + 2] - o0[i + 10];
        p0[i + 11] = o0[i + 3] - o0[i + 11];

        p1[i + 8] = (stp_int64)p0[i + 8]*t1[i] >> 31;
        p1[i + 9] = (stp_int64)p0[i + 9]*t1[i + 1] >> 31;
        p1[i + 10] = (stp_int64)p0[i + 10]*t1[i + 2] >> 31;
        p1[i + 11] = (stp_int64)p0[i + 11]*t1[i + 3] >> 31;

        p0[i + 8] = (stp_int64)p0[i + 8]*t0[i] >> 31;
        p0[i + 9] = (stp_int64)p0[i + 9]*t0[i + 1] >> 31;
        p0[i + 10] = (stp_int64)p0[i + 10]*t0[i + 2] >> 31;
        p0[i + 11] = (stp_int64)p0[i + 11]*t0[i + 3] >> 31;

        p0[i + 16] = o0[i + 16] + o0[i + 24];
        p0[i + 17] = o0[i + 17] + o0[i + 25];
        p0[i + 18] = o0[i + 18] + o0[i + 26];
        p0[i + 19] = o0[i + 19] + o0[i + 27];

        p1[i + 16] = o1[i + 16] + o1[i + 24];
        p1[i + 17] = o1[i + 17] + o1[i + 25];
        p1[i + 18] = o1[i + 18] + o1[i + 26];
        p1[i + 19] = o1[i + 19] + o1[i + 27];

        temp0[0] = o0[i + 16] - o0[i + 24];
        temp0[1] = o0[i + 17] - o0[i + 25];
        temp0[2] = o0[i + 18] - o0[i + 26];
        temp0[3] = o0[i + 19] - o0[i + 27];

        temp1[0] = o1[i + 16] - o1[i + 24];
        temp1[1] = o1[i + 17] - o1[i + 25];
        temp1[2] = o1[i + 18] - o1[i + 26];
        temp1[3] = o1[i + 19] - o1[i + 27];

        p0[i + 24] = ((stp_int64)temp0[0]*t0[i] - (stp_int64)temp1[0]*t1[i]) >> 31;
        p0[i + 25] = ((stp_int64)temp0[1]*t0[i + 1] - (stp_int64)temp1[1]*t1[i + 1]) >> 31;
        p0[i + 26] = ((stp_int64)temp0[2]*t0[i + 2] - (stp_int64)temp1[2]*t1[i + 2]) >> 31;
        p0[i + 27] = ((stp_int64)temp0[3]*t0[i + 3] - (stp_int64)temp1[3]*t1[i + 3]) >> 31;

        p1[i + 24] = ((stp_int64)temp0[0]*t1[i] + (stp_int64)temp1[0]*t0[i]) >> 31;
        p1[i + 25] = ((stp_int64)temp0[1]*t1[i + 1] + (stp_int64)temp1[1]*t0[i + 1]) >> 31;
        p1[i + 26] = ((stp_int64)temp0[2]*t1[i + 2] + (stp_int64)temp1[2]*t0[i + 2]) >> 31;
        p1[i + 27] = ((stp_int64)temp0[3]*t1[i + 3] + (stp_int64)temp1[3]*t0[i + 3]) >> 31;
    }

    p1[0] = 0;
    p1[1] = 0;
    p1[2] = 0;
    p1[3] = 0;
    p1[4] = 0;
    p1[5] = 0;
    p1[6] = 0;
    p1[7] = 0;

    /* third layer */
    t0 = (stp_int32*)table[2]->real->p_mem;
    t1 = (stp_int32*)table[2]->imag->p_mem;
    for (i = 0; i < 32; i += 8) {
        o0[i] = p0[i] + p0[i + 4];
        o0[i + 1] = p0[i + 1] + p0[i + 5];
        o0[i + 2] = p0[i + 2] + p0[i + 6];
        o0[i + 3] = p0[i + 3] + p0[i + 7];

        o1[i] = p1[i] + p1[i + 4];
        o1[i + 1] = p1[i + 1] + p1[i + 5];
        o1[i + 2] = p1[i + 2] + p1[i + 6];
        o1[i + 3] = p1[i + 3] + p1[i + 7];

        temp0[0] = p0[i] - p0[i + 4];
        temp0[1] = p0[i + 1] - p0[i + 5];
        temp0[2] = p0[i + 2] - p0[i + 6];
        temp0[3] = p0[i + 3] - p0[i + 7];

        temp1[0] = p1[i] - p1[i + 4];
        temp1[1] = p1[i + 1] - p1[i + 5];
        temp1[2] = p1[i + 2] - p1[i + 6];
        temp1[3] = p1[i + 3] - p1[i + 7];

        o0[i + 4] = ((stp_int64)temp0[0]*t0[0] - (stp_int64)temp1[0]*t1[0]) >> 31;
        o0[i + 5] = ((stp_int64)temp0[1]*t0[1] - (stp_int64)temp1[1]*t1[1]) >> 31;
        o0[i + 6] = ((stp_int64)temp0[2]*t0[2] - (stp_int64)temp1[2]*t1[2]) >> 31;
        o0[i + 7] = ((stp_int64)temp0[3]*t0[3] - (stp_int64)temp1[3]*t1[3]) >> 31;

        o1[i + 4] = ((stp_int64)temp0[0]*t1[0] + (stp_int64)temp1[0]*t0[0]) >> 31;
        o1[i + 5] = ((stp_int64)temp0[1]*t1[1] + (stp_int64)temp1[1]*t0[1]) >> 31;
        o1[i + 6] = ((stp_int64)temp0[2]*t1[2] + (stp_int64)temp1[2]*t0[2]) >> 31;
        o1[i + 7] = ((stp_int64)temp0[3]*t1[3] + (stp_int64)temp1[3]*t0[3]) >> 31;
    }

    /* last 2 layers, radix 4 */
    p0 = (stp_int32*)out->real->p_mem;
    p1 = (stp_int32*)out->imag->p_mem;
    for (i = 0; i < 32; i += 4) {
        p0[i] = o0[i] + o0[i + 1] + o0[i + 2] + o0[i + 3];
        p0[i + 1] = o0[i] - o0[i + 1] + o0[i + 2] - o0[i + 3];
        p0[i + 2] = o0[i] + o1[i + 1] - o0[i + 2] - o1[i + 3];
        p0[i + 3] = o0[i] - o1[i + 1] - o0[i + 2] + o1[i + 3];

        p1[i] = o1[i] + o1[i + 1] + o1[i + 2] + o1[i + 3];
        p1[i + 1] = o1[i] - o1[i + 1] + o1[i + 2] - o1[i + 3];
        p1[i + 2] = o1[i] - o0[i + 1] - o1[i + 2] + o0[i + 3];
        p1[i + 3] = o1[i] + o0[i + 1] - o1[i + 2] - o0[i + 3];
    }

    out->real->q = x->q;
    out->imag->q = x->q;
    out->sign = 1;

    return STP_OK;
}
#else
stp_ret STP_FFT1D32PointRealInCore(stp_complex_image *out, stp_complex_image* temp,
        stp_complex_image* table[], stp_image *x);
#endif

static inline stp_ret STP_FFT1D32PointComplexInCore(stp_complex_image *out, stp_complex_image* temp,
        stp_complex_image* table[], stp_complex_image *x) {
    stp_int32 i;
    stp_int32 *p0, *p1, *o0, *o1;
    stp_int32 *t0, *t1;

    /* first 2 layers, radix 4 */
    p0 = (stp_int32*)temp->real->p_mem;
    p1 = (stp_int32*)temp->imag->p_mem;
    o0 = (stp_int32*)x->real->p_mem;
    o1 = (stp_int32*)x->imag->p_mem;
    for (i = 0; i < 32; i += 4) {
        p0[i] = o0[i] + o0[i + 1] + o0[i + 2] + o0[i + 3];
        p0[i + 1] = o0[i] - o0[i + 1] - o1[i + 2] + o1[i + 3];
        p0[i + 2] = o0[i] + o0[i + 1] - o0[i + 2] - o0[i + 3];
        p0[i + 3] = o0[i] - o0[i + 1] + o1[i + 2] - o1[i + 3];

        p1[i] = o1[i] + o1[i + 1] + o1[i + 2] + o1[i + 3];
        p1[i + 1] = o1[i] - o1[i + 1] + o0[i + 2] - o0[i + 3];
        p1[i + 2] = o1[i] + o1[i + 1] - o1[i + 2] - o1[i + 3];
        p1[i + 3] = o1[i] - o1[i + 1] - o0[i + 2] + o0[i + 3];
    }

    /* third layer */
    t0 = (stp_int32*)table[2]->real->p_mem;
    t1 = (stp_int32*)table[2]->imag->p_mem;
    o0 = (stp_int32*)out->real->p_mem;
    o1 = (stp_int32*)out->imag->p_mem;
    for (i = 0; i < 32; i += 8) {
        o0[i + 4] = ((stp_int64)p0[i + 4]*t0[0] - (stp_int64)p1[i + 4]*t1[0]) >> 31;
        o0[i + 5] = ((stp_int64)p0[i + 5]*t0[1] - (stp_int64)p1[i + 5]*t1[1]) >> 31;
        o0[i + 6] = ((stp_int64)p0[i + 6]*t0[2] - (stp_int64)p1[i + 6]*t1[2]) >> 31;
        o0[i + 7] = ((stp_int64)p0[i + 7]*t0[3] - (stp_int64)p1[i + 7]*t1[3]) >> 31;

        o1[i + 4] = ((stp_int64)p0[i + 4]*t1[0] + (stp_int64)p1[i + 4]*t0[0]) >> 31;
        o1[i + 5] = ((stp_int64)p0[i + 5]*t1[1] + (stp_int64)p1[i + 5]*t0[1]) >> 31;
        o1[i + 6] = ((stp_int64)p0[i + 6]*t1[2] + (stp_int64)p1[i + 6]*t0[2]) >> 31;
        o1[i + 7] = ((stp_int64)p0[i + 7]*t1[3] + (stp_int64)p1[i + 7]*t0[3]) >> 31;

        o0[i] = p0[i] + o0[i + 4];
        o0[i + 1] = p0[i + 1] + o0[i + 5];
        o0[i + 2] = p0[i + 2] + o0[i + 6];
        o0[i + 3] = p0[i + 3] + o0[i + 7];

        o1[i] = p1[i] + o1[i + 4];
        o1[i + 1] = p1[i + 1] + o1[i + 5];
        o1[i + 2] = p1[i + 2] + o1[i + 6];
        o1[i + 3] = p1[i + 3] + o1[i + 7];

        o0[i + 4] = p0[i] - o0[i + 4];
        o0[i + 5] = p0[i + 1] - o0[i + 5];
        o0[i + 6] = p0[i + 2] - o0[i + 6];
        o0[i + 7] = p0[i + 3] - o0[i + 7];

        o1[i + 4] = p1[i] - o1[i + 4];
        o1[i + 5] = p1[i + 1] - o1[i + 5];
        o1[i + 6] = p1[i + 2] - o1[i + 6];
        o1[i + 7] = p1[i + 3] - o1[i + 7];
    }

    /* fourth layer */
    t0 = (stp_int32*)table[3]->real->p_mem;
    t1 = (stp_int32*)table[3]->imag->p_mem;
    for (i = 0; i < 8; i += 4) {
        p0[i + 8] = ((stp_int64)o0[i + 8]*t0[i] - (stp_int64)o1[i + 8]*t1[i]) >> 31;
        p0[i + 9] = ((stp_int64)o0[i + 9]*t0[i + 1] - (stp_int64)o1[i + 9]*t1[i + 1]) >> 31;
        p0[i + 10] = ((stp_int64)o0[i + 10]*t0[i + 2] - (stp_int64)o1[i + 10]*t1[i + 2]) >> 31;
        p0[i + 11] = ((stp_int64)o0[i + 11]*t0[i + 3] - (stp_int64)o1[i + 11]*t1[i + 3]) >> 31;

        p1[i + 8] = ((stp_int64)o0[i + 8]*t1[i] + (stp_int64)o1[i + 8]*t0[i]) >> 31;
        p1[i + 9] = ((stp_int64)o0[i + 9]*t1[i + 1] + (stp_int64)o1[i + 9]*t0[i + 1]) >> 31;
        p1[i + 10] = ((stp_int64)o0[i + 10]*t1[i + 2] + (stp_int64)o1[i + 10]*t0[i + 2]) >> 31;
        p1[i + 11] = ((stp_int64)o0[i + 11]*t1[i + 3] + (stp_int64)o1[i + 11]*t0[i + 3]) >> 31;

        p0[i + 24] = ((stp_int64)o0[i + 24]*t0[i] - (stp_int64)o1[i + 24]*t1[i]) >> 31;
        p0[i + 25] = ((stp_int64)o0[i + 25]*t0[i + 1] - (stp_int64)o1[i + 25]*t1[i + 1]) >> 31;
        p0[i + 26] = ((stp_int64)o0[i + 26]*t0[i + 2] - (stp_int64)o1[i + 26]*t1[i + 2]) >> 31;
        p0[i + 27] = ((stp_int64)o0[i + 27]*t0[i + 3] - (stp_int64)o1[i + 27]*t1[i + 3]) >> 31;

        p1[i + 24] = ((stp_int64)o0[i + 24]*t1[i] + (stp_int64)o1[i + 24]*t0[i]) >> 31;
        p1[i + 25] = ((stp_int64)o0[i + 25]*t1[i + 1] + (stp_int64)o1[i + 25]*t0[i + 1]) >> 31;
        p1[i + 26] = ((stp_int64)o0[i + 26]*t1[i + 2] + (stp_int64)o1[i + 26]*t0[i + 2]) >> 31;
        p1[i + 27] = ((stp_int64)o0[i + 27]*t1[i + 3] + (stp_int64)o1[i + 27]*t0[i + 3]) >> 31;

        p0[i] = o0[i] + p0[i + 8];
        p0[i + 1] = o0[i + 1] + p0[i + 9];
        p0[i + 2] = o0[i + 2] + p0[i + 10];
        p0[i + 3] = o0[i + 3] + p0[i + 11];

        p1[i] = o1[i] + p1[i + 8];
        p1[i + 1] = o1[i + 1] + p1[i + 9];
        p1[i + 2] = o1[i + 2] + p1[i + 10];
        p1[i + 3] = o1[i + 3] + p1[i + 11];

        p0[i + 8] = o0[i] - p0[i + 8];
        p0[i + 9] = o0[i + 1] - p0[i + 9];
        p0[i + 10] = o0[i + 2] - p0[i + 10];
        p0[i + 11] = o0[i + 3] - p0[i + 11];

        p1[i + 8] = o1[i] - p1[i + 8];
        p1[i + 9] = o1[i + 1] - p1[i + 9];
        p1[i + 10] = o1[i + 2] - p1[i + 10];
        p1[i + 11] = o1[i + 3] - p1[i + 11];

        p0[i + 16] = o0[i + 16] + p0[i + 24];
        p0[i + 17] = o0[i + 17] + p0[i + 25];
        p0[i + 18] = o0[i + 18] + p0[i + 26];
        p0[i + 19] = o0[i + 19] + p0[i + 27];

        p1[i + 16] = o1[i + 16] + p1[i + 24];
        p1[i + 17] = o1[i + 17] + p1[i + 25];
        p1[i + 18] = o1[i + 18] + p1[i + 26];
        p1[i + 19] = o1[i + 19] + p1[i + 27];

        p0[i + 24] = o0[i + 16] - p0[i + 24];
        p0[i + 25] = o0[i + 17] - p0[i + 25];
        p0[i + 26] = o0[i + 18] - p0[i + 26];
        p0[i + 27] = o0[i + 19] - p0[i + 27];

        p1[i + 24] = o1[i + 16] - p1[i + 24];
        p1[i + 25] = o1[i + 17] - p1[i + 25];
        p1[i + 26] = o1[i + 18] - p1[i + 26];
        p1[i + 27] = o1[i + 19] - p1[i + 27];
    }

    /* last layer */
    t0 = (stp_int32*)table[4]->real->p_mem;
    t1 = (stp_int32*)table[4]->imag->p_mem;
    for (i = 0; i < 16; i += 4) {
        o0[i + 16] = ((stp_int64)p0[i + 16]*t0[i] - (stp_int64)p1[i + 16]*t1[i]) >> 31;
        o0[i + 17] = ((stp_int64)p0[i + 17]*t0[i + 1] - (stp_int64)p1[i + 17]*t1[i + 1]) >> 31;
        o0[i + 18] = ((stp_int64)p0[i + 18]*t0[i + 2] - (stp_int64)p1[i + 18]*t1[i + 2]) >> 31;
        o0[i + 19] = ((stp_int64)p0[i + 19]*t0[i + 3] - (stp_int64)p1[i + 19]*t1[i + 3]) >> 31;

        o0[i] = p0[i] + o0[i + 16];
        o0[i + 1] = p0[i + 1] + o0[i + 17];
        o0[i + 2] = p0[i + 2] + o0[i + 18];
        o0[i + 3] = p0[i + 3] + o0[i + 19];

        o0[i + 16] = p0[i] - o0[i + 16];
        o0[i + 17] = p0[i + 1] - o0[i + 17];
        o0[i + 18] = p0[i + 2] - o0[i + 18];
        o0[i + 19] = p0[i + 3] - o0[i + 19];
    }

    out->real->q = x->real->q + 5;
    out->imag->q = x->imag->q + 5;
    out->sign = 1;

    return STP_OK;
}

static inline stp_int16 STP_FFTBitReverse(stp_int16 x, stp_int16 n) {
    register int i, r = 0;
    for (i = 0; i < n; i++) {
        r |= ((x >> i) & 0x1) << (n - i - 1);
    }
    return r;
}

static stp_ret STP_FFT2DImageReorder(stp_image *out_image, const stp_image *in_image, stp_int16 *table) {
    stp_int32 *p_in, *p_out, i;

    p_in = (stp_int32*)in_image->p_mem;
    p_out = (stp_int32*)out_image->p_mem;

    for (i = 0; i < in_image->height*in_image->width; i+=4) {
        p_out[i] = p_in[table[i]];
        p_out[i + 1] = p_in[table[i + 1]];
        p_out[i + 2] = p_in[table[i + 2]];
        p_out[i + 3] = p_in[table[i + 3]];
    }

    out_image->q = in_image->q;

    return STP_OK;
}

static stp_ret STP_FFT2DConvertToSquareImage(stp_complex_image *out[], const stp_complex_image *x) {
    stp_int32 i, j;
    stp_int32 w, h;
    stp_int32 ratio;

    w = x->real->width;
    h = x->real->height;

    if (w > h) {
        ratio = w/h;

        if (ratio == 2) {
            stp_int32 *p00, *p01, *p10, *p11;
            stp_int32 *d00, *d01, *d10, *d11;

            p00 = (stp_int32*)x->real->p_mem;
            p10 = (stp_int32*)x->imag->p_mem;
            p01 = p00 + h;
            p11 = p10 + h;
            d00 = (stp_int32*)out[0]->real->p_mem;
            d10 = (stp_int32*)out[0]->imag->p_mem;
            d01 = (stp_int32*)out[1]->real->p_mem;
            d11 = (stp_int32*)out[1]->imag->p_mem;

            out[0]->sign = x->sign;
            out[0]->real->q = x->real->q;
            out[0]->imag->q = x->imag->q;
            out[1]->sign = x->sign;
            out[1]->real->q = x->real->q;
            out[1]->imag->q = x->imag->q;

            i = h;
            while (i--) {
                j = h >> 2;
                while (j--) {
                    d00[0] = p00[0] + p01[0];
                    d00[1] = p00[1] + p01[1];
                    d00[2] = p00[2] + p01[2];
                    d00[3] = p00[3] + p01[3];

                    d10[0] = p10[0] + p11[0];
                    d10[1] = p10[1] + p11[1];
                    d10[2] = p10[2] + p11[2];
                    d10[3] = p10[3] + p11[3];

                    d01[0] = p00[0] - p01[0];
                    d01[1] = p00[1] - p01[1];
                    d01[2] = p00[2] - p01[2];
                    d01[3] = p00[3] - p01[3];

                    d11[0] = p10[0] - p11[0];
                    d11[1] = p10[1] - p11[1];
                    d11[2] = p10[2] - p11[2];
                    d11[3] = p10[3] - p11[3];

                    d00 += 4, d01 += 4, d10 += 4, d11 += 4;
                    p00 += 4, p01 += 4, p10 += 4, p11 += 4;
                }
                p00 = p01;
                p10 = p11;
                p01 = p00 + h;
                p11 = p10 + h;
            }

        } else {
            return STP_ERR_NOT_IMPL;
        }

    } else if (w < h) {
        ratio = h/w;

        if (ratio == 2) {
            stp_int32 *p00, *p01, *p10, *p11;
            stp_int32 *d00, *d01, *d10, *d11;

            p00 = (stp_int32*)x->real->p_mem;
            p10 = (stp_int32*)x->imag->p_mem;
            p01 = p00 + w*w;
            p11 = p10 + w*w;
            d00 = (stp_int32*)out[0]->real->p_mem;
            d10 = (stp_int32*)out[0]->imag->p_mem;
            d01 = (stp_int32*)out[1]->real->p_mem;
            d11 = (stp_int32*)out[1]->imag->p_mem;

            out[0]->sign = x->sign;
            out[0]->real->q = x->real->q;
            out[0]->imag->q = x->imag->q;
            out[1]->sign = x->sign;
            out[1]->real->q = x->real->q;
            out[1]->imag->q = x->imag->q;

            i = w*w >> 2;
            while (i--) {
                d00[0] = p00[0] + p01[0];
                d00[1] = p00[1] + p01[1];
                d00[2] = p00[2] + p01[2];
                d00[3] = p00[3] + p01[3];

                d10[0] = p10[0] + p11[0];
                d10[1] = p10[1] + p11[1];
                d10[2] = p10[2] + p11[2];
                d10[3] = p10[3] + p11[3];

                d01[0] = p00[0] - p01[0];
                d01[1] = p00[1] - p01[1];
                d01[2] = p00[2] - p01[2];
                d01[3] = p00[3] - p01[3];

                d11[0] = p10[0] - p11[0];
                d11[1] = p10[1] - p11[1];
                d11[2] = p10[2] - p11[2];
                d11[3] = p10[3] - p11[3];

                d00 += 4, d01 += 4, d10 += 4, d11 += 4;
                p00 += 4, p01 += 4, p10 += 4, p11 += 4;
            }

        } else {
            return STP_ERR_NOT_IMPL;
        }

    } else {
        out[0]->real = x->real;
        out[0]->imag = x->imag;
        out[0]->sign = x->sign;
    }

    return STP_OK;
}

#if !STP_FFT2DLayerCore_Optimized
static stp_ret STP_FFT2DLayerCore(stp_complex_image *out, stp_int32 block_num, stp_complex_image *temp,
        stp_complex_image *x, const stp_complex_image *table, stp_int32 layer_n) {
    stp_int32 i, j, k, l, m, n;
    stp_int32 *in, *i0, *i1, *i2, *i3;
    stp_int32 *o, *o0, *o1, *o2, *o3;
    stp_int32 wo, wi, ho, hi, s, ds;
    stp_int32 in_step0, in_step1, in_step2;
    stp_int32 out_step0, out_step1, out_step2, out_step3;
    stp_int32 out_off0, out_off1, out_off2, out_off3;
    stp_int32 a, b, c, d;

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

    out->real->q = temp->real->q;
    out->imag->q = temp->imag->q;
    out->sign = temp->sign;

    return STP_OK;
}
#else
extern stp_ret STP_FFT2DLayerCore(stp_complex_image *out, stp_int32 block_num, stp_complex_image *temp,
        stp_complex_image *x, const stp_complex_image *table, stp_int32 layer_n);
#endif

stp_ret STP_FFTInit(stp_fft **p_fft, stp_size size, stp_bool complex_in, stp_bool inverse) {
    stp_int32 i, j, k, l, m;
    stp_int32 ratio;
    stp_int16 *square_reorder_tbl;
    stp_fft *fft;
    stp_int32 *p0, *p1;
    stp_int32 a, b;
    double q_coef;

    STP_Assert(p_fft && size.h*size.w > 4);

    fft = (stp_fft *)STP_Calloc(1, sizeof(stp_fft));

    fft->complex_input = complex_in;
    fft->inverse = inverse;
    fft->size = size;

    if (size.h == 1 || size.w == 1) {
        /* only support 32-point 1-d fft */
        fft->one_dimension = STP_TRUE;
        fft->layers = 0;
        size.w = STP_MAX(size.h, size.w);
        size.h = 1;
        STP_ComplexImageInit(&fft->temp_image0, size);
        while (size.w >> (fft->layers + 1)) {
            stp_int32 s = (1 << fft->layers);
            STP_ComplexImageInit(&fft->coef_tables[fft->layers], size);
            p0 = (stp_int32*)fft->coef_tables[fft->layers]->real->p_mem;
            p1 = (stp_int32*)fft->coef_tables[fft->layers]->imag->p_mem;
            q_coef = pow(2, 31) - 1; /* minus one to work around overflow bugs on some platform */
            fft->coef_tables[fft->layers]->real->q = 31;
            fft->coef_tables[fft->layers]->imag->q = 31;

            if (fft->inverse) {
                for (i = 0; i < s; i++) {
                    p0[i] = q_coef*cos(STP_PI*i/s);
                    p1[i] = q_coef*sin(STP_PI*i/s);
                }
            } else {
                for (i = 0; i < s; i++) {
                    p0[i] = q_coef*cos(STP_PI*i/s);
                    p1[i] = -q_coef*sin(STP_PI*i/s);
                }
            }
            fft->layers++;
        }
    } else {
        fft->one_dimension = STP_FALSE;
        STP_ImageInit(&fft->zero_image, size);
        STP_ComplexImageInit(&fft->reorder_image, size);

        ratio = STP_MAX(size.h, size.w)/STP_MIN(size.h, size.w);
        size.h = STP_MIN(size.h, size.w);
        size.w = size.h;

        switch (ratio) {
        case 1:
            fft->square_input = STP_TRUE;
            fft->sub_blocks = 1;
            break;
        case 2:
            fft->square_input = STP_FALSE;
            fft->sub_blocks = 2;
            break;
        default:
            return STP_ERR_NOT_IMPL;
        }

        for (i = 0; i < fft->sub_blocks; i++) {
            STP_ComplexImageInit(&fft->square_images[i], size);
        }

        STP_ComplexImageInit(&fft->temp_image0, size);
        STP_ComplexImageInit(&fft->temp_image1, size);

        fft->layers = 0;
        while (size.h >> (fft->layers + 1)) {
            stp_int32 s = (1 << fft->layers), ds = s << 1;
            STP_ComplexImageInit(&fft->coef_tables[fft->layers], size);
            p0 = (stp_int32*)fft->coef_tables[fft->layers]->real->p_mem;
            p1 = (stp_int32*)fft->coef_tables[fft->layers]->imag->p_mem;
            q_coef = pow(2, fft->coef_tables[fft->layers]->real->q);

            for (j = 0; j < s; j++) {
                for (k = 0; k < s; k++) {
                    p0[j*size.w + k] = q_coef;
                    p1[j*size.w + k] = 0;

                    p0[j*size.w + k + s] = q_coef*cos(STP_PI*k/s);
                    p1[j*size.w + k + s] = q_coef*sin(STP_PI*k/s);

                    p0[(j + s)*size.w + k] = q_coef*cos(STP_PI*j/s);
                    p1[(j + s)*size.w + k] = q_coef*sin(STP_PI*j/s);

                    p0[(j + s)*size.w + k + s] = q_coef*cos(STP_PI*(j + k)/s);
                    p1[(j + s)*size.w + k + s] = q_coef*sin(STP_PI*(j + k)/s);
                }
            }

            for (l = 0; l < size.w/ds; l++) {
                for (m = 0; m < size.w/ds; m++) {
                    for (j = 0; j < ds; j++) {
                        for (k = 0; k < ds; k++) {
                            p0[(j + l*ds)*size.w + k + m*ds] = p0[j*size.w + k];
                            p1[(j + l*ds)*size.w + k + m*ds] = p1[j*size.w + k];
                        }
                    }
                }
            }

            if (fft->inverse) {
                fft->coef_tables[fft->layers]->sign = 1;
            } else {
                fft->coef_tables[fft->layers]->sign = -1;
            }

            fft->layers++;
        }

        square_reorder_tbl = (stp_int16*)STP_Calloc(size.h*size.w, sizeof(stp_int16));
        for (i = 0; i < size.w; i++) {
            for (j = 0; j < size.h; j++) {
                square_reorder_tbl[i*size.h + j] = STP_FFTBitReverse(i, fft->layers)*size.h
                        + STP_FFTBitReverse(j, fft->layers);
            }
        }

        if (fft->square_input) {
            fft->reorder_table = square_reorder_tbl;
        } else {
            STP_ComplexImageInit(&fft->prefft_coef, size);
            p0 = (stp_int32*)fft->prefft_coef->imag->p_mem;
            p1 = (stp_int32*)fft->prefft_coef->temp->p_mem;
            for (j = 0; j < size.h; j++) {
                a = q_coef*cos(STP_PI*j/size.h);
                b = q_coef*sin(STP_PI*j/size.h);
                if (fft->size.w < fft->size.h) {
                    for (k = 0; k < size.h; k++) {
                        p0[j*size.w + k] = a;
                        p1[j*size.w + k] = b;
                    }
                } else {
                    for (k = 0; k < size.h; k++) {
                        p0[j + k*size.h] = a;
                        p1[j + k*size.h] = b;
                    }
                }
            }

            STP_FFT2DImageReorder(fft->prefft_coef->real, fft->prefft_coef->imag, square_reorder_tbl);
            STP_FFT2DImageReorder(fft->prefft_coef->imag, fft->prefft_coef->temp, square_reorder_tbl);

            if (fft->inverse) {
                fft->prefft_coef->sign = 1;
            } else {
                fft->prefft_coef->sign = -1;
            }

            STP_Free(square_reorder_tbl);

            fft->reorder_table = (stp_int16*)STP_Calloc(fft->size.h*fft->size.w, sizeof(stp_int16));
            for (i = 0; i < fft->size.h; i++) {
                for (j = 0; j < fft->size.w; j++) {
                    fft->reorder_table[i*fft->size.w + j] = (STP_FFTBitReverse(i%size.h, fft->layers) + i/size.h*size.h)*fft->size.w
                            + STP_FFTBitReverse(j%size.w, fft->layers) + j/size.w*size.w;
                }
            }
        }
    }

    *p_fft = fft;
    return STP_OK;
}

stp_ret STP_FFTCompute(stp_fft *fft, void *in_image,
        stp_complex_image *out_image) {
    stp_int32 i, j;
    stp_complex_image work_image, *in = &work_image;

    if (fft->one_dimension) {
        if (fft->complex_input) {
            STP_FFT1D32PointComplexInCore(out_image, fft->temp_image0, fft->coef_tables, (stp_complex_image *)in_image);
        } else {
            STP_FFT1D32PointRealInCore(out_image, fft->temp_image0, fft->coef_tables, (stp_image *)in_image);
        }
    } else {
        if (fft->complex_input) {
            in = (stp_complex_image *)in_image;
            STP_FFT2DImageReorder(fft->reorder_image->real, in->real, fft->reorder_table);
            STP_FFT2DImageReorder(fft->reorder_image->imag, in->imag, fft->reorder_table);
            fft->reorder_image->sign = in->sign;
            in = fft->reorder_image;
        } else {
            STP_FFT2DImageReorder(fft->reorder_image->real, (stp_image *)in_image, fft->reorder_table);
            in->real = fft->reorder_image->real;
            in->imag = fft->zero_image;
            in->imag->q = in->real->q;
            in->sign = 1;
        }

        STP_Assert(in->real->height == fft->size.h);
        STP_Assert(in->real->width == fft->size.w);
        STP_Assert(out_image->real->height == fft->size.h);
        STP_Assert(out_image->real->width == fft->size.w);

        if (!fft->square_input) {
            STP_FFT2DConvertToSquareImage(fft->square_images, in);
            if (fft->inverse) {
                fft->square_images[0]->real->q++;
                fft->square_images[0]->imag->q++;
                fft->square_images[1]->real->q++;
                fft->square_images[1]->imag->q++;
            }
            STP_ComplexImageDotMulC(fft->temp_image1, fft->square_images[1], fft->prefft_coef);
            fft->square_images[0]->real->q++;
            fft->square_images[0]->imag->q++;
            STP_FFT2DLayerCore(fft->square_images[0], 0, fft->temp_image0,
                    fft->square_images[0], fft->coef_tables[0], 0);
            fft->temp_image1->real->q++;
            fft->temp_image1->imag->q++;
            STP_FFT2DLayerCore(fft->square_images[1], 0, fft->temp_image0,
                    fft->temp_image1, fft->coef_tables[0], 0);
        } else {
            in->real->q++;
            in->imag->q++;
            STP_FFT2DLayerCore(fft->square_images[0], 0, fft->temp_image0,
                    in, fft->coef_tables[0], 0);
        }

        for (i = 1; i < fft->layers - 1; i++) {
            for (j = 0; j < fft->sub_blocks; j++) {
                fft->square_images[j]->real->q++;
                fft->square_images[j]->imag->q++;
                STP_FFT2DLayerCore(fft->square_images[j], 0, fft->temp_image0,
                        fft->square_images[j], fft->coef_tables[i], i);
            }
        }

        /* last layer, we output result to out_image */
        for (j = 0; j < fft->sub_blocks; j++) {
            fft->square_images[j]->real->q++;
            fft->square_images[j]->imag->q++;
            STP_FFT2DLayerCore(out_image, j, fft->temp_image0,
                    fft->square_images[j], fft->coef_tables[i], i);
        }
    }

    return STP_OK;
}

stp_ret STP_FFTDestroy(stp_fft *fft) {
    stp_int32 i;

    if (fft) {
        STP_ImageDestroy(fft->zero_image);
        STP_ComplexImageDestroy(fft->temp_image0);
        STP_ComplexImageDestroy(fft->temp_image1);

        for (i = 0; i < fft->sub_blocks; i++) {
            STP_ComplexImageDestroy(fft->square_images[i]);
        }
        for (i = 0; i < fft->layers; i++) {
            STP_ComplexImageDestroy(fft->coef_tables[i]);
        }

        STP_ComplexImageDestroy(fft->prefft_coef);
        STP_ComplexImageDestroy(fft->reorder_image);

        STP_Free(fft->reorder_table);
        STP_Free(fft);
    }
    return STP_OK;
}



