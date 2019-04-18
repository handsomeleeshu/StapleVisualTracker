/*
 * feature_map.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_FEATURE_MAP_H_
#define _STP_FEATURE_MAP_H_

#include "stp_image.h"
#include "stp_fft.h"
#include "stp_fhog.h"
#ifdef _OPENMP
#include "stp_mp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    stp_fhog *fhog;
    stp_image *hogs[STP_MAX_HOG_DIMS];

    stp_fft *fft, *ifft;
#ifdef _OPENMP
    stp_fft *fft_mp[STP_MAX_MP];
#endif
    stp_complex_image *xtf[STP_MAX_HOG_DIMS];

    stp_image *hf_den, *new_hf_den, *float_hf_den;
#ifdef _OPENMP
    stp_image *new_hf_den_mp[STP_MAX_MP];
#endif
    stp_complex_image *hf_num[STP_MAX_HOG_DIMS], *new_hf_num[STP_MAX_HOG_DIMS], *float_hf_num[STP_MAX_HOG_DIMS];
    stp_complex_image *cf_response;

    stp_image hann_win;
    stp_complex_image *gauss_resp_fft_conj;

    stp_image *temp_image;
#ifdef _OPENMP
    stp_image *temp_image_mp[STP_MAX_MP];
#endif
    stp_complex_image *temp_c_image;

    stp_rect target_rect;

    stp_int32_q learning_rate, forgotting_rate, lambda;
    stp_int32_q learning_ratio;
} stp_feature_map;

stp_ret STP_FeatureMapInit(stp_feature_map **p_map, stp_multichannel_image *image,
        stp_rect target_rect, stp_int32 learning_rate_q30);
stp_ret STP_FeatureMapCompute(stp_feature_map *map, stp_multichannel_image *image);
stp_ret STP_FeatureMapGetResponse(stp_feature_map *map, stp_image **resp);
stp_ret STP_FeatureMapSetLearningRatio(stp_feature_map *map, stp_int32 learning_ratio_q24);
stp_ret STP_FeatureMapUpdate(stp_feature_map *map, stp_multichannel_image *image);
stp_ret STP_FeatureMapDestroy(stp_feature_map *map);

#ifdef __cplusplus
}
#endif

#endif /* _STP_FEATURE_MAP_H_ */
