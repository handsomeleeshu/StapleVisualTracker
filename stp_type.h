/*
 * stp_type.h
 *
 *  Created on: 2018-9-5
 *      Author: handsomelee
 */

#ifndef _STP_TYPE_H_
#define _STP_TYPE_H_

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "assert.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* for easy code porting  */

typedef int8_t stp_int8;
typedef int16_t stp_int16;
typedef int32_t stp_int32;
typedef int64_t stp_int64;
typedef uint8_t stp_uint8;
typedef uint16_t stp_uint16;
typedef uint32_t stp_uint32;
typedef uint64_t stp_uint64;
typedef uint32_t stp_bool;

#define STP_Malloc malloc
#define STP_Calloc calloc
#define STP_Memclr memset
#define STP_Free free
#define STP_Assert assert

#define STP_TRUE ((stp_uint32)(-1LL))
#define STP_FALSE (0)

#define STP_PI                  3.14159265358979323846  /* pi */

#define STP_POWERS_OF_2(x) ((x & (x - 1)) == 0)

#define STP_SIGNED_HALF(val)   ((val)>=0? 0.5:-0.5)

#define STP_MININTVAL(wl)      ((stp_int64)(-1)*((stp_int64)1<<((wl)-1)))      // min value of the integer with word length wl
#define STP_MAXINTVAL(wl)      (~STP_MININTVAL(wl))           // max value of the integer with word length wl

#define STP_CLIP(val, wl)      ((val)> STP_MAXINTVAL(wl)? STP_MAXINTVAL(wl):((val)<STP_MININTVAL(wl)? STP_MININTVAL(wl):val))

#define STP_FLOAT_VAL_OF_FXP_REP(val, wl, iwl)     ((val)*((stp_int64)1<<(iwl)))

#define STP_FXP16(val, iwl)     ((stp_int16)STP_CLIP(STP_FLOAT_VAL_OF_FXP_REP(val, 16, iwl) + STP_SIGNED_HALF(val), 16))
#define STP_FXP32(val, iwl)     ((stp_int32)STP_CLIP(STP_FLOAT_VAL_OF_FXP_REP(val, 32, iwl) + STP_SIGNED_HALF(val), 32))
#define STP_FXP64(val, iwl)     ((stp_int64)STP_CLIP(STP_FLOAT_VAL_OF_FXP_REP(val, 64, iwl) + STP_SIGNED_HALF(val), 64))

#define STP_ABS(x) (((x) > 0) ? (x):(-(x)))
#define STP_MUL(n, m, shift)    (stp_int64)((((stp_int64)(n) * (m)) + (1LL << (shift - 1) ) ) >> shift)
#define STP_RIGHT_SHIFT_RND(n, shift)    ((stp_int32)(((n) + (1LL << ((shift) - 1))) >> (shift)))
#define STP_MAX(a, b) ((a) > (b) ? (a) : (b))
#define STP_MIN(a, b) ((a) < (b) ? (a) : (b))

#define STP_ALIGN_ON2(x) ((x + 1) >> 1 << 1)
#define STP_ALIGN_ON4(x) ((x + 3) >> 2 << 2)
#define STP_ALIGN_ON8(x) ((x + 7) >> 3 << 3)

//#define inline __attribute((always_inline))

typedef enum {
    STP_OK = 0,
    STP_ERR_NOMEM = -1,
    STP_ERR_INVALID_PARAM = -2,
    STP_ERR_NOT_IMPL = -3,
    STP_ERR_UNKNOWN = -100
} stp_ret;

typedef struct {
    stp_int32 value;
    stp_int32 q;
} stp_int32_q;

typedef struct {
    stp_int32 x, y;
} stp_pos;

typedef struct {
    stp_int32 q;
    stp_pos pos;
} stp_pos_q;

typedef struct {
    stp_int32 w, h;
} stp_size;

typedef struct {
    stp_int32 q;
    stp_size size;
} stp_size_q;

typedef struct {
    stp_pos pos;
    stp_size size;
} stp_rect;

typedef struct {
    stp_pos_q pos;
    stp_size_q size;
} stp_rect_q;

static inline stp_int32_q STP_INT_Q_MAX(stp_int32_q a, stp_int32_q b) {
    return (((stp_int64)a.value << b.q) > ((stp_int64)b.value << a.q)) ? a : b;
}

static inline stp_int32_q STP_INT_Q_MIN(stp_int32_q a, stp_int32_q b) {
    return (((stp_int64)a.value << b.q) < ((stp_int64)b.value << a.q)) ? a : b;
}

static inline stp_int32_q STP_INT_Q_ADD(stp_int32_q a, stp_int32_q b) {
    stp_int32_q ret;
    ret.value = (a.value << (b.q - a.q)) + b.value;
    ret.q = a.q;
    return ret;
}

static inline stp_int32_q STP_INT_Q_SUB(stp_int32_q a, stp_int32_q b) {
    stp_int32_q ret;
    ret.value = (a.value << (b.q - a.q)) - b.value;
    ret.q = a.q;
    return ret;
}

static inline stp_int32_q STP_INT_Q_MUL(stp_int32_q a, stp_int32_q b) {
    stp_int32_q ret;
    stp_int64 x = (stp_int64)a.value*b.value + (1LL << (b.q - 1));
    ret.value = x >> b.q;
    ret.q = a.q;
    return ret;
}

static inline stp_int32_q STP_INT_Q_DIV(stp_int32_q a, stp_int32_q b) {
    stp_int32_q ret;
    stp_int64 x = ((stp_int64)a.value << 32)/b.value;
    ret.value = x >> (32 - b.q);
    ret.q = a.q;
    return ret;
}

static inline stp_int32 STP_INT_Q_FLOOR(stp_int32_q a) {
    return a.value >> a.q;
}

static inline stp_int32 STP_INT_Q_ROUND(stp_int32_q a) {
    return STP_RIGHT_SHIFT_RND(a.value, a.q);
}

#ifdef __cplusplus
}
#endif

#endif /* _STP_TYPE_H_ */
