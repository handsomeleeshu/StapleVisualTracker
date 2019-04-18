/*
 * colour_map.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_COLOUR_MAP_H_
#define _STP_COLOUR_MAP_H_

#include "stp_image.h"
#include "stp_hist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    stp_hist *bg_hist;
    stp_hist *fg_hist;
    stp_image *bg_pwp, *fg_pwp;
    stp_image *pwp_response;
    stp_rect tgt_rect;

    stp_int32_q learning_rate, forgotting_rate;
    stp_int32_q bg_ratio, fg_ratio, pwp_ratio;
} stp_colour_map;

stp_ret STP_ColourMapInit(stp_colour_map **p_map, stp_multichannel_image *image,
        stp_rect tgt_rect, stp_int32 learning_rate_q30);
stp_ret STP_ColourMapCompute(stp_colour_map *map, stp_multichannel_image *image);
stp_ret STP_ColourMapGetResponse(stp_colour_map *map, stp_image **image);
stp_ret STP_ColourMapSetLearningRatio(stp_colour_map *map, stp_int32 learning_ratio_q24);
stp_ret STP_ColourMapUpdate(stp_colour_map *map, stp_multichannel_image *image);
stp_ret STP_ColourMapDestroy(stp_colour_map *map);

#ifdef __cplusplus
}
#endif

#endif /* _STP_COLOUR_MAP_H_ */
