/*
 * feature_map.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "stp_feature_map.h"
#include "stp_table.h"
#ifdef _OPENMP
#include <omp.h>
#endif

stp_ret STP_FeatureMapInit(stp_feature_map **p_map, stp_multichannel_image *image,
        stp_rect target_rect, stp_int32 learning_rate_q30) {
    stp_int32 i, hog_dims;
    stp_ret ret = STP_OK;
    stp_size imag_sz = {image->images[0]->width, image->images[0]->height}, hog_cell_sz = {4, 4},
            hog_sz = {image->images[0]->width/hog_cell_sz.w, image->images[0]->height/hog_cell_sz.h};
    stp_image gauss_resp_image;

    STP_Assert(p_map);
    stp_feature_map *map = (stp_feature_map *)STP_Calloc(1, sizeof(stp_feature_map));
    STP_Assert(map);

    map->target_rect = target_rect;

    if (hog_sz.w == 64 && hog_sz.h == 64) {
        map->hann_win = hann_win_64_64_imag;
        gauss_resp_image = gauss_resp_64_64_imag;
    } else if (hog_sz.w == 64 && hog_sz.h == 32) {
        map->hann_win = hann_win_64_32_imag;
        gauss_resp_image = gauss_resp_64_32_imag;
    } else if (hog_sz.w == 32 && hog_sz.h == 64) {
        map->hann_win = hann_win_32_64_imag;
        gauss_resp_image = gauss_resp_32_64_imag;
    } else if (hog_sz.w == 32 && hog_sz.h == 32) {
        map->hann_win = hann_win_32_32_imag;
        gauss_resp_image = gauss_resp_32_32_imag;
    } else {
        return STP_ERR_NOT_IMPL;
    }

    /* pre-scale hann-win */
    map->hann_win.q -= 8;

    ret = STP_FhogInit(&map->fhog, imag_sz, hog_cell_sz, STP_HOGDIM_18_9_3);
    STP_Assert(ret == STP_OK);
    ret = STP_FhogGetDims(map->fhog, &hog_dims);
    STP_Assert(ret == STP_OK);
    ret = STP_FFTInit(&map->fft, hog_sz, STP_FALSE, STP_FALSE);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    int mp_i;
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_FFTInit(&map->fft_mp[mp_i], hog_sz, STP_FALSE, STP_FALSE);
        STP_Assert(ret == STP_OK);
    }
#endif
    ret = STP_FFTInit(&map->ifft, hog_sz, STP_TRUE, STP_TRUE);
    STP_Assert(ret == STP_OK);

    STP_ComplexImageInit(&map->gauss_resp_fft_conj, hog_sz);
    gauss_resp_image.q -= 8;
    ret = STP_FFTCompute(map->fft, &gauss_resp_image, map->gauss_resp_fft_conj);
    map->gauss_resp_fft_conj->real->q += 8;
    map->gauss_resp_fft_conj->imag->q += 8;
    STP_Assert(ret == STP_OK);
    ret = STP_ComplexImageConj(map->gauss_resp_fft_conj);
    STP_Assert(ret == STP_OK);

    STP_ImageInit(&map->hf_den, hog_sz);
    STP_ImageInit(&map->new_hf_den, hog_sz);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        STP_ImageInit(&map->new_hf_den_mp[mp_i], hog_sz);
    }
#endif
    STP_ImageInit(&map->float_hf_den, hog_sz);
    STP_ImageSetDataType(map->float_hf_den, STP_FLOAT32);
    for (i = 0; i < hog_dims; i++) {
        STP_ImageInit(&map->hogs[i], hog_sz);
        STP_ComplexImageInit(&map->hf_num[i], hog_sz);
        STP_ComplexImageInit(&map->new_hf_num[i], hog_sz);
        STP_ComplexImageInit(&map->float_hf_num[i], hog_sz);
        STP_ComplexImageSetDataType(map->float_hf_num[i], STP_FLOAT32);
        STP_ComplexImageInit(&map->xtf[i], hog_sz);
    }

    STP_ComplexImageInit(&map->cf_response, hog_sz);
    STP_ImageInit(&map->temp_image, hog_sz);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        STP_ImageInit(&map->temp_image_mp[mp_i], hog_sz);
    }
