/*
 * colour_map.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "stp_colour_map.h"

stp_ret STP_ColourMapFilterResponse(stp_image *out_resp, stp_image *in_resp, stp_rect tgt_rect) {
    stp_int32 i, j, h, w, w_o, hth, htw, x0, x1, y0, y1;
    stp_int32 *in, *o;

    h = in_resp->height;
    w = in_resp->width;
    in = (stp_int32*)in_resp->p_mem;
    o = (stp_int32*)out_resp->p_mem;
    w_o = out_resp->width;

    hth = tgt_rect.size.h >> 1; /* half target height */
    htw = tgt_rect.size.w >> 1; /* half target width */

    for (i = 1; i < h; i++) {
        in[i*w] += in[i*w - w];
    }
    for (i = 1; i < w; i++) {
        in[i] += in[i - 1];
    }

    for (i = 1; i < h; i++) {
        for (j = 1; j < w; j++) {
            in[i*w + j] += in[i*w + j - 1] + in[(i - 1)*w + j] - in[(i - 1)*w + j - 1];
        }
    }

    for (i = hth; i < h - hth; i++) {
        for (j = htw; j < w - htw; j++) {
            x0 = j - htw;
            x1 = j + htw - 1;
            y0 = i - hth;
            y1 = i + hth - 1;
            o[y0*w_o + x0] = in[y1*w + x1] - in[y1*w + x0] + in[y0*w + x0] - in[y0*w + x1];
        }
    }

    out_resp->q = in_resp->q;

    return STP_OK;
}

stp_ret STP_ColourMapInit(stp_colour_map **p_map, stp_multichannel_image *image,
        stp_rect tgt_rect, stp_int32 learning_rate_q30) {
    stp_ret ret;
    stp_colour_map *map;

    STP_Assert(p_map && image);
    map = (stp_colour_map *)STP_Calloc(1, sizeof(stp_colour_map));
    STP_Assert(map);

    stp_size image_size = {image->images[0]->width, image->images[0]->height};
    ret = STP_HistogramInit(&map->bg_hist, image, tgt_rect, STP_FALSE, learning_rate_q30);
    STP_Assert(ret == STP_OK);
    stp_rect fg_rect = {tgt_rect.pos,
            {tgt_rect.size.w*4/5, tgt_rect.size.h*4/5}};
    ret = STP_HistogramInit(&map->fg_hist, image, fg_rect, STP_TRUE, learning_rate_q30);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageInit(&map->bg_pwp, image_size);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageInit(&map->fg_pwp, image_size);
    STP_Assert(ret == STP_OK);
    stp_size pwp_size = {image_size.w - tgt_rect.size.w, image_size.h - tgt_rect.size.h};
    ret = STP_ImageInit(&map->pwp_response, pwp_size);
    STP_Assert(ret == STP_OK);

    map->bg_ratio.q = 30;
    map->bg_ratio.value = STP_FXP32(1.0f, 30)/(image_size.h*image_size.w - tgt_rect.size.h*tgt_rect.size.w);
    map->fg_ratio.q = 30;
    map->fg_ratio.value = STP_FXP32(1.0f, 30)/(fg_rect.size.h*fg_rect.size.w);
    map->pwp_ratio.q = 30;
    map->pwp_ratio.value = STP_FXP32(1.0f, 30)/(tgt_rect.size.h*tgt_rect.size.w);

    map->learning_rate.value = learning_rate_q30;
    map->learning_rate.q = 30;
    map->forgotting_rate.value = (1L << 30) - learning_rate_q30;
    map->forgotting_rate.q = 30;

    map->tgt_rect = tgt_rect;

    *p_map = map;
    return STP_OK;
}

stp_ret STP_ColourMapCompute(stp_colour_map *map, stp_multichannel_image *image) {
    stp_ret ret;
    stp_int32_q lambda;
    stp_int32_q scalar;

    STP_Assert(map && image);

    ret = STP_HistogramCompute(map->bg_hist, image, map->bg_pwp);
    STP_Assert(ret == STP_OK);
    ret = STP_HistogramCompute(map->fg_hist, image, map->fg_pwp);
    STP_Assert(ret == STP_OK);

    scalar = map->bg_ratio;
    /* ajust q to make bg_pwp more accurate */
    scalar.q -= 10;
    ret = STP_ImageDotMulScalar(map->bg_pwp, map->bg_pwp, scalar);
    STP_Assert(ret == STP_OK);
    scalar = map->fg_ratio;
    /* ajust q to make fg_pwp more accurate */
    scalar.q -= 10;
    ret = STP_ImageDotMulScalar(map->fg_pwp, map->fg_pwp, scalar);
    STP_Assert(ret == STP_OK);

    if (map->bg_pwp->q != map->fg_pwp->q) {
        STP_ImageDotShift(map->bg_pwp, map->bg_pwp, map->bg_pwp->q - map->fg_pwp->q);
        map->bg_pwp->q = map->fg_pwp->q;
    }

    ret = STP_ImageDotAdd(map->bg_pwp, map->bg_pwp, map->fg_pwp);
    STP_Assert(ret == STP_OK);

    lambda.q = map->bg_pwp->q;
    lambda.value = 1; /* eps */
    ret = STP_ImageDotDiv(map->bg_pwp, map->fg_pwp, map->bg_pwp, lambda);
    STP_Assert(ret == STP_OK);

    ret = STP_ColourMapFilterResponse(map->pwp_response, map->bg_pwp, map->tgt_rect);
    STP_Assert(ret == STP_OK);

    scalar = map->pwp_ratio;
    scalar.q -= 8;
    ret = STP_ImageDotMulScalar(map->pwp_response, map->pwp_response, scalar);
    map->pwp_response->q += 8;
    STP_Assert(ret == STP_OK);

    return STP_OK;
}

stp_ret STP_ColourMapGetResponse(stp_colour_map *map, stp_image **image) {
    STP_Assert(map && image);
    *image = map->pwp_response;
    return STP_OK;
}

stp_ret STP_ColourMapSetLearningRatio(stp_colour_map *map, stp_int32 learning_ratio_q24) {
    STP_Assert(map);
    STP_HistogramSetLearningRatio(map->fg_hist, learning_ratio_q24);
    STP_HistogramSetLearningRatio(map->bg_hist, learning_ratio_q24);
    return STP_OK;
}

stp_ret STP_ColourMapUpdate(stp_colour_map *map, stp_multichannel_image *image) {
    stp_ret ret;
    STP_Assert(map && image);

    ret = STP_HistogramUpdate(map->bg_hist, image);
    STP_Assert(ret == STP_OK);
    ret = STP_HistogramUpdate(map->fg_hist, image);
    STP_Assert(ret == STP_OK);

    return STP_OK;
}

stp_ret STP_ColourMapDestroy(stp_colour_map *map) {
    if (map) {
        STP_HistogramDestroy(map->bg_hist);
        STP_HistogramDestroy(map->fg_hist);

        STP_ImageDestroy(map->bg_pwp);
        STP_ImageDestroy(map->fg_pwp);
        STP_ImageDestroy(map->pwp_response);

        STP_Free(map);
    }
    return STP_OK;
}
