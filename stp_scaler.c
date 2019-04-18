/*
 * stp_scaler.c
 *
 *  Created on: 2018-9-21
 *      Author: handsomelee
 */
#include "stp_scaler.h"
#include "stp_table.h"
#include "math.h"
#ifdef _OPENMP
#include <omp.h>
#endif

stp_ret STP_ScalerInit(stp_scaler **p_scaler, stp_multichannel_image *image, stp_rect_q tgt_rect, stp_int32 learning_rate_q30) {
    stp_int32 i, j, k, cnt = 0, hog_dims;
    float ratio;
    stp_ret ret = STP_OK;
    stp_size fft_size;
    stp_size hog_cell_sz = {4, 4}, hog_sz;
    stp_rect_q rect;
    stp_image gauss_resp;
    STP_Assert(p_scaler && image);

    stp_scaler *scaler = (stp_scaler *)STP_Calloc(1, sizeof(stp_scaler));
    STP_Assert(scaler);

    ratio = STP_MAX_SCALE_MODEL_AREA/((float)((stp_int64)tgt_rect.size.size.h*tgt_rect.size.size.w >> 2*tgt_rect.size.q));

    ratio = STP_MIN(1.0f, sqrt(ratio));

    scaler->scale_model_size.h = (stp_int32)(tgt_rect.size.size.h*ratio) >> tgt_rect.size.q;
    scaler->scale_model_size.h = scaler->scale_model_size.h & 0xfffffffc;
    scaler->scale_model_size.w = STP_MAX_SCALE_MODEL_AREA/scaler->scale_model_size.h;
    scaler->scale_model_size.w = scaler->scale_model_size.w & 0xfffffffc;

    fft_size.w = STP_NUM_SCALES;
    fft_size.h = 1;

    hog_sz.w = scaler->scale_model_size.w/hog_cell_sz.w;
    hog_sz.h = scaler->scale_model_size.h/hog_cell_sz.h;

    scaler->scale_step.q = 30;
    scaler->scale_step.value = STP_FXP32(1.028f, 30);
    scaler->scale_factors[STP_NUM_SCALES/2].q = 30;
    scaler->scale_factors[STP_NUM_SCALES/2].value = STP_FXP32(1.0f, 30);

    for (i = 1; i <= STP_NUM_SCALES/2; i++) {
        scaler->scale_factors[STP_NUM_SCALES/2 + i] =
                STP_INT_Q_DIV(scaler->scale_factors[STP_NUM_SCALES/2 + i - 1], scaler->scale_step);
        scaler->scale_factors[STP_NUM_SCALES/2 - i] =
                STP_INT_Q_MUL(scaler->scale_factors[STP_NUM_SCALES/2 - i + 1], scaler->scale_step);
    }

    scaler->base_target_size = tgt_rect.size;
    scaler->cur_target_size = tgt_rect.size;
    scaler->cur_scale_factor.q = 24;
    scaler->cur_scale_factor.value = STP_FXP32(1.0f, scaler->cur_scale_factor.q);

    /* FIX ME */
    /* simple way to compute max and min scale factors */
    scaler->max_scale_factor.q = 15;
    scaler->max_scale_factor.value = STP_FXP32(720.0f/(tgt_rect.size.size.h >> tgt_rect.size.q), 15);
    scaler->min_scale_factor.q = 15;
    scaler->min_scale_factor.value = STP_FXP32(20.0f/(tgt_rect.size.size.h >> tgt_rect.size.q), 15);

    ret = STP_FFTInit(&scaler->fft, fft_size, STP_FALSE, STP_FALSE);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    int mp_i;
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_FFTInit(&scaler->fft_mp[mp_i], fft_size, STP_FALSE, STP_FALSE);
        STP_Assert(ret == STP_OK);
    }