#endif
    STP_ComplexImageInit(&map->temp_c_image, hog_sz);

    map->learning_ratio.value = STP_FXP32(1.0f, 24);
    map->learning_ratio.q = 24;
    map->learning_rate.value = learning_rate_q30;
    map->learning_rate.q = 30;
    map->forgotting_rate.value = (1L << 30) - learning_rate_q30;
    map->forgotting_rate.q = 30;
    map->lambda.q = 15;
    map->lambda.value = STP_FXP32(0.001f, map->lambda.q);

    ret = STP_FhogCompute(map->fhog, image, map->hogs);
    STP_Assert(ret == STP_OK);

    for (i = 0; i < hog_dims; i++) {
        ret = STP_ImageDotMul(map->temp_image, map->hogs[i], &map->hann_win);
        STP_Assert(ret == STP_OK);
        ret = STP_FFTCompute(map->fft, map->temp_image, map->xtf[i]);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotMulC(map->hf_num[i],
                map->gauss_resp_fft_conj, map->xtf[i]);
        STP_Assert(ret == STP_OK);

        /* restore scale caused by hann-win pre-scale */
        map->hf_num[i]->real->q += 8;
        map->hf_num[i]->imag->q += 8;
        STP_ComplexImageConvertDataType(map->float_hf_num[i], map->hf_num[i], STP_FLOAT32);

        map->xtf[i]->real->q += 7;
        map->xtf[i]->imag->q += 7;

        if (i == 0) {
            ret = STP_ComplexImageDotEnergy(map->hf_den, map->xtf[i]);
            STP_Assert(ret == STP_OK);
        } else {
            ret = STP_ComplexImageDotEnergy(map->temp_image, map->xtf[i]);
            STP_Assert(ret == STP_OK);
            ret = STP_ImageDotAdd(map->hf_den, map->hf_den, map->temp_image);
            STP_Assert(ret == STP_OK);
        }
    }

    map->hf_den->q += 2;
    STP_ImageConvertDataType(map->float_hf_den, map->hf_den, STP_FLOAT32);

    *p_map = map;

    return ret;
}

stp_ret STP_FeatureMapCompute(stp_feature_map *map, stp_multichannel_image *image) {
    stp_int32 i, hog_dims;
    stp_ret ret = STP_OK;
    STP_Assert(map && image);

    ret = STP_FhogGetDims(map->fhog, &hog_dims);
    STP_Assert(ret == STP_OK);

    ret = STP_FhogCompute(map->fhog, image, map->hogs);
    STP_Assert(ret == STP_OK);

#ifdef _OPENMP
    int tid, first_i[STP_MAX_MP];
    int mp_i;
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        first_i[mp_i] = -1;
    }
