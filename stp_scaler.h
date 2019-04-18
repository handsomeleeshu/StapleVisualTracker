/*
 * stp_scaler.h
 *
 *  Created on: 2018-9-21
 *      Author: handsomelee
 */
#ifndef _STP_SCALER_H_
#define _STP_SCALER_H_

#include "stp_type.h"
#include "stp_image.h"
#include "stp_fft.h"
#include "stp_fhog.h"
#ifdef _OPENMP
#include "stp_mp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define STP_NUM_SCALES (32)
#define STP_MAX_SCALE_MODEL_AREA (512)
#define STP_MAX_HOG_PIXELS ((STP_MAX_SCALE_MODEL_AREA >> 4)*STP_MAX_HOG_DIMS)

typedef struct {
    stp_bool enable;

    stp_int32_q scale_step;
    stp_int32_q cur_scale_factor;
    stp_int32_q scale_factors[STP_NUM_SCALES + 1];
    stp_int32_q min_scale_factor;
    stp_int32_q max_scale_factor;

    stp_size_q base_target_size;
    stp_size_q cur_target_size;

    stp_int32_q learning_rate, forgotting_rate;
    stp_int32_q learning_ratio;

    stp_fft *fft, *ifft;
#ifdef _OPENMP
    stp_fft *fft_mp[STP_MAX_MP];
#endif
    stp_image *fft_temp_in;
#ifdef _OPENMP
    stp_image *fft_temp_in_mp[STP_MAX_MP];
#endif
    stp_complex_image *xsf[STP_MAX_HOG_PIXELS];

    stp_fhog *fhog[STP_NUM_SCALES];
    stp_image *hogs[STP_NUM_SCALES][STP_MAX_HOG_DIMS];

    stp_size scale_model_size;

    stp_multichannel_image *model_images;
#ifdef _OPENMP
    stp_multichannel_image *model_images_mp[STP_MAX_MP];
#endif

    stp_int32_q x_clip_ratio, y_clip_ratio;

    stp_image *sf_den, *float_sf_den, *new_sf_den, *temp_sum_den;
#ifdef _OPENMP
    stp_image *new_sf_den_mp[STP_MAX_MP], *temp_sum_den_mp[STP_MAX_MP];
#endif

    stp_complex_image *sf_num[STP_MAX_HOG_PIXELS], *float_sf_num[STP_MAX_HOG_PIXELS], *new_sf_num[STP_MAX_HOG_PIXELS], *temp_sum_num;
#ifdef _OPENMP
    stp_complex_image *temp_sum_num_mp[STP_MAX_MP];
#endif
    stp_complex_image *ysf;
    const stp_image *hann_win;
    stp_int32_q lambda;
} stp_scaler;


stp_ret STP_ScalerInit(stp_scaler **p_scaler, stp_multichannel_image *image, stp_rect_q tgt_rect, stp_int32 learning_rate_q30);
stp_ret STP_ScalerRun(stp_scaler *scaler, stp_multichannel_image *image, stp_rect_q tgt_rect);
stp_ret STP_ScalerGetTargetSize(stp_scaler *scaler, stp_size_q *size);
stp_ret STP_ScalerSetLearningRatio(stp_scaler *scaler, stp_int32 learning_ratio_q24);
stp_ret STP_ScalerUpdate(stp_scaler *scaler, stp_multichannel_image *image, stp_rect_q tgt_rect);
stp_ret STP_ScalerDestroy(stp_scaler *scaler);

#ifdef __cplusplus
}
#endif

#endif /* _STP_SCALER_H_ */
