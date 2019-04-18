/*
 * stp_hist.c
 *
 *  Created on: 2018-9-19
 *      Author: handsomelee
 */

#include "stp_hist.h"

stp_ret STP_HistogramInit(stp_hist **p_hist, stp_multichannel_image *image,
        stp_rect rect, stp_bool inside, stp_int32 learning_rate_q30) {
    stp_int32 i, j;
    stp_hist *hist;
    stp_size hist_size;

    hist = (stp_hist *)STP_Calloc(1, sizeof(stp_hist));

    hist->start.x = rect.pos.x - (rect.size.w >> 1);
    hist->start.y = rect.pos.y - (rect.size.h >> 1);
    hist->end.x = rect.pos.x + (rect.size.w >> 1);
    hist->end.y = rect.pos.y + (rect.size.h >> 1);
    hist->rect_inside_hist = inside;

    if (image->images[1] && image->images[2]) {
        stp_int32 *p0, *p1, *p2, *o;
        stp_int32 r, g, b, h, w, s0, s1, s2;
        hist->gray_image = STP_FALSE;
        hist_size.h = 1;
        hist_size.w = HIST_BIN_WIDTH*HIST_BIN_WIDTH*HIST_BIN_WIDTH;

        s0 = 8 - HIST_BIN_SHIFT;
        s1 = s0 << 1;
        s2 = image->images[0]->q + HIST_BIN_SHIFT;
        h = image->images[0]->height;
        w = image->images[0]->width;

        STP_ImageInit(&hist->hist_data, hist_size);
        STP_ImageInit(&hist->new_hist_data, hist_size);
        hist->hist_data->q = 8;
        hist->new_hist_data->q = 8;

        p0 = (stp_int32*)image->images[0]->p_mem;
        p1 = (stp_int32*)image->images[1]->p_mem;
        p2 = (stp_int32*)image->images[2]->p_mem;
        o = (stp_int32*)hist->hist_data->p_mem;

        if (hist->rect_inside_hist) {
            /* compute histogram of the rect inside area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->start.x; j < hist->end.x; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
        } else {
            /* compute histogram of the rect outside area */
            /* top part area */
            for (i = 0; i < hist->start.y; i++) {
                for (j = 0; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* left part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = 0; j < hist->start.x; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* right part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->end.x; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* bottom part area */
            for (i = hist->end.y; i < h; i++) {
                for (j = 0; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
        }
    } else {
        stp_int32 *p, *o;
        stp_int32 h, w, s;
        hist->gray_image = STP_TRUE;
        hist_size.h = 1;
        hist_size.w = HIST_BIN_WIDTH;

        s = image->images[0]->q + HIST_BIN_SHIFT;
        h = image->images[0]->height;
        w = image->images[0]->width;

        STP_ImageInit(&hist->hist_data, hist_size);
        STP_ImageInit(&hist->new_hist_data, hist_size);
        hist->hist_data->q = 8;
        hist->new_hist_data->q = 8;

        p = (stp_int32*)image->images[0]->p_mem;
        o = (stp_int32*)hist->hist_data->p_mem;

        if (hist->rect_inside_hist) {
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->start.x; j < hist->end.x; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
        } else {
            /* compute histogram of the rect outside area */
            /* top part area */
            for (i = 0; i < hist->start.y; i++) {
                for (j = 0; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* left part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = 0; j < hist->start.x; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* right part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->end.x; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* bottom part area */
            for (i = hist->end.y; i < h; i++) {
                for (j = 0; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
        }
    }

    hist->learning_ratio.q = 24;
    hist->learning_ratio.value = STP_FXP32(1.0f, 24);
    hist->learning_rate.value = learning_rate_q30;
    hist->learning_rate.q = 30;
    hist->forgotting_rate.value = (1LL << 30) - learning_rate_q30;
    hist->forgotting_rate.q = 30;

    *p_hist = hist;

    return STP_OK;
}

stp_ret STP_HistogramCompute(stp_hist *hist, stp_multichannel_image *image, stp_image *pwp) {
    stp_int32 i, j;

    STP_Assert(hist && image && pwp);

    if (!hist->gray_image) {
        stp_int32 *p0, *p1, *p2, *t, *o;
        stp_int32 r, g, b, w, h, s0, s1, s2;

        s0 = 8 - HIST_BIN_SHIFT;
        s1 = s0 << 1;
        s2 = image->images[0]->q + HIST_BIN_SHIFT;
        w = image->images[0]->width;
        h = image->images[0]->height;

        p0 = (stp_int32*)image->images[0]->p_mem;
        p1 = (stp_int32*)image->images[1]->p_mem;
        p2 = (stp_int32*)image->images[2]->p_mem;
        t = (stp_int32*)hist->hist_data->p_mem;
        o = (stp_int32*)pwp->p_mem;

        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                r = p0[i*w + j] >> s2;
                g = p1[i*w + j] >> s2;
                b = p2[i*w + j] >> s2;
                o[i*w + j] = t[(r << s1) | (g << s0) | b];
            }
        }
    } else {
        stp_int32 *p, *o, *t;
        stp_int32 w, h, s;

        s = image->images[0]->q + HIST_BIN_SHIFT;
        w = image->images[0]->width;
        h = image->images[0]->height;

        p = (stp_int32*)image->images[0]->p_mem;
        t = (stp_int32*)hist->hist_data->p_mem;
        o = (stp_int32*)pwp->p_mem;

        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               o[i*w + j] = t[p[i*w + j] >> s];
            }
        }
        pwp->q = hist->hist_data->q;
    }

    return STP_OK;
}

stp_ret STP_HistogramSetLearningRatio(stp_hist *hist, stp_int32 learning_ratio_q24) {
    STP_Assert(hist);
    hist->learning_ratio.q = 24;
    hist->learning_ratio.value = learning_ratio_q24;
    return STP_OK;
}

stp_ret STP_HistogramUpdate(stp_hist *hist, stp_multichannel_image *image) {
    stp_int32 i, j;
    stp_int32_q learning_rate, forgotting_rate;

    STP_Assert(hist && image);

    STP_ImageClear(hist->new_hist_data);
    hist->new_hist_data->q = 8;

    if (!hist->gray_image) {
        stp_int32 *p0, *p1, *p2, *o;
        stp_int32 r, g, b, h, w, s0, s1, s2;

        s0 = 8 - HIST_BIN_SHIFT;
        s1 = s0 << 1;
        s2 = image->images[0]->q + HIST_BIN_SHIFT;
        h = image->images[0]->height;
        w = image->images[0]->width;

        p0 = (stp_int32*)image->images[0]->p_mem;
        p1 = (stp_int32*)image->images[1]->p_mem;
        p2 = (stp_int32*)image->images[2]->p_mem;
        o = (stp_int32*)hist->new_hist_data->p_mem;

        if (hist->rect_inside_hist) {
            /* compute histogram of the rect inside area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->start.x; j < hist->end.x; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
        } else {
            /* compute histogram of the rect outside area */
            /* top part area */
            for (i = 0; i < hist->start.y; i++) {
                for (j = 0; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* left part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = 0; j < hist->start.x; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* right part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->end.x; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
            /* bottom part area */
            for (i = hist->end.y; i < h; i++) {
                for (j = 0; j < w; j++) {
                    r = p0[i*w + j] >> s2;
                    g = p1[i*w + j] >> s2;
                    b = p2[i*w + j] >> s2;
                    o[(r << s1) | (g << s0) | b] += (1L << 8);
                }
            }
        }
    } else {
        stp_int32 *p, *o;
        stp_int32 h, w, s;

        s = image->images[0]->q + HIST_BIN_SHIFT;
        h = image->images[0]->height;
        w = image->images[0]->width;

        p = (stp_int32*)image->images[0]->p_mem;
        o = (stp_int32*)hist->new_hist_data->p_mem;

        if (hist->rect_inside_hist) {
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->start.x; j < hist->end.x; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
        } else {
            /* compute histogram of the rect outside area */
            /* top part area */
            for (i = 0; i < hist->start.y; i++) {
                for (j = 0; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* left part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = 0; j < hist->start.x; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* right part area */
            for (i = hist->start.y; i < hist->end.y; i++) {
                for (j = hist->end.x; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
            /* bottom part area */
            for (i = hist->end.y; i < h; i++) {
                for (j = 0; j < w; j++) {
                    o[p[i*w + j] >> s] += (1L << 8);
                }
            }
        }
    }

    learning_rate = STP_INT_Q_MUL(hist->learning_rate, hist->learning_ratio);
    forgotting_rate.value = STP_FXP32(1.0f, learning_rate.q) - learning_rate.value;
    forgotting_rate.q = learning_rate.q;
    STP_ImageDotMulScalar(hist->new_hist_data, hist->new_hist_data, learning_rate);
    STP_ImageDotMulScalar(hist->hist_data, hist->hist_data, forgotting_rate);
    STP_ImageDotAdd(hist->hist_data, hist->hist_data, hist->new_hist_data);

    if (hist->hist_data->q != 8) {
        STP_ImageDotShift(hist->hist_data, hist->hist_data, hist->hist_data->q - 8);
        hist->hist_data->q = 8;
    }

    return STP_OK;
}

stp_ret STP_HistogramDestroy(stp_hist *hist) {
    if (hist) {
        STP_ImageDestroy(hist->hist_data);
        STP_ImageDestroy(hist->new_hist_data);
    }
    return STP_OK;
}