#endif
    ret = STP_FFTInit(&scaler->ifft, fft_size, STP_TRUE, STP_TRUE);
    STP_Assert(ret == STP_OK);

    for (i = 0; i < STP_NUM_SCALES; i++) {
        ret = STP_FhogInit(&scaler->fhog[i], scaler->scale_model_size, hog_cell_sz, STP_HOGDIM_18);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogGetDims(scaler->fhog[i], &hog_dims);
        for (j = 0; j < hog_dims; j++) {
            ret = STP_ImageInit(&scaler->hogs[i][j], hog_sz);
            STP_Assert(ret == STP_OK);
        }
    }

    scaler->model_images = (stp_multichannel_image *)STP_Calloc(1, sizeof(stp_multichannel_image));

    ret = STP_ImageInit(&scaler->model_images->images[0], scaler->scale_model_size);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageInit(&scaler->model_images->images[1], scaler->scale_model_size);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageInit(&scaler->model_images->images[2], scaler->scale_model_size);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        scaler->model_images_mp[mp_i] = (stp_multichannel_image *)STP_Calloc(1, sizeof(stp_multichannel_image));
        ret = STP_ImageInit(&scaler->model_images_mp[mp_i]->images[0], scaler->scale_model_size);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageInit(&scaler->model_images_mp[mp_i]->images[1], scaler->scale_model_size);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageInit(&scaler->model_images_mp[mp_i]->images[2], scaler->scale_model_size);
        STP_Assert(ret == STP_OK);
    }
#endif

    scaler->hann_win = &hann_win_32_1_imag;

    for (i = 0; i < STP_NUM_SCALES; i++) {
        stp_int32_q scalar;
        rect.pos = tgt_rect.pos;
        rect.size.size.h = (stp_int64)tgt_rect.size.size.h * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.size.w = (stp_int64)tgt_rect.size.size.w * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.q = 15;
        ret = STP_ImageResize(scaler->model_images->images[0], image->images[0], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[1], image->images[1], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[2], image->images[2], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogCompute(scaler->fhog[i], scaler->model_images, scaler->hogs[i]);
        STP_Assert(ret == STP_OK);

        scalar.q = scaler->hann_win->q;
        scalar.value = ((stp_int32*)scaler->hann_win->p_mem)[i];
        for (j = 0; j < hog_dims; j++) {
            STP_ImageDotMulScalar(scaler->hogs[i][j], scaler->hogs[i][j], scalar);
        }
    }

    ret = STP_FhogGetDims(scaler->fhog[0], &hog_dims);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageInit(&scaler->fft_temp_in, fft_size);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ImageInit(&scaler->fft_temp_in_mp[mp_i], fft_size);
        STP_Assert(ret == STP_OK);
    }
#endif

    for (i = 0; i < hog_dims; i++) {
        scaler->fft_temp_in->q = scaler->hogs[0][i]->q;
        for (j = 0; j < hog_sz.h*hog_sz.w; j++) {
            stp_int32 *p = (stp_int32*)scaler->fft_temp_in->p_mem;
            for (k = 0; k < STP_NUM_SCALES; k++) {
                *p++ = *((stp_int32 *)scaler->hogs[k][i]->p_mem + j);
            }
            ret = STP_ComplexImageInit(&scaler->xsf[cnt], fft_size);
            STP_Assert(ret == STP_OK);
            ret = STP_FFTCompute(scaler->fft, scaler->fft_temp_in, scaler->xsf[cnt++]);
            STP_Assert(ret == STP_OK);
        }
    }

    ret = STP_ImageInit(&scaler->sf_den, fft_size);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageInit(&scaler->float_sf_den, fft_size);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageSetDataType(scaler->float_sf_den, STP_FLOAT32);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageInit(&scaler->new_sf_den, fft_size);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ImageInit(&scaler->new_sf_den_mp[mp_i], fft_size);
        STP_Assert(ret == STP_OK);
    }
#endif
    ret = STP_ImageInit(&scaler->temp_sum_den, fft_size);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ImageInit(&scaler->temp_sum_den_mp[mp_i], fft_size);
        STP_Assert(ret == STP_OK);
    }
#endif
    ret = STP_ComplexImageInit(&scaler->temp_sum_num, fft_size);
    STP_Assert(ret == STP_OK);
