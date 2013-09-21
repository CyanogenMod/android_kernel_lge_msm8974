/****************************************************************************
 *   FileName    : tcc353x_main.c
 *   Description : sample source main
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited to re-
distribution in source or binary form is strictly prohibited.
This source code is provided "AS IS" and nothing contained in this source code shall 
constitute any express or implied warranty of any kind, including without limitation, any warranty 
of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright 
or other third party intellectual property right. No warranty is made, express or implied, 
regarding the information's accuracy, completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, out of 
or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************/

#include "tcc353x_common.h"
#include "tcc353x_api.h"
#include "tcc353x_monitoring.h"
#include "tcc353x_user_defines.h"

#if defined (_MODEL_F9J_)
#define __USER_GPIO9_STRENGTH_MAX__
#endif

/*                               */
Tcc353xRegisterConfig_t Tcc353xSingle[3] = {
	{
	 /*              */
	 /*    */0x06,			/*                                              */
	 /*            */
	 0x10,
	 /*                                           */
	 TCC353X_REMAP_TYPE, TCC353X_INIT_PC_H, TCC353X_INIT_PC_L,
	 /*                                                            */
	 0x00, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                            */
#if defined (__USER_GPIO9_STRENGTH_MAX__)
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x02, 0x00,
#else
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x00, 0x00,
#endif
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                               */
	 0x00, 0x00, 0x00,
	 /*            */
	 0x10,
	 /*                                              */
	 0x00, TCC353X_STREAM_THRESHOLD_SPISLV_WH,
	 /*                                               */
	 TCC353X_STREAM_THRESHOLD_SPISLV_WL, 0x10,
	 /*                                  */
	 0, 0,
	 /*                                  */
	 0, 0,
	 /*                                     */
	 0x11, 0x0F,
	 /*                                     */
	 TCC353X_STREAM_THRESHOLD_SPISLV_WH,
	 TCC353X_STREAM_THRESHOLD_SPISLV_WL,
	 /*                                     */
	 ((TCC353X_BUFF_A_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_A_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_END >> 2) & 0xFF),
	 }
	,
	{
	 /*              */
	 0x00,			/*                   */
	 /*            */
	 0x00,
	 /*                                           */
	 TCC353X_REMAP_TYPE, TCC353X_INIT_PC_H, TCC353X_INIT_PC_L,
	 /*                                                            */
	 0xF0, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                            */
#if defined (__USER_GPIO9_STRENGTH_MAX__)
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x02, 0x00,
#else
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x00, 0x00,
#endif
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                               */
	 0x00, 0x00, 0x00,
	 /*            */
	 0x10,
	 /*                                              */
	 0x0F, TCC353X_STREAM_THRESHOLD_WH,
	 /*                                               */
	 TCC353X_STREAM_THRESHOLD_WL, 0x90,
	 /*                                  */
	 0x21, 0x10 | TCC353X_DLR,
	 /*                                  */
	 STS_POLARITY | 0x12, 0x40,
	 /*                                     */
	 0x11, 0x0F,
	 /*                                     */
	 TCC353X_STREAM_THRESHOLD_WH, TCC353X_STREAM_THRESHOLD_WL,
	 /*                                     */
	 ((TCC353X_BUFF_A_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_A_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_END >> 2) & 0xFF),
	 }
	,
	{
	 /*              */
	 0x00,			/*                   */
	 /*            */
	 0x00,
	 /*                                           */
	 TCC353X_REMAP_TYPE, TCC353X_INIT_PC_H, TCC353X_INIT_PC_L,
	 /*                                                            */
	 0xF0, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                            */
#if defined (__USER_GPIO9_STRENGTH_MAX__)
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x02, 0x00,
#else
	 TCC353X_DRV_STR_GPIO_0x13_07_00, 0x00, 0x00,
#endif
	 /*                                                         */
	 0x00, 0x00, 0x00,
	 /*                                                               */
	 0x00, 0x00, 0x00,
	 /*            */
	 0x10,
	 /*                                              */
	 0x0F, TCC353X_STREAM_THRESHOLD_WH,
	 /*                                               */
	 TCC353X_STREAM_THRESHOLD_WL, 0x90,
	 /*                                  */
	 0x11, TCC353X_DLR << 2,
	 /*                                  */
	 0x10, 0x00,
	 /*                                     */
	 0x11, 0x0F,
	 /*                                     */
	 TCC353X_STREAM_THRESHOLD_WH, TCC353X_STREAM_THRESHOLD_WL,
	 /*                                     */
	 ((TCC353X_BUFF_A_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_A_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_A_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_B_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_B_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_C_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_C_END >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_START >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_START >> 2) & 0xFF),
	 /*                                     */
	 ((TCC353X_BUFF_D_END >> 10) & 0xFF),
	 ((TCC353X_BUFF_D_END >> 2) & 0xFF),
	 }
	,
};

