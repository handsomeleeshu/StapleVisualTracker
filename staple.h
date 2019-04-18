/*
 * staple.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STAPLE_H_
#define _STAPLE_H_

#include "stp_tracker.h"
#include "stp_scaler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STP_MAX_REDETECT_AREA 12

typedef enum {
    STP_RGB_888,
    STP_BGR_888,
} Saple_ImageType;

typedef enum {
    STP_HIGH_PROFILE,
    STP_LOW_PROFILE,
} Staple_Profile;

typedef enum {
    STP_RUNING,
    STP_REDETECTING,
} Staple_State;

typedef struct {
    stp_size image_size;
    stp_pos target_pos;
    stp_size target_size;
    stp_size_q target_size_q;
    stp_size bg_size;
    stp_int32_q rgb_coef[3];
    float target_size_ratio;

    Staple_State state;
    Staple_Profile profile;

    stp_int32 frame_cnt;
    stp_bool dynamic_learning_ratio;

    stp_int32 redect_area_total, redetect_looper;
    stp_pos redetect_pos[STP_MAX_REDETECT_AREA];
    float long_term_resp, cur_resp;

    stp_tracker *tracker;
    stp_multichannel_image *tracker_work_image;
    stp_size tracker_work_size;
    stp_rect tracker_work_rect;

    stp_scaler *scaler;
} Staple;

stp_ret Staple_Init(Staple **p_staple, stp_multichannel_image *in_image, stp_rect target_rect);
stp_ret Staple_Run(Staple *staple, stp_multichannel_image *in_image);
Staple_State Staple_GetTargetRect(Staple *staple, stp_rect *rect);
stp_ret Staple_Destroy(Staple *staple);

#ifdef __cplusplus
}
#endif

#endif /* _STAPLE_H_ */
