/*
 * stp_fft.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_FFT_H_
#define _STP_FFT_H_

#include "stp_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STP_MAX_FFT_LAYER       8 /* 256*256 2d fft supported */
#define STP_MAX_FFT_W_H_RATIO   2

typedef struct {
    stp_bool inverse;
    stp_bool complex_input;
    stp_bool one_dimension;
    stp_bool square_input;
    stp_size size;
    stp_int32 layers;
    stp_int32 sub_blocks;

    stp_image *zero_image;
    stp_complex_image *temp_image0, *temp_image1;
    stp_complex_image *square_images[STP_MAX_FFT_W_H_RATIO];
    stp_complex_image *prefft_coef;
    stp_complex_image *reorder_image;

    stp_int16 *reorder_table;
    stp_complex_image *coef_tables[STP_MAX_FFT_LAYER];
} stp_fft;

stp_ret STP_FFTInit(stp_fft **p_fft, stp_size size, stp_bool complex_in, stp_bool inverse);
stp_ret STP_FFTCompute(stp_fft *fft, void *in_image,
        stp_complex_image *out_image);
stp_ret STP_FFTDestroy(stp_fft *fft);

#ifdef __cplusplus
}
#endif

#endif /* _STP_FFT_H_ */