#if defined (_MODEL_F9J_)			/*              */
Tcc353xOption_t Tcc353xOptionSingle = {
	/*                          */
	BB_TCC3530,

	/*                          */
	TCC353X_BOARD_SINGLE,

	/*                          */
	TCC353X_IF_TCCSPI,

	/*                          */
	TCC353X_STREAM_IO_MAINIO,

	/*                          */
	/*                          */
	/*                          */
	/*                          */
	/*                          */
	0xA8,

	/*                          */
	0x00, /*                 */ /*        */

	/*                          */
	19200,

	/*                           */
	TCC353X_DIVERSITY_NONE,

	/*                                      */
	1,

	/*                     */
	/*                              */
	/*                                                            */
	0x00,

	/*                               */
	/*                                */
	0x00, /*                   */
	0x00, /*                    */
	0x00, /*                */
	0x00, /*                     */

	/*                             */
	TCC353X_DUAL_BAND_RF,

	/*                          */
	&Tcc353xSingle[0]
};

#elif defined (_MODEL_GV_) 	/*                        */
Tcc353xOption_t Tcc353xOptionSingle = {
	/*                          */
	BB_TCC3530,

	/*                          */
	TCC353X_BOARD_SINGLE,

	/*                          */
	TCC353X_IF_TCCSPI,

	/*                          */
	TCC353X_STREAM_IO_MAINIO,

	/*                          */
	/*                          */
	/*                          */
	/*                          */
	/*                          */
	0xA8,

	/*                          */
	0x00, /*                 */ /*        */

	/*                          */
	38400,

	/*                           */
	TCC353X_DIVERSITY_NONE,

	/*                                      */
	1,

	/*                     */
	/*                              */
	/*                                                            */
	(1<<GPIO_NUM_RF_SWITCHING_TCC3530),

	/*                               */
	/*                                */
	0x00, /*                   */
	0x00, /*                    */
	(1<<GPIO_NUM_RF_SWITCHING_TCC3530), /*                */
	0x00, /*                     */

	/*                             */
	TCC353X_DUAL_BAND_RF,

	/*                          */
	&Tcc353xSingle[0]
};

#elif defined (_MODEL_TCC3535_)
Tcc353xOption_t Tcc353xOptionSingle = {
	/*                          */
	BB_TCC3535,

	/*                          */
	TCC353X_BOARD_SINGLE,

	/*                          */
	TCC353X_IF_I2C,

	/*                          */
	TCC353X_STREAM_IO_STS,

	/*                          */
	/*                          */
	/*                          */
	/*                          */
	/*                          */
	0xA8,

	/*                          */
	0x00, /*                 */ /*        */

	/*                          */
	38400,

	/*                           */
	TCC353X_DIVERSITY_NONE,

	/*                                      */
	0,

	/*                     */
	/*                              */
	/*                                                            */
	0x00,

	/*                               */
	/*                                */
	0x00, /*                   */
	0x00, /*                    */
	0x00, /*                */
	0x00, /*                     */

	/*                             */
	TCC353X_TRIPLE_BAND_RF,

	/*                          */
	&Tcc353xSingle[1]
};
#endif