#pragma omp parallel for private(tid) num_threads(STP_MAX_MP)
    for (i = 0; i < hog_dims; i++) {
        tid = omp_get_thread_num();
        if (first_i[tid] < 0) first_i[tid] = i;
        ret = STP_ImageDotMul(map->temp_image_mp[tid], map->hogs[i], &map->hann_win);
        STP_Assert(ret == STP_OK);
        ret = STP_FFTCompute(map->fft_mp[tid], map->temp_image_mp[tid], map->xtf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConvertDataType(map->hf_num[i], map->float_hf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConj(map->hf_num[i]);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotMulC(map->new_hf_num[i],
                map->hf_num[i], map->xtf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConj(map->hf_num[i]);
        STP_Assert(ret == STP_OK);
        if (i > first_i[tid]) {
            ret = STP_ComplexImageDotAddC(map->new_hf_num[first_i[tid]],
                    map->new_hf_num[first_i[tid]], map->new_hf_num[i]);
            STP_Assert(ret == STP_OK);
        }
    }
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        if (first_i[mp_i] > 0)
            STP_ComplexImageDotAddC(map->new_hf_num[0], map->new_hf_num[0], map->new_hf_num[first_i[mp_i]]);
    }
#else
    for (i = 0; i < hog_dims; i++) {
        ret = STP_ImageDotMul(map->temp_image, map->hogs[i], &map->hann_win);
        STP_Assert(ret == STP_OK);
        ret = STP_FFTCompute(map->fft, map->temp_image, map->xtf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConvertDataType(map->hf_num[i], map->float_hf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConj(map->hf_num[i]);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotMulC(map->new_hf_num[i],
                map->hf_num[i], map->xtf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConj(map->hf_num[i]);
        STP_Assert(ret == STP_OK);
        if (i > 0) {
            ret = STP_ComplexImageDotAddC(map->new_hf_num[0],
                    map->new_hf_num[0], map->new_hf_num[i]);
            STP_Assert(ret == STP_OK);
        }
    }
#endif

    ret = STP_ImageConvertDataType(map->hf_den, map->float_hf_den, STP_INT32);
    STP_Assert(ret == STP_OK);

    ret = STP_ComplexImageDotDiv(map->temp_c_image,
            map->new_hf_num[0], map->hf_den, map->lambda);
    STP_Assert(ret == STP_OK);

    ret = STP_FFTCompute(map->ifft, map->temp_c_image, map->cf_response);
    STP_Assert(ret == STP_OK);

    /* restore scale caused by hann-win pre-scale */
    map->cf_response->real->q += 8;
    map->cf_response->imag->q += 8;

    return ret;
}

stp_ret STP_FeatureMapGetResponse(stp_feature_map *map, stp_image **resp) {
    stp_ret ret = STP_OK;
    STP_Assert(map && resp);

    *resp = map->cf_response->real;
    return ret;
}

stp_ret STP_FeatureMapSetLearningRatio(stp_feature_map *map, stp_int32 learning_ratio_q24) {
    STP_Assert(map);
    map->learning_ratio.q = 24;
    map->learning_ratio.value = learning_ratio_q24;
    return STP_OK;
}

stp_ret STP_FeatureMapUpdate(stp_feature_map *map, stp_multichannel_image *image) {
    stp_int32 i, hog_dims;
    stp_int32_q learning_rate;
    stp_ret ret = STP_OK;
    float coef[2];
    STP_Assert(map && image);

    learning_rate = STP_INT_Q_MUL(map->learning_rate, map->learning_ratio);
    coef[1] = (float)learning_rate.value/(1L << learning_rate.q);
    coef[0] = 1.0f - coef[1];

    ret = STP_FhogGetDims(map->fhog, &hog_dims);
    STP_Assert(ret == STP_OK);

    ret = STP_FhogCompute(map->fhog, image, map->hogs);
    STP_Assert(ret == STP_OK);

#ifdef _OPENMP
    int tid, first_i[STP_MAX_MP];
    int mp_i;
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        first_i[mp_i] = -1;
    }
#pragma omp parallel for private(tid) num_threads(STP_MAX_MP)
    for (i = 0; i < hog_dims; i++) {
        tid = omp_get_thread_num();
        if (first_i[tid] < 0) first_i[tid] = i;

        ret = STP_ImageDotMul(map->temp_image_mp[tid], map->hogs[i], &map->hann_win);
        STP_Assert(ret == STP_OK);

        ret = STP_FFTCompute(map->fft_mp[tid], map->temp_image_mp[tid], map->xtf[i]);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotMulC(map->new_hf_num[i],
                map->gauss_resp_fft_conj, map->xtf[i]);
        STP_Assert(ret == STP_OK);

        /* restore scale caused by hann-win pre-scale */
        map->new_hf_num[i]->real->q += 8;
        map->new_hf_num[i]->imag->q += 8;
        map->xtf[i]->real->q += 7;
        map->xtf[i]->imag->q += 7;

        if (i == first_i[tid]) {
            ret = STP_ComplexImageDotEnergy(map->new_hf_den_mp[tid], map->xtf[i]);
            STP_Assert(ret == STP_OK);
        } else {
            ret = STP_ComplexImageDotEnergy(map->temp_image_mp[tid], map->xtf[i]);
            STP_Assert(ret == STP_OK);
            ret = STP_ImageDotAdd(map->new_hf_den_mp[tid], map->new_hf_den_mp[tid], map->temp_image_mp[tid]);
            STP_Assert(ret == STP_OK);
        }

        ret = STP_ComplexImageConvertDataType(map->new_hf_num[i], map->new_hf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageCoefMix(map->float_hf_num[i], map->float_hf_num[i], map->new_hf_num[i], coef);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageSetDataType(map->new_hf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
    }
    STP_ImageClear(map->new_hf_den);
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        if (first_i[mp_i] >= 0) {
            map->new_hf_den->q = map->new_hf_den_mp[mp_i]->q;
            STP_ImageDotAdd(map->new_hf_den, map->new_hf_den, map->new_hf_den_mp[mp_i]);
        }
    }
#else
    for (i = 0; i < hog_dims; i++) {
        ret = STP_ImageDotMul(map->temp_image, map->hogs[i], &map->hann_win);
        STP_Assert(ret == STP_OK);

        ret = STP_FFTCompute(map->fft, map->temp_image, map->xtf[i]);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotMulC(map->new_hf_num[i],
                map->gauss_resp_fft_conj, map->xtf[i]);
        STP_Assert(ret == STP_OK);

        /* restore scale caused by hann-win pre-scale */
        map->new_hf_num[i]->real->q += 8;
        map->new_hf_num[i]->imag->q += 8;
        map->xtf[i]->real->q += 7;
        map->xtf[i]->imag->q += 7;

        if (i == 0) {
            ret = STP_ComplexImageDotEnergy(map->new_hf_den, map->xtf[i]);
            STP_Assert(ret == STP_OK);
        } else {
            ret = STP_ComplexImageDotEnergy(map->temp_image, map->xtf[i]);
            STP_Assert(ret == STP_OK);
            ret = STP_ImageDotAdd(map->new_hf_den, map->new_hf_den, map->temp_image);
            STP_Assert(ret == STP_OK);
        }

        ret = STP_ComplexImageConvertDataType(map->new_hf_num[i], map->new_hf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageCoefMix(map->float_hf_num[i], map->float_hf_num[i], map->new_hf_num[i], coef);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageSetDataType(map->new_hf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
    }
#endif
    map->new_hf_den->q += 2;

    ret = STP_ImageConvertDataType(map->new_hf_den, map->new_hf_den, STP_FLOAT32);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageCoefMix(map->float_hf_den, map->float_hf_den, map->new_hf_den, coef);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageSetDataType(map->new_hf_den, STP_INT32);
    STP_Assert(ret == STP_OK);

    return ret;
}

stp_ret STP_FeatureMapDestroy(stp_feature_map *map) {
    stp_int32 i, hog_dims;
    stp_ret ret = STP_OK;

    if (map) {
        ret = STP_FhogGetDims(map->fhog, &hog_dims);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogDestroy(map->fhog);
        STP_Assert(ret == STP_OK);
        ret = STP_FFTDestroy(map->fft);
        STP_Assert(ret == STP_OK);
#ifdef _OPENMP
        int mp_i;
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            ret = STP_FFTDestroy(map->fft_mp[mp_i]);
            STP_Assert(ret == STP_OK);
        }
#endif
        ret = STP_FFTDestroy(map->ifft);
        STP_Assert(ret == STP_OK);

        for (i = 0; i < hog_dims; i++) {
            STP_ImageDestroy(map->hogs[i]);
            STP_ComplexImageDestroy(map->hf_num[i]);
            STP_ComplexImageDestroy(map->new_hf_num[i]);
            STP_ComplexImageDestroy(map->float_hf_num[i]);
            STP_ComplexImageDestroy(map->xtf[i]);
        }

        STP_ImageDestroy(map->hf_den);
        STP_ImageDestroy(map->new_hf_den);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(map->new_hf_den_mp[mp_i]);
        }
#endif
        STP_ImageDestroy(map->float_hf_den);
        STP_ImageDestroy(map->temp_image);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(map->temp_image_mp[mp_i]);
        }
#endif

        STP_ComplexImageDestroy(map->temp_c_image);
        STP_ComplexImageDestroy(map->cf_response);

        STP_Free(map);
    }

    return ret;
}
