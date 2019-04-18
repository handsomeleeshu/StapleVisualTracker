/*
 * stp_tracker.c
 *
 *  Created on: 2018-9-21
 *      Author: handsomelee
 */

#include "stp_tracker.h"
#include "stp_type.h"

stp_ret STP_TrackerInit(stp_tracker **p_tracker, stp_multichannel_image *image,
        stp_rect tgt_rect, stp_int32 cf_learning_rate_q30, stp_int32 pwp_learning_rate_q30) {
    stp_int32 i, *p_mem;
    stp_ret ret = STP_OK;
    stp_size patch_sz, resp_size, size;
    stp_tracker *tracker;

    tracker = (stp_tracker *)STP_Calloc(1, sizeof(stp_tracker));
    STP_Assert(tracker && image && image->images[0]);

    patch_sz.h = image->images[0]->height;
    patch_sz.w = image->images[0]->width;
    STP_Assert(STP_POWERS_OF_2(patch_sz.h) && STP_POWERS_OF_2(patch_sz.w));

    /* target size must be even */
    tgt_rect.size.w = 0.75*patch_sz.w - 0.25*patch_sz.h;
    tgt_rect.size.h = 0.75*patch_sz.h - 0.25*patch_sz.w;
    tgt_rect.size.h &= 0xfffffffe;
    tgt_rect.size.w &= 0xfffffffe;

    resp_size.w = patch_sz.w - tgt_rect.size.w;
    resp_size.h = patch_sz.h - tgt_rect.size.h;

    tracker->patch_sz = patch_sz;
    tracker->target_rect = tgt_rect;
    tracker->cf_merge_ratio.q = 30;
    tracker->cf_merge_ratio.value = STP_FXP32(0.5f, 30);
    tracker->pwp_merge_ratio.q = 30;
    tracker->pwp_merge_ratio.value = STP_FXP32(0.5f, 30);

    ret = STP_ImageInit(&tracker->temp_image, tracker->patch_sz);
    STP_Assert(ret == STP_OK);

    ret = STP_FeatureMapInit(&tracker->feature_map, image, tracker->target_rect, cf_learning_rate_q30);
    STP_Assert(ret == STP_OK);
    tracker->cf_learning_rate_q30 = cf_learning_rate_q30;

    ret = STP_ColourMapInit(&tracker->colour_map, image, tracker->target_rect, pwp_learning_rate_q30);
    STP_Assert(ret == STP_OK);
    tracker->pwp_learning_rate_q30 = pwp_learning_rate_q30;

    ret = STP_ImageInit(&tracker->merged_response, resp_size);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageInit(&tracker->filtered_cf_response, resp_size);
    STP_Assert(ret == STP_OK);

    size.h = 5;
    size.w = 5;
    ret = STP_ImageInit(&tracker->resp_filter, size);
    STP_Assert(ret == STP_OK);
    p_mem = (stp_int32 *)tracker->resp_filter->p_mem;
    for (i = 0; i < size.h*size.w; i++) {
        p_mem[i] = (1L << tracker->resp_filter->q)/(size.h*size.w);
    }

    resp_size.h -= size.h;
    resp_size.w -= size.w;
    ret = STP_ImageInit(&tracker->temp_response, resp_size);
    STP_Assert(ret == STP_OK);

    *p_tracker = tracker;
    return ret;
}

stp_ret STP_TrackerRun(stp_tracker *tracker, stp_multichannel_image *image) {
    stp_ret ret = STP_OK;
    stp_image *cf_resp, *pwp_resp;
    stp_rect_q rect;
    STP_Assert(tracker && image);

    ret = STP_FeatureMapCompute(tracker->feature_map, image);
    STP_Assert(ret == STP_OK);
    ret = STP_ColourMapCompute(tracker->colour_map, image);
    STP_Assert(ret == STP_OK);

    ret = STP_FeatureMapGetResponse(tracker->feature_map, &cf_resp);
    STP_Assert(ret == STP_OK);
    ret = STP_ColourMapGetResponse(tracker->colour_map, &pwp_resp);
    STP_Assert(ret == STP_OK);

    /* crop cf center response and resize x4 up (hog cell size) */
    rect.pos.pos.x = cf_resp->width*(1L << 14);
    rect.pos.pos.y = cf_resp->height*(1L << 14);
    rect.pos.q = 15;
    rect.size.size.h = tracker->temp_response->height*(1L << (15 - 2));
    rect.size.size.w = tracker->temp_response->width*(1L << (15 - 2));
    rect.size.q = 15;
    ret = STP_ImageResize(tracker->temp_response, cf_resp, &rect);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageFilter(tracker->filtered_cf_response, tracker->temp_response, tracker->resp_filter);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageDotMulScalar(tracker->filtered_cf_response, tracker->filtered_cf_response, tracker->cf_merge_ratio);
    STP_Assert(ret == STP_OK);


    ret = STP_ImageDotMulScalar(tracker->merged_response, pwp_resp, tracker->pwp_merge_ratio);
    STP_Assert(ret == STP_OK);
    /* align q with cf response  */
    if (tracker->merged_response->q != tracker->filtered_cf_response->q) {
        ret = STP_ImageDotShift(tracker->merged_response, tracker->merged_response, tracker->merged_response->q - tracker->filtered_cf_response->q);
        STP_Assert(ret == STP_OK);
        tracker->merged_response->q = tracker->filtered_cf_response->q;
    }
    ret = STP_ImageDotAdd(tracker->merged_response, tracker->merged_response, tracker->filtered_cf_response);
    STP_Assert(ret == STP_OK);

    return ret;
}

stp_ret STP_TrackerGetResponseMap(stp_tracker *tracker, stp_image **resp) {
    stp_ret ret = STP_OK;
    STP_Assert(tracker && resp);
    *resp = tracker->merged_response;
    return ret;
}

stp_ret STP_TrackerSetLearningRatio(stp_tracker *tracker, stp_int32 learning_ratio_q24) {
    stp_ret ret = STP_OK;
    STP_Assert(tracker);
    ret = STP_FeatureMapSetLearningRatio(tracker->feature_map, learning_ratio_q24);
    STP_Assert(ret == STP_OK);
    ret = STP_ColourMapSetLearningRatio(tracker->colour_map, learning_ratio_q24);
    STP_Assert(ret == STP_OK);
    return ret;
}

stp_ret STP_TrackerUpdate(stp_tracker *tracker, stp_multichannel_image *image) {
    stp_ret ret = STP_OK;

    STP_Assert(tracker && image);

    ret = STP_FeatureMapUpdate(tracker->feature_map, image);
    STP_Assert(ret == STP_OK);
    ret = STP_ColourMapUpdate(tracker->colour_map, image);
    STP_Assert(ret == STP_OK);

    return ret;
}

stp_ret STP_TrackerDestroy(stp_tracker *tracker) {
    stp_ret ret = STP_OK;

    if (tracker) {
        ret = STP_FeatureMapDestroy(tracker->feature_map);
        STP_Assert(ret == STP_OK);
        ret = STP_ColourMapDestroy(tracker->colour_map);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDestroy(tracker->merged_response);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDestroy(tracker->filtered_cf_response);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDestroy(tracker->resp_filter);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDestroy(tracker->temp_response);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDestroy(tracker->temp_image);
        STP_Assert(ret == STP_OK);

        STP_Free(tracker);
    }

    return ret;
}
