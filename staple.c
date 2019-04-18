/*
 * staple.c
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#include "staple.h"
#include "math.h"

stp_ret Staple_Init(Staple **p_staple, stp_multichannel_image *in_image, stp_rect target_rect) {
    stp_ret ret = STP_OK;
    stp_int32 h, w;
    stp_size size;
    stp_rect temp_rect;
    stp_rect_q temp_rect_q;

    Staple *staple = (Staple *)STP_Calloc(1, sizeof(Staple));

    staple->profile = STP_LOW_PROFILE;

    staple->image_size.w = in_image->images[0]->width;
    staple->image_size.h = in_image->images[0]->height;

    /* restrict target rect to even pixels */
    target_rect.size.h &= 0xfffffffe;
    target_rect.size.w &= 0xfffffffe;

    h = target_rect.size.h;
    w = target_rect.size.w;

    if ((float)h > (float)w*1.4) {
        size.h = 256;
        size.w = 128;
        temp_rect.size.h = (stp_int32)(sqrt(target_rect.size.h*target_rect.size.w*8) + 3) & 0xfffffffc;
        temp_rect.size.w = temp_rect.size.h >> 1;
        while (temp_rect.size.w > target_rect.size.w*2.2f) {
            temp_rect.size.w -= 2;
        }
        while (temp_rect.size.w < target_rect.size.w*2.0f) {
            temp_rect.size.w += 2;
        }
        temp_rect.size.h = temp_rect.size.w << 1;
        target_rect.size.h = STP_MIN(target_rect.size.h, temp_rect.size.h - target_rect.size.w);
    } else if ((float)w > (float)h*1.4) {
        size.w = 256;
        size.h = 128;
        temp_rect.size.w = (stp_int32)(sqrt(target_rect.size.h*target_rect.size.w*8) + 3) & 0xfffffffc;
        temp_rect.size.h = temp_rect.size.w >> 1;
        while (temp_rect.size.h > target_rect.size.h*2.2f) {
            temp_rect.size.h -= 2;
        }
        while (temp_rect.size.h < target_rect.size.h*2.0f) {
            temp_rect.size.h += 2;
        }
        temp_rect.size.w = temp_rect.size.h << 1;
        target_rect.size.w = STP_MIN(target_rect.size.w, temp_rect.size.w - target_rect.size.h);
    } else {
        if (staple->profile == STP_HIGH_PROFILE) {
            size.w = 256;
            size.h = 256;
        } else {
            size.w = 128;
            size.h = 128;
        }
        temp_rect.size.w = (stp_int32)(sqrt(target_rect.size.h*target_rect.size.w*4) + 3) & 0xfffffffc;
        temp_rect.size.h = temp_rect.size.w;
    }

    staple->tracker_work_size = size;
    staple->tracker_work_image = (stp_multichannel_image *)STP_Calloc(1, sizeof(stp_multichannel_image));
    STP_ImageInit(&staple->tracker_work_image->images[0], size);
    STP_ImageInit(&staple->tracker_work_image->images[1], size);
    STP_ImageInit(&staple->tracker_work_image->images[2], size);

    staple->bg_size = temp_rect.size;
    temp_rect.pos = target_rect.pos;
    temp_rect_q.pos.pos = temp_rect.pos;
    temp_rect_q.pos.q = 0;
    temp_rect_q.size.size = temp_rect.size;
    temp_rect_q.size.q = 0;
    ret = STP_ImageResize(staple->tracker_work_image->images[0], in_image->images[0], &temp_rect_q);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageResize(staple->tracker_work_image->images[1], in_image->images[1], &temp_rect_q);
    STP_Assert(ret == STP_OK);
    ret = STP_ImageResize(staple->tracker_work_image->images[2], in_image->images[2], &temp_rect_q);
    STP_Assert(ret == STP_OK);

    staple->target_pos = target_rect.pos;
    staple->tracker_work_rect.pos.x = size.w >> 1;
    staple->tracker_work_rect.pos.y = size.h >> 1;
    staple->tracker_work_rect.size.h = target_rect.size.h*size.h/temp_rect.size.h;
    staple->tracker_work_rect.size.w = target_rect.size.w*size.w/temp_rect.size.w;
    ret = STP_TrackerInit(&staple->tracker, staple->tracker_work_image,
            staple->tracker_work_rect, STP_FXP32(0.01f, 30), STP_FXP32(0.02f, 30));
    STP_Assert(ret == STP_OK);

    staple->target_size = target_rect.size;
    staple->target_size_ratio = (float)staple->target_size.w/staple->target_size.h;
    staple->target_size_q.size = staple->target_size;
    staple->target_size_q.q = 0;
    temp_rect_q.pos.pos = temp_rect.pos;
    temp_rect_q.pos.q = 0;
    temp_rect_q.size.size = target_rect.size;
    temp_rect_q.size.q = 0;
    ret = STP_ScalerInit(&staple->scaler, in_image, temp_rect_q, STP_FXP32(0.01f, 30));
    STP_Assert(ret == STP_OK);

    staple->state = STP_RUNING;
    staple->long_term_resp = 0.25f;
    staple->cur_resp = 0.0f;
    staple->frame_cnt = 0;
    staple->dynamic_learning_ratio = STP_TRUE;

    *p_staple = staple;

    return STP_OK;
}

