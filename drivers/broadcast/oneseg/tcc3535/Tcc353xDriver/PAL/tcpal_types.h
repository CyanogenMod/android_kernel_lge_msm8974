/****************************************************************************
 *   FileName    : tcpal_types.h
 *   Description : OS glue Function
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

#ifndef __TCPAL_TYPES_H__
#define __TCPAL_TYPES_H__

#ifdef __cplusplus
extern    "C"
{
#endif

#ifdef __cplusplus
#ifdef NULL
#undef NULL
#endif
#define NULL 0
#define TCBB_FUNC extern "C"
#else
#ifdef NULL
#undef NULL
#endif
#define NULL (void*)0
#define TCBB_FUNC
#endif

typedef unsigned char I08U;	/*        */
typedef signed char I08S;	/*        */
typedef unsigned short I16U;	/*         */
typedef signed short I16S;	/*         */
typedef unsigned long I32U;	/*         */
typedef signed long I32S;	/*         */
typedef signed long long I64S;	/*         */
typedef unsigned long long I64U;	/*         */

typedef I64U TcpalTime_t;
typedef I32U TcpalSemaphore_t;
typedef I32U TcpalHandle_t;

#ifdef __cplusplus
	};
#endif

#endif
