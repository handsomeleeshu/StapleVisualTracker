/*
 * stp_hist.h
 *
 *  Created on: 2018-9-19
 *      Author: handsomelee
 */

#ifndef _STP_HIST_H_
#define _STP_HIST_H_

#include "stp_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HIST_BIN_SHIFT (3)
#define HIST_BIN_WIDTH (256 >> HIST_BIN_SHIFT)

typedef struct {
    stp_bool gray_image;
    stp_bool rect_inside_hist;
    stp_int32_q learning_rate;
    stp_int32_q forgotting_rate;
    stp_int32_q learning_ratio;
    stp_pos start, end;
    stp_image *hist_data;
    stp_image *new_hist_data;
} stp_hist;

stp_ret STP_HistogramInit(stp_hist **p_hist, stp_multichannel_image *image,
        stp_rect rect, stp_bool inside, stp_int32 learning_rate_q30);
stp_ret STP_HistogramCompute(stp_hist *hist, stp_multichannel_image *image, stp_image *pwp);
stp_ret STP_HistogramSetLearningRatio(stp_hist *hist, stp_int32 learning_ratio_q24);
stp_ret STP_HistogramUpdate(stp_hist *hist, stp_multichannel_image *image);
stp_ret STP_HistogramDestroy(stp_hist *hist);

#ifdef __cplusplus
}
#endif

#endif /* _STP_HIST_H_ */
