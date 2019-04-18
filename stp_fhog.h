/*
 * fhog.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_FHOG_H_
#define _STP_FHOG_H_

#include "stp_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STP_MAX_HOG_DIMS (31)

typedef enum {
    STP_HOGDIM_18,
    STP_HOGDIM_18_9,
    STP_HOGDIM_18_9_1,
    STP_HOGDIM_18_9_3,
    STP_HOGDIM_18_9_4
} stp_hog_type;

typedef struct {
    stp_hog_type hog_type;
    stp_int32 hog_dims;
    stp_int32 max_grad;
    stp_size imag_sz;
    stp_size cell_sz;
    stp_bool is_clip;
    stp_int32_q clip_value;

    stp_image *x_grad, *y_grad, *x_grad_temp, *y_grad_temp;
    stp_image *norm[4], *temp[4];
    const stp_image *angle_table, *norm_table;
} stp_fhog;

stp_ret STP_FhogInit(stp_fhog **p_fhog, stp_size imag_size, stp_size cell_size, stp_hog_type hog_type);
stp_ret STP_FhogCompute(stp_fhog *fhog, stp_multichannel_image *image, stp_image *out_hogs[]);
stp_ret STP_FhogGetDims(stp_fhog *fhog, stp_int32 *dims);
stp_ret STP_FhogDestroy(stp_fhog *fhog);

#ifdef __cplusplus
}
#endif

#endif /* _STP_FHOG_H_ */
