/*
 * fhog.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "stp_fhog.h"
#include "math.h"

#define STP_FHOG_MAX_GRAD 128
static stp_image *global_angle_table, *global_norm_table;
static stp_bool global_tbl_inited = STP_FALSE;

#if !STP_FhogGradHist_Optimized
static stp_ret STP_FhogGradHist(stp_image *x_grad, stp_image *y_grad,
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

    if (c_w == 8) c_s = 3, c_mask = 0x7;
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

    /* tri-linear interpolation unnormalized gradient histograms */
    for (i = 0; i < h - c_w; i++) {
        for (j = 0; j < w - c_w; j++) {
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

    return STP_OK;
}
#else
extern stp_ret STP_FhogGradHist(stp_image *x_grad, stp_image *y_grad,
        const stp_image *angle_table, const stp_image *norm_table,
        stp_image *out_hogs[], stp_int32 cell_w);
#endif

stp_ret STP_FhogInit(stp_fhog **p_fhog, stp_size imag_size, stp_size cell_size, stp_hog_type hog_type) {
    stp_int32 i, j = 0, q, temp, *p;
    stp_fhog *fhog = (stp_fhog *)STP_Calloc(1, sizeof(stp_fhog));
    stp_size hog_sz = {imag_size.w/cell_size.w, imag_size.h/cell_size.h}, table_sz;

    STP_Assert(fhog && p_fhog);

    switch (hog_type) {
        case STP_HOGDIM_18:
            fhog->hog_dims = 18;
            break;
        case STP_HOGDIM_18_9:
            fhog->hog_dims = 27;
            break;
        case STP_HOGDIM_18_9_1:
            fhog->hog_dims = 28;
            break;
        case STP_HOGDIM_18_9_3:
            fhog->hog_dims = 30;
            break;
        case STP_HOGDIM_18_9_4:
            fhog->hog_dims = 31;
            break;
        default:
            STP_Assert(STP_TRUE);
            break;
    }

    fhog->hog_type = hog_type;
    fhog->imag_sz = imag_size;
    fhog->cell_sz = cell_size;
    fhog->max_grad = STP_FHOG_MAX_GRAD;

    fhog->is_clip = STP_TRUE;
    fhog->clip_value.q = 15;
    fhog->clip_value.value = STP_FXP32(0.2f, fhog->clip_value.q);

    STP_ImageInit(&fhog->x_grad, imag_size);
    STP_ImageInit(&fhog->y_grad, imag_size);
    STP_ImageInit(&fhog->x_grad_temp, imag_size);
    STP_ImageInit(&fhog->y_grad_temp, imag_size);
    STP_ImageInit(&fhog->norm[0], hog_sz);
    STP_ImageInit(&fhog->norm[1], hog_sz);
    STP_ImageInit(&fhog->norm[2], hog_sz);
    STP_ImageInit(&fhog->norm[3], hog_sz);
    STP_ImageInit(&fhog->temp[0], hog_sz);
    STP_ImageInit(&fhog->temp[1], hog_sz);
    STP_ImageInit(&fhog->temp[2], hog_sz);
    STP_ImageInit(&fhog->temp[3], hog_sz);

    if (!global_tbl_inited) {
        table_sz.w = fhog->max_grad*2;
        table_sz.h = fhog->max_grad*2;
        STP_ImageInit(&global_norm_table, table_sz);
        STP_ImageInit(&global_angle_table, table_sz);

        global_norm_table->q = 6;
        p = (stp_int32*)global_norm_table->p_mem;
        q = 1L << global_norm_table->q;
        for (i = 0; i < fhog->max_grad; i++) {
            for (j = 0; j < fhog->max_grad; j++) {
                temp = sqrt(((i*i)+(j*j)))*q + 0.5f;
                p[(i + fhog->max_grad)*fhog->max_grad*2 + j + fhog->max_grad] = temp;
                p[(-i + fhog->max_grad)*fhog->max_grad*2 + j + fhog->max_grad] = temp;
                p[(i + fhog->max_grad)*fhog->max_grad*2 - j + fhog->max_grad] = temp;
                p[(-i + fhog->max_grad)*fhog->max_grad*2 - j + fhog->max_grad] = temp;
            }
            temp = sqrt(((i*i)+(j*j)))*q + 0.5f;
            p[(i + fhog->max_grad)*fhog->max_grad*2] = temp;
            p[(-i + fhog->max_grad)*fhog->max_grad*2] = temp;
            p[i + fhog->max_grad] = temp;
            p[-i + fhog->max_grad] = temp;
        }
        p[0] = sqrt(((i*i)+(j*j)))*q + 0.5f;

        global_angle_table->q = 0;
        p = (stp_int32*)global_angle_table->p_mem;
        for (i = 0; i < fhog->max_grad; i++) {
            p[(i + fhog->max_grad)*fhog->max_grad*2 + fhog->max_grad] = (i == 0 ? 0 : 16*9);
            p[(-i + fhog->max_grad)*fhog->max_grad*2 + fhog->max_grad] = (i == 0 ? 0 : 16*27);
            for (j = 1; j < fhog->max_grad; j++) {
                temp = atan((double)i/j)/STP_PI*32*9 + 0.5f;
                p[(-i + fhog->max_grad)*fhog->max_grad*2 + j + fhog->max_grad] = 32*18 - temp;
                p[(i + fhog->max_grad)*fhog->max_grad*2 - j + fhog->max_grad] = 32*9 - temp;
                p[(-i + fhog->max_grad)*fhog->max_grad*2 - j + fhog->max_grad] = temp + 32*9;
                p[(i + fhog->max_grad)*fhog->max_grad*2 + j + fhog->max_grad] = temp;
            }
            temp = atan((double)i/j)/STP_PI*32*9 + 0.5f;
            p[(i + fhog->max_grad)*fhog->max_grad*2] = 32*9 - temp;
            p[(-i + fhog->max_grad)*fhog->max_grad*2] = temp + 32*9;
            p[i + fhog->max_grad] = temp + 16*27;
            p[-i + fhog->max_grad] = 16*27 - temp;
        }
        p[0] = atan((double)i/j)/STP_PI*32*9 + 0.5f + 32*9;

        global_tbl_inited = STP_TRUE;
    }

    fhog->norm_table = global_norm_table;
    fhog->angle_table = global_angle_table;

    *p_fhog = fhog;

    return STP_OK;
}

stp_ret STP_FhogCompute(stp_fhog *fhog, stp_multichannel_image *in_image, stp_image *out_hogs[]) {
    stp_int32 i, j, h, w, h_w, h_h, c_w, c_s, max_g;
    stp_int32 o, eps;
    stp_int32 *p[4];
    stp_int32_q temp = {1, 15};
    stp_int32_q scalar;

    STP_Assert(fhog && in_image && out_hogs);

    h = in_image->images[0]->height;
    w = in_image->images[0]->width;
    STP_ImageGradient(fhog->x_grad, fhog->y_grad, in_image->images[0]);
    STP_ImageGradient(fhog->x_grad_temp, fhog->y_grad_temp, in_image->images[1]);
    STP_ImageDotDominance(fhog->x_grad, fhog->x_grad, fhog->x_grad_temp);
    STP_ImageDotDominance(fhog->y_grad, fhog->y_grad, fhog->y_grad_temp);
    STP_ImageGradient(fhog->x_grad_temp, fhog->y_grad_temp, in_image->images[2]);
    STP_ImageDotDominance(fhog->x_grad, fhog->x_grad, fhog->x_grad_temp);
    STP_ImageDotDominance(fhog->y_grad, fhog->y_grad, fhog->y_grad_temp);

    max_g = fhog->max_grad;
    temp.q = fhog->x_grad->q;
    temp.value = (max_g - 1) << temp.q;
    STP_ImageDotClipCeil(fhog->x_grad, fhog->x_grad, temp);
    STP_ImageDotClipCeil(fhog->y_grad, fhog->y_grad, temp);
    temp.q = fhog->x_grad->q;
    temp.value = -(max_g) << temp.q;
    STP_ImageDotClipFloor(fhog->x_grad, fhog->x_grad, temp);
    STP_ImageDotClipFloor(fhog->y_grad, fhog->y_grad, temp);

    temp.q = fhog->x_grad->q;
    temp.value = max_g << temp.q;
    STP_ImageDotAddScalar(fhog->x_grad, fhog->x_grad, temp);
    STP_ImageDotAddScalar(fhog->y_grad, fhog->y_grad, temp);
    c_w = fhog->cell_sz.h;
    if (c_w == 8) c_s = 3;
    if (c_w == 4) c_s = 2;
    h_w = w >> c_s; /* hog width */
    h_h = h >> c_s; /* hog width */

    /* un-normalized hog */
    STP_FhogGradHist(fhog->x_grad, fhog->y_grad,
            fhog->angle_table, fhog->norm_table,
            out_hogs, fhog->cell_sz.w);

    if (fhog->hog_type > STP_HOGDIM_18) {
        STP_ImageDotAdd(out_hogs[18], out_hogs[0], out_hogs[9]);
        STP_ImageDotMul(fhog->norm[3], out_hogs[18], out_hogs[18]);
        for (i = 1; i < 9; i++) {
            STP_ImageDotAdd(out_hogs[i + 18], out_hogs[i], out_hogs[i + 9]);
            STP_ImageDotMul(fhog->temp[0], out_hogs[i + 18], out_hogs[i + 18]);
            STP_ImageDotAdd(fhog->norm[3], fhog->norm[3], fhog->temp[0]);
        }
    } else {
        STP_ImageDotAdd(fhog->temp[0], out_hogs[0], out_hogs[9]);
        STP_ImageDotMul(fhog->norm[3], fhog->temp[0], fhog->temp[0]);
        for (i = 1; i < 9; i++) {
            STP_ImageDotAdd(fhog->temp[0], out_hogs[i], out_hogs[i + 9]);
            STP_ImageDotMul(fhog->temp[0], fhog->temp[0], fhog->temp[0]);
            STP_ImageDotAdd(fhog->norm[3], fhog->norm[3], fhog->temp[0]);
        }
    }

    p[0] = (stp_int32*)fhog->norm[0]->p_mem;
    p[1] = (stp_int32*)fhog->norm[1]->p_mem;
    p[2] = (stp_int32*)fhog->norm[2]->p_mem;
    p[3] = (stp_int32*)fhog->norm[3]->p_mem;

    eps = 16;
    for (i = 0; i < h_h - 1; i++) {
        for (j = 0; j < h_w - 1; j++) {
            p[3][i*h_w + j] = p[3][i*h_w + j] + p[3][(i + 1)*h_w + j] + p[3][i*h_w + j + 1] + p[3][(i + 1)*h_w + j + 1] + eps;
        }
    }

    STP_ImageDotSquareRootRecipe(fhog->norm[3], fhog->norm[3]);

    for (i = 0; i < h_h - 1; i++) {
        for (j = 0; j < h_w - 1; j++) {
            o = p[3][i*h_w + j];
            p[0][(i + 1)*h_w + j + 1] = o;
            p[1][(i + 1)*h_w + j] = o;
            p[2][i*h_w + j + 1] = o;
        }
    }

    p[0][0] = p[0][h_w + 1];
    p[1][h_w - 1] = p[1][h_w*2 - 2];
    p[2][(h_h - 1)*h_w] = p[2][(h_h - 2)*h_w + 1];
    p[3][h_h*h_w - 1] = p[3][h_h*h_w - h_w - 2];

    for (i = 0; i < h_w - 1; i++) {
        p[0][i + 1] = p[0][i + h_w + 1];
        p[1][i] = p[1][i + h_w];
        p[2][(h_h - 1)*h_w + i + 1] = p[2][(h_h - 2)*h_w + i + 1];
        p[3][(h_h - 1)*h_w + i] = p[3][(h_h - 2)*h_w + i];
    }

    for (i = 0; i < h_h - 1; i++) {
        p[0][(i + 1)*h_w] = p[0][(i + 1)*h_w + 1];
        p[1][(i + 2)*h_w - 1] = p[1][(i + 2)*h_w - 2];
        p[2][i*h_w] = p[2][i*h_w + 1];
        p[3][(i + 1)*h_w - 1] = p[3][(i + 1)*h_w - 2];
    }

    fhog->norm[0]->q = fhog->norm[3]->q;
    fhog->norm[1]->q = fhog->norm[3]->q;
    fhog->norm[2]->q = fhog->norm[3]->q;

    if (!fhog->is_clip) {
        STP_ImageDotAdd(fhog->temp[0], fhog->norm[0], fhog->norm[1]);
        STP_ImageDotAdd(fhog->temp[0], fhog->temp[0], fhog->norm[2]);
        STP_ImageDotAdd(fhog->temp[0], fhog->temp[0], fhog->norm[3]);

        /* normalized hog */
        for (i = 0; i < 18; i++) {
            STP_ImageDotMul(out_hogs[i], out_hogs[i], fhog->temp[0]);
            out_hogs[i]->q += 1; /* *0.5f */
        }
        if (fhog->hog_type == STP_HOGDIM_18_9_4) {
            STP_ImageDotAdd(out_hogs[30], out_hogs[18], out_hogs[19]);
            for (i = 2; i < 9; i++) {
                STP_ImageDotAdd(out_hogs[30], out_hogs[i + 18], out_hogs[30]);
            }

            scalar.q = 15;
            scalar.value = STP_FXP32(0.2357f, scalar.q);
            STP_ImageDotMulScalar(out_hogs[30], out_hogs[30], scalar);

            STP_ImageDotMul(out_hogs[27], fhog->norm[0], out_hogs[30]);
            STP_ImageDotMul(out_hogs[28], fhog->norm[1], out_hogs[30]);
            STP_ImageDotMul(out_hogs[29], fhog->norm[2], out_hogs[30]);
            STP_ImageDotMul(out_hogs[30], fhog->norm[3], out_hogs[30]);
        }
        if (fhog->hog_type > STP_HOGDIM_18) {
            for (i = 0; i < 9; i++) {
                STP_ImageDotMul(out_hogs[i + 18], out_hogs[i + 18], fhog->temp[0]);
                out_hogs[i + 18]->q += 1; /* 0.5f */
            }
        }
    } else {
        if (fhog->hog_type == STP_HOGDIM_18_9_4) {
            STP_ImageClear(out_hogs[27]);
            STP_ImageClear(out_hogs[28]);
            STP_ImageClear(out_hogs[29]);
            STP_ImageClear(out_hogs[30]);
        }

        for (i = 0; i < 18; i++) {
            STP_ImageDotMul(fhog->temp[0], out_hogs[i], fhog->norm[0]);
            STP_ImageDotClipCeil(fhog->temp[0], fhog->temp[0], fhog->clip_value);

            STP_ImageDotMul(fhog->temp[1], out_hogs[i], fhog->norm[1]);
            STP_ImageDotClipCeil(fhog->temp[1], fhog->temp[1], fhog->clip_value);

            STP_ImageDotMul(fhog->temp[2], out_hogs[i], fhog->norm[2]);
            STP_ImageDotClipCeil(fhog->temp[2], fhog->temp[2], fhog->clip_value);

            STP_ImageDotMul(out_hogs[i], out_hogs[i], fhog->norm[3]);
            STP_ImageDotClipCeil(out_hogs[i], out_hogs[i], fhog->clip_value);
            if (fhog->hog_type == STP_HOGDIM_18_9_4) {
                STP_ImageDotAdd(out_hogs[27], out_hogs[27], fhog->temp[0]);
                STP_ImageDotAdd(out_hogs[28], out_hogs[28], fhog->temp[1]);
                STP_ImageDotAdd(out_hogs[29], out_hogs[29], fhog->temp[2]);
                STP_ImageDotAdd(out_hogs[30], out_hogs[30], out_hogs[i]);
            }

            STP_ImageDotAdd(out_hogs[i], out_hogs[i], fhog->temp[0]);
            STP_ImageDotAdd(out_hogs[i], out_hogs[i], fhog->temp[1]);
            STP_ImageDotAdd(out_hogs[i], out_hogs[i], fhog->temp[2]);

            out_hogs[i]->q += 1; /* *0.5f */
        }

        if (fhog->hog_type == STP_HOGDIM_18_9_4) {
            scalar.q = 30;
            scalar.value = STP_FXP32(0.2357f, scalar.q);
            STP_ImageDotMulScalar(out_hogs[27], out_hogs[27], scalar);
            STP_ImageDotMulScalar(out_hogs[28], out_hogs[28], scalar);
            STP_ImageDotMulScalar(out_hogs[29], out_hogs[29], scalar);
            STP_ImageDotMulScalar(out_hogs[30], out_hogs[30], scalar);
        }

        if (fhog->hog_type > STP_HOGDIM_18) {
            for (i = 0; i < 9; i++) {
                STP_ImageDotMul(fhog->temp[0], out_hogs[i + 18], fhog->norm[0]);
                STP_ImageDotClipCeil(fhog->temp[0], fhog->temp[0], fhog->clip_value);

                STP_ImageDotMul(fhog->temp[1], out_hogs[i + 18], fhog->norm[1]);
                STP_ImageDotClipCeil(fhog->temp[1], fhog->temp[1], fhog->clip_value);

                STP_ImageDotMul(fhog->temp[2], out_hogs[i + 18], fhog->norm[2]);
                STP_ImageDotClipCeil(fhog->temp[2], fhog->temp[2], fhog->clip_value);

                STP_ImageDotMul(out_hogs[i + 18], out_hogs[i + 18], fhog->norm[3]);
                STP_ImageDotClipCeil(out_hogs[i + 18], out_hogs[i + 18], fhog->clip_value);

                STP_ImageDotAdd(out_hogs[i + 18], out_hogs[i + 18], fhog->temp[0]);
                STP_ImageDotAdd(out_hogs[i + 18], out_hogs[i + 18], fhog->temp[1]);
                STP_ImageDotAdd(out_hogs[i + 18], out_hogs[i + 18], fhog->temp[2]);
                out_hogs[i + 18]->q += 1; /* *0.5f */
            }
        }
    }

    if (fhog->hog_type == STP_HOGDIM_18_9_1) {
        STP_ImageResize(fhog->temp[0], in_image->images[0], NULL);
        STP_ImageResize(fhog->temp[1], in_image->images[1], NULL);
        STP_ImageResize(fhog->temp[2], in_image->images[2], NULL);
        scalar.q = 30;
        scalar.value = STP_FXP32(0.299f, 30);
        STP_ImageDotMulScalar(out_hogs[27], fhog->temp[0], scalar);
        scalar.q = 30;
        scalar.value = STP_FXP32(0.587f, 30);
        STP_ImageDotMulScalar(fhog->temp[1], fhog->temp[1], scalar);
        scalar.q = 30;
        scalar.value = STP_FXP32(0.144f, 30);
        STP_ImageDotMulScalar(fhog->temp[2], fhog->temp[2], scalar);
        STP_ImageDotAdd(out_hogs[27], out_hogs[27], fhog->temp[1]);
        STP_ImageDotAdd(out_hogs[27], out_hogs[27], fhog->temp[2]);

        out_hogs[27]->q = 8; /* image/256 */
        scalar.q = 8;
        scalar.value = STP_FXP32(-0.5f, scalar.q);
        STP_ImageDotAddScalar(out_hogs[27], out_hogs[27], scalar);
    }

    if (fhog->hog_type == STP_HOGDIM_18_9_3) {
        STP_ImageResize(out_hogs[27], in_image->images[0], NULL);
        STP_ImageResize(out_hogs[28], in_image->images[1], NULL);
        STP_ImageResize(out_hogs[29], in_image->images[2], NULL);

        out_hogs[27]->q = 8; /* image/256 */
        out_hogs[28]->q = 8; /* image/256 */
        out_hogs[29]->q = 8; /* image/256 */
        scalar.q = 8;
        scalar.value = STP_FXP32(-0.5f, scalar.q);
        STP_ImageDotAddScalar(out_hogs[27], out_hogs[27], scalar);
        STP_ImageDotAddScalar(out_hogs[28], out_hogs[28], scalar);
        STP_ImageDotAddScalar(out_hogs[29], out_hogs[29], scalar);
    }

    return STP_OK;
}

stp_ret STP_FhogGetDims(stp_fhog *fhog, stp_int32 *dims) {
    STP_Assert(fhog && dims);
    *dims = fhog->hog_dims;
    return STP_OK;
}

stp_ret STP_FhogDestroy(stp_fhog *fhog) {
    if (fhog) {
        STP_ImageDestroy(fhog->temp[0]);
        STP_ImageDestroy(fhog->temp[1]);
        STP_ImageDestroy(fhog->temp[2]);
        STP_ImageDestroy(fhog->temp[3]);
        STP_ImageDestroy(fhog->norm[0]);
        STP_ImageDestroy(fhog->norm[1]);
        STP_ImageDestroy(fhog->norm[2]);
        STP_ImageDestroy(fhog->norm[3]);
        STP_ImageDestroy(fhog->x_grad);
        STP_ImageDestroy(fhog->y_grad);
        STP_ImageDestroy(fhog->x_grad_temp);
        STP_ImageDestroy(fhog->y_grad_temp);
        fhog->angle_table = NULL;
        fhog->norm_table = NULL;

        STP_Free(fhog);
    }

    return STP_OK;
}