stp_ret Staple_Run(Staple *staple, stp_multichannel_image *in_image) {
    stp_ret ret = STP_OK;
    stp_size size;
    stp_pos pos;
    stp_int32_q resp_value;
    stp_rect temp_rect;
    stp_rect_q temp_rect_q;
    stp_image *resp = NULL;

    STP_Assert(staple);

    if (staple->state == STP_RUNING) {
        temp_rect.pos = staple->target_pos;
        temp_rect.size = staple->bg_size;
        temp_rect_q.pos.pos = temp_rect.pos;
        temp_rect_q.pos.q = 0;
        temp_rect_q.size.size = temp_rect.size;
        temp_rect_q.size.q = 0;
        ret = STP_ImageResize(staple->tracker_work_image->images[0], in_image->images[0], &temp_rect_q);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(staple->tracker_work_image->images[1], in_image->images[1], &temp_rect_q);
        STP_Assert(ret == STP_OK);
        ret = STP_ImageResize(staple->tracker_work_image->images[2], in_image->images[2], &temp_rect_q);
        STP_Assert(ret == STP_OK);

        ret = STP_TrackerRun(staple->tracker, staple->tracker_work_image);
        STP_Assert(ret == STP_OK);
        ret = STP_TrackerGetResponseMap(staple->tracker, &resp);
        STP_Assert(ret == STP_OK);

        ret = STP_ImageMaxPoint(resp, &pos);
        STP_Assert(ret == STP_OK);
        resp_value.q = resp->q;
        resp_value.value = ((stp_int32*)resp->p_mem)[pos.y*resp->width + pos.x];
        pos.x = pos.x - (resp->width >> 1);
        pos.y = pos.y - (resp->height >> 1);

        pos.x = (float)pos.x*staple->bg_size.w/staple->tracker_work_image->images[0]->width + (rand() & 0x1);
        pos.y = (float)pos.y*staple->bg_size.h/staple->tracker_work_image->images[0]->height + (rand() & 0x1);
        staple->target_pos.x += pos.x;
        staple->target_pos.y += pos.y;

        staple->target_pos.x = STP_MIN(staple->target_pos.x, staple->image_size.w);
        staple->target_pos.x = STP_MAX(staple->target_pos.x, 0);
        staple->target_pos.y = STP_MIN(staple->target_pos.y, staple->image_size.h);
        staple->target_pos.y = STP_MAX(staple->target_pos.y, 0);

        float cur_resp = (float)resp_value.value/(1L << resp_value.q);
        float cur_resp_ratio = cur_resp/staple->long_term_resp;
        staple->long_term_resp = staple->long_term_resp*0.96f + cur_resp*0.04f;
        if (staple->frame_cnt > 100 && (cur_resp_ratio < 0.55f)) {
            /* lost target */
            size.h = resp->height*staple->bg_size.h/staple->tracker_work_image->images[0]->height;
            size.w = resp->width*staple->bg_size.w/staple->tracker_work_image->images[0]->width;

            size.h *= 0.9f;
            size.w *= 0.9f;
            staple->redetect_looper = 0;
            if (staple->bg_size.h == staple->bg_size.w) {
                staple->redect_area_total = 9;
                staple->redetect_pos[0] = staple->target_pos;
                staple->redetect_pos[1].x = staple->target_pos.x - size.w;
                staple->redetect_pos[1].y = staple->target_pos.y;
                staple->redetect_pos[2].x = staple->target_pos.x + size.w;
                staple->redetect_pos[2].y = staple->target_pos.y;
                staple->redetect_pos[3].x = staple->target_pos.x;
                staple->redetect_pos[3].y = staple->target_pos.y + size.h;
                staple->redetect_pos[4].x = staple->target_pos.x;
                staple->redetect_pos[4].y = staple->target_pos.y - size.h;
                staple->redetect_pos[5].x = staple->target_pos.x - size.w;
                staple->redetect_pos[5].y = staple->target_pos.y - size.h;
                staple->redetect_pos[6].x = staple->target_pos.x - size.w;
                staple->redetect_pos[6].y = staple->target_pos.y + size.h;
                staple->redetect_pos[7].x = staple->target_pos.x + size.w;
                staple->redetect_pos[7].y = staple->target_pos.y + size.h;
                staple->redetect_pos[8].x = staple->target_pos.x + size.w;
                staple->redetect_pos[8].y = staple->target_pos.y - size.h;
            } else {
                staple->redect_area_total = 11;
                staple->redetect_pos[0] = staple->target_pos;
                staple->redetect_pos[1].x = staple->target_pos.x - size.w;
                staple->redetect_pos[1].y = staple->target_pos.y;
                staple->redetect_pos[2].x = staple->target_pos.x + size.w;
                staple->redetect_pos[2].y = staple->target_pos.y;
                staple->redetect_pos[3].x = staple->target_pos.x;
                staple->redetect_pos[3].y = staple->target_pos.y + size.h;
                staple->redetect_pos[4].x = staple->target_pos.x;
                staple->redetect_pos[4].y = staple->target_pos.y - size.h;
                staple->redetect_pos[5].x = staple->target_pos.x - size.w;
                staple->redetect_pos[5].y = staple->target_pos.y - size.h;
                staple->redetect_pos[6].x = staple->target_pos.x - size.w;
                staple->redetect_pos[6].y = staple->target_pos.y + size.h;
                staple->redetect_pos[7].x = staple->target_pos.x + size.w;
                staple->redetect_pos[7].y = staple->target_pos.y + size.h;
                staple->redetect_pos[8].x = staple->target_pos.x + size.w;
                staple->redetect_pos[8].y = staple->target_pos.y - size.h;
                staple->redetect_pos[9].x = staple->target_pos.x + 2*size.w;
                staple->redetect_pos[9].y = staple->target_pos.y;
                staple->redetect_pos[10].x = staple->target_pos.x - 2*size.w;
                staple->redetect_pos[10].y = staple->target_pos.y;
            }
            staple->state = STP_REDETECTING;
        } else {
            if (staple->dynamic_learning_ratio) {
                float ratio;

                ratio = (float)staple->bg_size.w*staple->bg_size.h/(staple->tracker_work_size.w*staple->tracker_work_size.h);
                ratio = STP_MIN(1.0f, ratio);
                ret = STP_TrackerSetLearningRatio(staple->tracker, STP_FXP32(ratio, 24));
                STP_Assert(ret == STP_OK);
                ratio = STP_MAX(0.1f, ratio);
                ret = STP_ScalerSetLearningRatio(staple->scaler, STP_FXP32(ratio, 24));
                STP_Assert(ret == STP_OK);
            }

            temp_rect.pos = staple->target_pos;
            temp_rect.size = staple->bg_size;
            if ((staple->frame_cnt & 0x1) == 0) {
                temp_rect_q.pos.pos = temp_rect.pos;
                temp_rect_q.pos.q = 0;
                temp_rect_q.size = staple->target_size_q;
                ret = STP_ScalerRun(staple->scaler, in_image, temp_rect_q);
                STP_Assert(ret == STP_OK);
                ret = STP_ScalerGetTargetSize(staple->scaler, &staple->target_size_q);
                STP_Assert(ret == STP_OK);
                temp_rect_q.size = staple->target_size_q;
                ret = STP_ScalerUpdate(staple->scaler, in_image, temp_rect_q);
                STP_Assert(ret == STP_OK);

                staple->target_size.h = staple->target_size_q.size.h >> staple->target_size_q.q;
                staple->target_size.w = staple->target_size.h*staple->target_size_ratio;
                if (staple->bg_size.h > staple->bg_size.w) {
                    temp_rect.size.h = (stp_int32)(sqrt(staple->target_size.h*staple->target_size.w*8) + 3) & 0xfffffffc;
                    temp_rect.size.w = temp_rect.size.h >> 1;
                    while (temp_rect.size.w > staple->target_size.w*2.2f) {
                        temp_rect.size.w -= 2;
                    }
                    while (temp_rect.size.w < staple->target_size.w*2.0f) {
                        temp_rect.size.w += 2;
                    }
                    temp_rect.size.h = temp_rect.size.w << 1;
                } else if (staple->bg_size.h < staple->bg_size.w) {
                    temp_rect.size.w = (stp_int32)(sqrt(staple->target_size.h*staple->target_size.w*8) + 3) & 0xfffffffc;
                    temp_rect.size.h = temp_rect.size.w >> 1;
                    while (temp_rect.size.h > staple->target_size.h*2.2f) {
                        temp_rect.size.h -= 2;
                    }
                    while (temp_rect.size.h < staple->target_size.h*2.0f) {
                        temp_rect.size.h += 2;
                    }
                    temp_rect.size.w = temp_rect.size.h << 1;
                } else {
                    temp_rect.size.w = (stp_int32)(sqrt(staple->target_size.h*staple->target_size.w*4) + 3) & 0xfffffffc;
                    temp_rect.size.h = temp_rect.size.w;
                }
            }

            staple->bg_size = temp_rect.size;

            temp_rect_q.pos.pos = temp_rect.pos;
            temp_rect_q.pos.q = 0;
            temp_rect_q.size.size = temp_rect.size;
            temp_rect_q.size.q = 0;
            ret = STP_ImageResize(staple->tracker_work_image->images[0], in_image->images[0], &temp_rect_q);
            STP_Assert(ret == STP_OK);
            ret = STP_ImageResize(staple->tracker_work_image->images[1], in_image->images[1], &temp_rect_q);
            STP_Assert(ret == STP_OK);
            ret = STP_ImageResize(staple->tracker_work_image->images[2], in_image->images[2], &temp_rect_q);
            STP_Assert(ret == STP_OK);

            ret = STP_TrackerUpdate(staple->tracker, staple->tracker_work_image);
            STP_Assert(ret == STP_OK);
        }
    } else if (staple->state == STP_REDETECTING) {
        stp_int32 i, j;

        for (i = 0; i < 3; i++) {
            j = staple->redect_area_total;
            while (j--) {
                if (staple->redetect_pos[staple->redetect_looper].x > 0 &&
                        staple->redetect_pos[staple->redetect_looper].x < staple->image_size.w &&
                        staple->redetect_pos[staple->redetect_looper].y > 0 &&
                        staple->redetect_pos[staple->redetect_looper].y < staple->image_size.h) {
                    break;
                }
                staple->redetect_looper++;
                if (staple->redetect_looper >= staple->redect_area_total) {
                    staple->redetect_looper = 0;
                }
            }

            pos = staple->redetect_pos[staple->redetect_looper];

            if (pos.x >= 0 && pos.x < staple->image_size.w &&
                    pos.y >= 0 && pos.y < staple->image_size.h) {
                temp_rect.pos = pos;
                temp_rect.size = staple->bg_size;
                temp_rect_q.pos.pos = temp_rect.pos;
                temp_rect_q.pos.q = 0;
                temp_rect_q.size.size = temp_rect.size;
                temp_rect_q.size.q = 0;
                ret = STP_ImageResize(staple->tracker_work_image->images[0], in_image->images[0], &temp_rect_q);
                STP_Assert(ret == STP_OK);
                ret = STP_ImageResize(staple->tracker_work_image->images[1], in_image->images[1], &temp_rect_q);
                STP_Assert(ret == STP_OK);
                ret = STP_ImageResize(staple->tracker_work_image->images[2], in_image->images[2], &temp_rect_q);
                STP_Assert(ret == STP_OK);

                ret = STP_TrackerRun(staple->tracker, staple->tracker_work_image);
                STP_Assert(ret == STP_OK);
                ret = STP_TrackerGetResponseMap(staple->tracker, &resp);
                STP_Assert(ret == STP_OK);
                ret = STP_ImageMaxPoint(resp, &pos);
                STP_Assert(ret == STP_OK);
                resp_value.q = resp->q;
                resp_value.value = ((stp_int32*)resp->p_mem)[pos.y*resp->width + pos.x];

                pos.x = (float)pos.x*staple->bg_size.w/staple->tracker_work_image->images[0]->width;
                pos.y = (float)pos.y*staple->bg_size.h/staple->tracker_work_image->images[0]->height;

                float cur_resp = (float)resp_value.value/(1L << resp_value.q);
                if (cur_resp > staple->long_term_resp*0.65f) {
                    /* object redected */
                    staple->target_pos.x = staple->redetect_pos[staple->redetect_looper].x + pos.x;
                    staple->target_pos.y = staple->redetect_pos[staple->redetect_looper].y + pos.y;

                    staple->target_pos.x = STP_MIN(staple->target_pos.x, staple->image_size.w);
                    staple->target_pos.x = STP_MAX(staple->target_pos.x, 0);
                    staple->target_pos.y = STP_MIN(staple->target_pos.y, staple->image_size.h);
                    staple->target_pos.y = STP_MAX(staple->target_pos.y, 0);
                    staple->state = STP_RUNING;
                    break;
                }
            }
            staple->redetect_looper++;
            if (staple->redetect_looper >= staple->redect_area_total) {
                staple->redetect_looper = 0;
            }
        }
    }

    staple->frame_cnt++;

    return STP_OK;
}

Staple_State Staple_GetTargetRect(Staple *staple, stp_rect *rect) {
    STP_Assert(staple && rect);

    rect->pos = staple->target_pos;
    rect->size = staple->target_size;

    return staple->state;
}

stp_ret Staple_Destroy(Staple *staple) {
    if (staple) {
        STP_ImageDestroy(staple->tracker_work_image->images[0]);
        STP_ImageDestroy(staple->tracker_work_image->images[1]);
        STP_ImageDestroy(staple->tracker_work_image->images[2]);
        STP_Free(staple->tracker_work_image);

        STP_TrackerDestroy(staple->tracker);
        STP_ScalerDestroy(staple->scaler);
    }
    return STP_OK;
}
