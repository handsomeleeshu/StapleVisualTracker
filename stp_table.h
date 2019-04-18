/*
 * stp_table.h
 *
 *  Created on: 2018-9-22
 *      Author: handsomelee
 */

#ifndef _STP_TABLE_H_
#define _STP_TABLE_H_

#include "stp_type.h"
#include "stp_image.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const stp_image hann_win_32_1_imag;
extern const stp_image hann_win_32_32_imag;
extern const stp_image hann_win_64_32_imag;
extern const stp_image hann_win_32_64_imag;
extern const stp_image hann_win_64_64_imag;

extern const stp_image gauss_resp_32_1_imag;
extern const stp_image gauss_resp_32_32_imag;
extern const stp_image gauss_resp_64_32_imag;
extern const stp_image gauss_resp_32_64_imag;
extern const stp_image gauss_resp_64_64_imag;

#ifdef __cplusplus
}
#endif

#endif /* _STP_TABLE_H_ */
