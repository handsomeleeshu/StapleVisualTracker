/*
 * stp_tracker.h
 *
 *  Created on: 2018-9-21
 *      Author: handsomelee
 */

#ifndef _STP_TRACKER_H_
#define _STP_TRACKER_H_

#include "stp_type.h"
#include "stp_hist.h"
#include "stp_image.h"
#include "stp_feature_map.h"
#include "stp_colour_map.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    stp_size patch_sz;
    stp_rect target_rect;

    stp_image *temp_image, *resp_filter;
    stp_feature_map *feature_map;
    stp_colour_map *colour_map;
    stp_image *merged_response, *temp_response, *filtered_cf_response;
    stp_int32_q cf_merge_ratio;
    stp_int32_q pwp_merge_ratio;

    stp_int32 cf_learning_rate_q30;
    stp_int32 pwp_learning_rate_q30;

    stp_pos pos;
} stp_tracker;

stp_ret STP_TrackerInit(stp_tracker **p_tracker, stp_multichannel_image *image,
        stp_rect tgt_rect, stp_int32 cf_learning_rate_q30, stp_int32 pwp_learning_rate_q30);
stp_ret STP_TrackerRun(stp_tracker *tracker, stp_multichannel_image *image);
stp_ret STP_TrackerGetResponseMap(stp_tracker *tracker, stp_image **resp);
stp_ret STP_TrackerSetLearningRatio(stp_tracker *tracker, stp_int32 learning_ratio_q24);
stp_ret STP_TrackerUpdate(stp_tracker *tracker, stp_multichannel_image *image);
stp_ret STP_TrackerDestroy(stp_tracker *tracker);

#ifdef __cplusplus
}
#endif

#endif /* _STP_TRACKER_H_ */