#ifdef _OPENMP
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ComplexImageInit(&scaler->temp_sum_num_mp[mp_i], fft_size);
        STP_Assert(ret == STP_OK);
    }
#endif
    ret = STP_ComplexImageInit(&scaler->ysf, fft_size);
    STP_Assert(ret == STP_OK);

    gauss_resp = gauss_resp_32_1_imag;
    ret = STP_FFTCompute(scaler->fft, &gauss_resp, scaler->ysf);
    STP_Assert(ret == STP_OK);

    for (i = 0; i < cnt; i++) {
        ret = STP_ComplexImageInit(&scaler->sf_num[i], fft_size);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageInit(&scaler->float_sf_num[i], fft_size);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageSetDataType(scaler->float_sf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageInit(&scaler->new_sf_num[i], fft_size);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConj(scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotMulC(scaler->sf_num[i], scaler->xsf[i], scaler->ysf);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageConvertDataType(scaler->float_sf_num[i], scaler->sf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotEnergy(scaler->temp_sum_den, scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDotAdd(scaler->sf_den, scaler->sf_den, scaler->temp_sum_den);
        STP_Assert(ret == STP_OK);
    }

    ret = STP_ImageConvertDataType(scaler->float_sf_den, scaler->sf_den, STP_FLOAT32);
    STP_Assert(ret == STP_OK);

    scaler->learning_ratio.q = 24;
    scaler->learning_ratio.value = STP_FXP32(1.0f, 24);
    scaler->learning_rate.value = learning_rate_q30;
    scaler->learning_rate.q = 30;
    scaler->forgotting_rate.value = (1L << 30) - learning_rate_q30;
    scaler->forgotting_rate.q = 30;
    scaler->lambda.q = 15;
    scaler->lambda.value = 1;

    *p_scaler = scaler;

    return ret;
}

stp_ret STP_ScalerRun(stp_scaler *scaler, stp_multichannel_image *image, stp_rect_q tgt_rect) {
    stp_int32 i, j, k, cnt = 0, hog_dims, hog_sz;
    stp_rect_q rect;
    stp_int32_q scalar;
    stp_pos pos;
    stp_ret ret = STP_OK;

    STP_Assert(scaler && image);

    ret = STP_FhogGetDims(scaler->fhog[0], &hog_dims);
    STP_Assert(ret == STP_OK);

#ifdef _OPENMP
    int tid;
#pragma omp parallel for private(tid, j, rect, scalar) num_threads(STP_MAX_MP)
    for (i = 0; i < STP_NUM_SCALES; i++) {
        tid = omp_get_thread_num();
        rect.pos = tgt_rect.pos;
        rect.size.size.h = (stp_int64)tgt_rect.size.size.h * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.size.w = (stp_int64)tgt_rect.size.size.w * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.q = 15;
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[0], image->images[0], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[1], image->images[1], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[2], image->images[2], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogCompute(scaler->fhog[i], scaler->model_images_mp[tid], scaler->hogs[i]);
        STP_Assert(ret == STP_OK);

        scalar.q = scaler->hann_win->q;
        scalar.value = ((stp_int32*)scaler->hann_win->p_mem)[i];
        for (j = 0; j < hog_dims; j++) {
            STP_ImageDotMulScalar(scaler->hogs[i][j], scaler->hogs[i][j], scalar);
        }
    }

#else
    for (i = 0; i < STP_NUM_SCALES; i++) {
        rect.pos = tgt_rect.pos;
        rect.size.size.h = (stp_int64)tgt_rect.size.size.h * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.size.w = (stp_int64)tgt_rect.size.size.w * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.q = 15;
        ret = STP_ImageResize(scaler->model_images->images[0], image->images[0], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[1], image->images[1], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[2], image->images[2], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogCompute(scaler->fhog[i], scaler->model_images, scaler->hogs[i]);
        STP_Assert(ret == STP_OK);

        scalar.q = scaler->hann_win->q;
        scalar.value = ((stp_int32*)scaler->hann_win->p_mem)[i];
        for (j = 0; j < hog_dims; j++) {
            STP_ImageDotMulScalar(scaler->hogs[i][j], scaler->hogs[i][j], scalar);
        }
    }
#endif

    hog_sz = scaler->hogs[0][0]->height*scaler->hogs[0][0]->width;

#ifdef _OPENMP
#pragma omp parallel for private(tid, j, k) reduction(+:cnt) num_threads(STP_MAX_MP)
    for (i = 0; i < hog_dims; i++) {
        tid = omp_get_thread_num();
        scaler->fft_temp_in_mp[tid]->q = scaler->hogs[0][i]->q;
        for (j = 0; j < hog_sz; j++) {
            stp_int32 *p = (stp_int32*)scaler->fft_temp_in_mp[tid]->p_mem;
            for (k = 0; k < STP_NUM_SCALES; k++) {
                *p++ = *((stp_int32 *)scaler->hogs[k][i]->p_mem + j);
            }
            ret = STP_FFTCompute(scaler->fft_mp[tid], scaler->fft_temp_in_mp[tid], scaler->xsf[i*hog_sz + j]);
            STP_Assert(ret == STP_OK);
            cnt++;
        }
    }
#else
    for (i = 0; i < hog_dims; i++) {
        scaler->fft_temp_in->q = scaler->hogs[0][i]->q;
        for (j = 0; j < hog_sz; j++) {
            stp_int32 *p = (stp_int32*)scaler->fft_temp_in->p_mem;
            for (k = 0; k < STP_NUM_SCALES; k++) {
                *p++ = *((stp_int32 *)scaler->hogs[k][i]->p_mem + j);
            }
            ret = STP_FFTCompute(scaler->fft, scaler->fft_temp_in, scaler->xsf[cnt++]);
            STP_Assert(ret == STP_OK);
        }
    }
#endif

#ifdef _OPENMP
    int mp_i;
    ret = STP_ComplexImageClear(scaler->temp_sum_num);
    STP_Assert(ret == STP_OK);
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ComplexImageClear(scaler->temp_sum_num_mp[mp_i]);
        STP_Assert(ret == STP_OK);
    }
#pragma omp parallel for private(tid) num_threads(STP_MAX_MP)
    for (i = 0; i < cnt; i++) {
        tid = omp_get_thread_num();
        ret = STP_ComplexImageConvertDataType(scaler->sf_num[i], scaler->float_sf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
        scaler->xsf[i]->real->q += 2;
        scaler->xsf[i]->imag->q += 2;
        ret = STP_ComplexImageDotMulC(scaler->new_sf_num[i], scaler->xsf[i], scaler->sf_num[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotAddC(scaler->temp_sum_num_mp[tid], scaler->temp_sum_num_mp[tid], scaler->new_sf_num[i]);
        STP_Assert(ret == STP_OK);
    }
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ComplexImageDotAddC(scaler->temp_sum_num, scaler->temp_sum_num, scaler->temp_sum_num_mp[mp_i]);
        STP_Assert(ret == STP_OK);
    }
#else
    ret = STP_ComplexImageClear(scaler->temp_sum_num);
    STP_Assert(ret == STP_OK);

    for (i = 0; i < cnt; i++) {
        ret = STP_ComplexImageConvertDataType(scaler->sf_num[i], scaler->float_sf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);
        scaler->xsf[i]->real->q += 2;
        scaler->xsf[i]->imag->q += 2;
        ret = STP_ComplexImageDotMulC(scaler->new_sf_num[i], scaler->xsf[i], scaler->sf_num[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotAddC(scaler->temp_sum_num, scaler->temp_sum_num, scaler->new_sf_num[i]);
        STP_Assert(ret == STP_OK);
    }
#endif

    ret = STP_ImageConvertDataType(scaler->sf_den, scaler->float_sf_den, STP_INT32);
    STP_Assert(ret == STP_OK);

    /* to make STP_ComplexImageDotDiv more accurate */
    STP_ImageDotShift(scaler->temp_sum_den, scaler->sf_den, 6);
    scaler->temp_sum_num->real->q += 4;
    scaler->temp_sum_num->imag->q += 4;
    ret = STP_ComplexImageDotDiv(scaler->temp_sum_num, scaler->temp_sum_num, scaler->temp_sum_den, scaler->lambda);
    STP_Assert(ret == STP_OK);

    ret = STP_FFTCompute(scaler->ifft, scaler->temp_sum_num, scaler->xsf[0]);
    STP_Assert(ret == STP_OK);

    ret = STP_ImageMaxPoint(scaler->xsf[0]->real, &pos);
    STP_Assert(ret == STP_OK);

    scaler->cur_scale_factor = STP_INT_Q_MUL(scaler->cur_scale_factor, scaler->scale_factors[pos.x]);
    scaler->cur_scale_factor = STP_INT_Q_MAX(scaler->min_scale_factor, scaler->cur_scale_factor);
    scaler->cur_scale_factor = STP_INT_Q_MIN(scaler->max_scale_factor, scaler->cur_scale_factor);

    scaler->cur_target_size.size.h = (stp_int64)scaler->cur_scale_factor.value*scaler->base_target_size.size.h
            >> (scaler->cur_scale_factor.q + scaler->base_target_size.q - 15);
    scaler->cur_target_size.size.w = (stp_int64)scaler->cur_scale_factor.value*scaler->base_target_size.size.w
            >> (scaler->cur_scale_factor.q + scaler->base_target_size.q - 15);
    scaler->cur_target_size.q = 15;

    return STP_OK;
}

stp_ret STP_ScalerGetTargetSize(stp_scaler *scaler, stp_size_q *size) {
    STP_Assert(scaler && size);
    *size = scaler->cur_target_size;
    return STP_OK;
}

stp_ret STP_ScalerSetLearningRatio(stp_scaler *scaler, stp_int32 learning_ratio_q24) {
    STP_Assert(scaler);
    scaler->learning_ratio.q = 24;
    scaler->learning_ratio.value = learning_ratio_q24;
    return STP_OK;
}

stp_ret STP_ScalerUpdate(stp_scaler *scaler, stp_multichannel_image *image, stp_rect_q tgt_rect) {
    stp_int32 i, j, k, cnt = 0, hog_dims, hog_sz;
    stp_rect_q rect;
    stp_int32_q scalar, learning_rate;
    float coef[2];
    stp_ret ret = STP_OK;

    STP_Assert(scaler && image);

    learning_rate = STP_INT_Q_MUL(scaler->learning_rate, scaler->learning_ratio);
    coef[1] = (float)learning_rate.value/(1L << learning_rate.q);
    coef[0] = 1.0f - coef[1];

    ret = STP_FhogGetDims(scaler->fhog[0], &hog_dims);
    STP_Assert(ret == STP_OK);

#ifdef _OPENMP
    int tid;
#pragma omp parallel for private(tid, j, rect, scalar) num_threads(STP_MAX_MP)
    for (i = 0; i < STP_NUM_SCALES; i++) {
        tid = omp_get_thread_num();
        rect.pos = tgt_rect.pos;
        rect.size.size.h = (stp_int64)tgt_rect.size.size.h * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.size.w = (stp_int64)tgt_rect.size.size.w * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.q = 15;
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[0], image->images[0], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[1], image->images[1], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images_mp[tid]->images[2], image->images[2], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogCompute(scaler->fhog[i], scaler->model_images_mp[tid], scaler->hogs[i]);
        STP_Assert(ret == STP_OK);

        scalar.q = scaler->hann_win->q;
        scalar.value = ((stp_int32*)scaler->hann_win->p_mem)[i];
        for (j = 0; j < hog_dims; j++) {
            STP_ImageDotMulScalar(scaler->hogs[i][j], scaler->hogs[i][j], scalar);
        }
    }
#else
    for (i = 0; i < STP_NUM_SCALES; i++) {
        rect.pos = tgt_rect.pos;
        rect.size.size.h = (stp_int64)tgt_rect.size.size.h * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.size.w = (stp_int64)tgt_rect.size.size.w * scaler->scale_factors[i].value
                >> (tgt_rect.size.q + scaler->scale_factors[i].q - 15);
        rect.size.q = 15;
        ret = STP_ImageResize(scaler->model_images->images[0], image->images[0], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[1], image->images[1], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(scaler->model_images->images[2], image->images[2], &rect);
        STP_Assert(ret == STP_OK);
        ret = STP_FhogCompute(scaler->fhog[i], scaler->model_images, scaler->hogs[i]);
        STP_Assert(ret == STP_OK);

        scalar.q = scaler->hann_win->q;
        scalar.value = ((stp_int32*)scaler->hann_win->p_mem)[i];
        for (j = 0; j < hog_dims; j++) {
            STP_ImageDotMulScalar(scaler->hogs[i][j], scaler->hogs[i][j], scalar);
        }
    }
#endif

    hog_sz = scaler->hogs[0][0]->height*scaler->hogs[0][0]->width;

#ifdef _OPENMP
#pragma omp parallel for private(tid, j, k) reduction(+:cnt) num_threads(STP_MAX_MP)
    for (i = 0; i < hog_dims; i++) {
        tid = omp_get_thread_num();
        scaler->fft_temp_in_mp[tid]->q = scaler->hogs[0][i]->q;
        for (j = 0; j < hog_sz; j++) {
            stp_int32 *p = (stp_int32*)scaler->fft_temp_in_mp[tid]->p_mem;
            for (k = 0; k < STP_NUM_SCALES; k++) {
                *p++ = *((stp_int32 *)scaler->hogs[k][i]->p_mem + j);
            }
            ret = STP_FFTCompute(scaler->fft_mp[tid], scaler->fft_temp_in_mp[tid], scaler->xsf[i*hog_sz + j]);
            STP_Assert(ret == STP_OK);
            cnt++;
        }
    }
#else
    for (i = 0; i < hog_dims; i++) {
        scaler->fft_temp_in->q = scaler->hogs[0][i]->q;
        for (j = 0; j < hog_sz; j++) {
            stp_int32 *p = (stp_int32*)scaler->fft_temp_in->p_mem;
            for (k = 0; k < STP_NUM_SCALES; k++) {
                *p++ = *((stp_int32 *)scaler->hogs[k][i]->p_mem + j);
            }
            ret = STP_FFTCompute(scaler->fft, scaler->fft_temp_in, scaler->xsf[cnt++]);
            STP_Assert(ret == STP_OK);
        }
    }
#endif

#ifdef _OPENMP
    int mp_i;
    ret = STP_ImageClear(scaler->new_sf_den);
    STP_Assert(ret == STP_OK);
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ImageClear(scaler->new_sf_den_mp[mp_i]);
        STP_Assert(ret == STP_OK);
    }
#pragma omp parallel for private(tid) num_threads(STP_MAX_MP)
    for (i = 0; i < cnt; i++) {
        tid = omp_get_thread_num();
        ret = STP_ComplexImageConj(scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotMulC(scaler->new_sf_num[i], scaler->xsf[i], scaler->ysf);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageConvertDataType(scaler->new_sf_num[i], scaler->new_sf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageCoefMix(scaler->float_sf_num[i], scaler->float_sf_num[i], scaler->new_sf_num[i], coef);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageSetDataType(scaler->new_sf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotEnergy(scaler->temp_sum_den_mp[tid], scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDotAdd(scaler->new_sf_den_mp[tid], scaler->new_sf_den_mp[tid], scaler->temp_sum_den_mp[tid]);
        STP_Assert(ret == STP_OK);
    }
    for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
        ret = STP_ImageDotAdd(scaler->new_sf_den, scaler->new_sf_den, scaler->new_sf_den_mp[mp_i]);
        STP_Assert(ret == STP_OK);
    }
#else
    ret = STP_ImageClear(scaler->new_sf_den);
    STP_Assert(ret == STP_OK);

    for (i = 0; i < cnt; i++) {
        ret = STP_ComplexImageConj(scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageDotMulC(scaler->new_sf_num[i], scaler->xsf[i], scaler->ysf);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageConvertDataType(scaler->new_sf_num[i], scaler->new_sf_num[i], STP_FLOAT32);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageCoefMix(scaler->float_sf_num[i], scaler->float_sf_num[i], scaler->new_sf_num[i], coef);
        STP_Assert(ret == STP_OK);
        ret = STP_ComplexImageSetDataType(scaler->new_sf_num[i], STP_INT32);
        STP_Assert(ret == STP_OK);

        ret = STP_ComplexImageDotEnergy(scaler->temp_sum_den, scaler->xsf[i]);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageDotAdd(scaler->new_sf_den, scaler->new_sf_den, scaler->temp_sum_den);
        STP_Assert(ret == STP_OK);
    }
#endif

    ret = STP_ImageConvertDataType(scaler->new_sf_den, scaler->new_sf_den, STP_FLOAT32);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageCoefMix(scaler->float_sf_den, scaler->float_sf_den, scaler->new_sf_den, coef);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageSetDataType(scaler->new_sf_den, STP_INT32);
    STP_Assert(ret == STP_OK);

    return ret;
}

stp_ret STP_ScalerDestroy(stp_scaler *scaler) {
    stp_int32 i, j, hog_dims;
    stp_ret ret;

    if (scaler) {
        for (i = 0; i < STP_MAX_HOG_PIXELS; i++) {
            STP_ComplexImageDestroy(scaler->sf_num[i]);
            STP_ComplexImageDestroy(scaler->new_sf_num[i]);
            STP_ComplexImageDestroy(scaler->float_sf_num[i]);
            STP_ComplexImageDestroy(scaler->xsf[i]);
        }
        STP_ComplexImageDestroy(scaler->temp_sum_num);
#ifdef _OPENMP
        int mp_i;
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ComplexImageDestroy(scaler->temp_sum_num_mp[mp_i]);
        }
#endif

        STP_ImageDestroy(scaler->fft_temp_in);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(scaler->fft_temp_in_mp[mp_i]);
        }
#endif
        STP_ImageDestroy(scaler->sf_den);
        STP_ImageDestroy(scaler->model_images->images[0]);
        STP_ImageDestroy(scaler->model_images->images[1]);
        STP_ImageDestroy(scaler->model_images->images[2]);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(scaler->model_images_mp[mp_i]->images[0]);
            STP_ImageDestroy(scaler->model_images_mp[mp_i]->images[1]);
            STP_ImageDestroy(scaler->model_images_mp[mp_i]->images[2]);
            STP_Free(scaler->model_images_mp[mp_i]);
        }
#endif

        STP_ImageDestroy(scaler->new_sf_den);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(scaler->new_sf_den_mp[mp_i]);
        }
#endif
        STP_ImageDestroy(scaler->float_sf_den);
        STP_ImageDestroy(scaler->temp_sum_den);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_ImageDestroy(scaler->temp_sum_den_mp[mp_i]);
        }
#endif

        ret = STP_FhogGetDims(scaler->fhog[0], &hog_dims);
        STP_Assert(ret == STP_OK);

        for (i = 0; i < STP_NUM_SCALES; i++) {
            for (j = 0; j < hog_dims; j++) {
                STP_ImageDestroy(scaler->hogs[i][j]);
            }
            STP_FhogDestroy(scaler->fhog[i]);
        }

        STP_FFTDestroy(scaler->fft);
#ifdef _OPENMP
        for (mp_i = 0; mp_i < STP_MAX_MP; mp_i++) {
            STP_FFTDestroy(scaler->fft_mp[mp_i]);
        }
#endif
        STP_FFTDestroy(scaler->ifft);

        STP_Free(scaler->model_images);

        STP_Free(scaler);
    }

    return STP_OK;
}
