/****************************************************************************
 *   FileName    : tcc353x_common.h
 *   Description : common values
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

#ifndef __TCC353X_COMMON_H__
#define __TCC353X_COMMON_H__

#ifdef __cplusplus
extern    "C"
{
#endif

#include "tcpal_types.h"

/*                                                                    */

/*                                      */
#define TCC353X_MAX                 2	/*              */
#define TCC353X_DIVERSITY_MAX       4	/*                       */

#define GPIO_NUM_RF_SWITCHING_TCC3530		9

/*                                  */
#define GPIO_NUM_TRIPLE_BAND_RF_SWITCHING1		8
#define GPIO_NUM_TRIPLE_BAND_RF_SWITCHING2		9
#define GPIO_NUM_TRIPLE_BAND_RF_SWITCHING3		10
typedef enum {
	BB_TCC3530 = 0, 	/*                         */
	BB_TCC3531,		/*                                */
	BB_TCC3532,		/*                         */
	BB_TCC3535		/*                       */
} EnumTcc353xBaseBandName;

typedef enum {
	TCC353X_BOARD_SINGLE = 0,
	TCC353X_BOARD_2DIVERSITY,
	TCC353X_BOARD_3DIVERSITY,
	TCC353X_BOARD_4DIVERSITY,
	TCC353X_BOARD_MAX
} EnumTcc353xBoardType;

typedef enum {
	TCC353X_DIVERSITY_NONE = 0,
	TCC353X_DIVERSITY_MASTER,
	TCC353X_DIVERSITY_MID,
	TCC353X_DIVERSITY_LAST
} EnumTcc353xDiversityPosition;

typedef enum {
	/*                     */
	/*                             */
	/*                     */
	/*                       */
	TCC353X_DUAL_BAND_RF = 0,
	TCC353X_TRIPLE_BAND_RF
} EnumTcc353xRfType;

typedef struct {
	I08U irqMode_0x02;

	I08U irqEn_0x03; 	/*   */

	I08U initRemap_0x0D; 	/*   */
	I08U initPC_0x0E; 	/*   */
	I08U initPC_0x0F; 	/*   */

	I08U gpioAlt_0x10_07_00;
	I08U gpioAlt_0x10_15_08;
	I08U gpioAlt_0x10_23_16;

	I08U gpioDr_0x11_07_00;
	I08U gpioDr_0x11_15_08;
	I08U gpioDr_0x11_23_16;

	I08U gpioLr_0x12_07_00;
	I08U gpioLr_0x12_15_08;
	I08U gpioLr_0x12_23_16;

	I08U gpioDrv_0x13_07_00;
	I08U gpioDrv_0x13_15_08;
	I08U gpioDrv_0x13_23_16;

	I08U gpioPe_0x14_07_00;
	I08U gpioPe_0x14_15_08;
	I08U gpioPe_0x14_23_16;

	I08U gpioSDrv_0x15_07_00;
	I08U gpioSDrv_0x15_15_08;
	I08U gpioSDrv_0x15_23_16;

	I08U ioMisc_0x16;

	I08U streamDataConfig_0x1B;
	I08U streamDataConfig_0x1C;
	I08U streamDataConfig_0x1D;
	I08U streamDataConfig_0x1E;

	I08U periConfig_0x30;
	I08U periConfig_0x31;
	I08U periConfig_0x32;
	I08U periConfig_0x33;

	I08U bufferConfig_0x4E;
	I08U bufferConfig_0x4F;
	I08U bufferConfig_0x54;
	I08U bufferConfig_0x55;

	I08U bufferConfig_0x50;/*   */
	I08U bufferConfig_0x51;/*   */
	I08U bufferConfig_0x52;/*   */
	I08U bufferConfig_0x53;/*   */

	I08U bufferConfig_0x58;/*   */
	I08U bufferConfig_0x59;/*   */
	I08U bufferConfig_0x5A;/*   */
	I08U bufferConfig_0x5B;/*   */

	I08U bufferConfig_0x60;/*   */
	I08U bufferConfig_0x61;/*   */
	I08U bufferConfig_0x62;/*   */
	I08U bufferConfig_0x63;/*   */

	I08U bufferConfig_0x68;/*   */
	I08U bufferConfig_0x69;/*   */
	I08U bufferConfig_0x6A;/*   */
	I08U bufferConfig_0x6B;/*   */
} Tcc353xRegisterConfig_t;

typedef struct {
	/*               */
	I08S basebandName;

	/*                          */
	I08S boardType;

	/*                          */
	I08S commandInterface;

	/*                          */
	I08S streamInterface;

	/*                          */
	/*                          */
	/*                          */
	/*                          */
	/*                          */
	I08U address;

	/*                          */
	/*                                      */
	/*                                     */
	/*                         */
	I16U pll;

	/*                          */
	I32U oscKhz;

	/*                           */
	I16S diversityPosition;

	/*                                      */
	I16S useInterrupt;

	/*                     */
	/*                              */
	/*                                                            */
	I32U rfSwitchingGpioN;

	/*                               */
	/*                                */
	I32U rfSwitchingVhfLow;
	I32U rfSwitchingVhfHigh;
	I32U rfSwitchingUhf;
	I32U rfSwitchingReserved;

	/*                             */
	I16S rfType;

	/*                          */
	Tcc353xRegisterConfig_t *Config;
} Tcc353xOption_t;

typedef enum {
	TCC353X_NOT_USE_SRAMLIKE = 0,	/*                   */
	TCC353X_IF_I2C,
	TCC353X_IF_TCCSPI,
	TCC353X_NOT_USE_SDIO1BIT,	/*                   */
	TCC353X_NOT_USE_SDIO4BIT	/*                   */
} EnumCommandIo;

typedef enum {
	TCC353X_STREAM_IO_MAINIO = 0,
	TCC353X_STREAM_IO_PTS,		/*                   */
	TCC353X_STREAM_IO_STS,
	TCC353X_STREAM_IO_SPIMS,
	TCC353X_STREAM_IO_SPISLV,	/*                   */
	TCC353X_STREAM_IO_HPI_HEADERON,	/*                   */
	TCC353X_STREAM_IO_HPI_HEADEROFF,/*                   */
	TCC353X_STREAM_IO_MAX
} EnumStreamIo;

typedef enum {
	TCC353X_RETURN_FAIL_INVALID_HANDLE = -5,
	TCC353X_RETURN_FAIL_NULL_ACCESS = -4,
	TCC353X_RETURN_FAIL_UNKNOWN = -3,
	TCC353X_RETURN_FAIL_TIMEOUT = -2,
	TCC353X_RETURN_FAIL = -1,
	TCC353X_RETURN_SUCCESS = 0,
	TCC353X_RETURN_FIRST
} EnumReturn;

/*                                                                          */
typedef enum {
	TCC353X_CMD_NONE = 0,
	TCC353X_CMD_DSP_RESET
} EnumUserCmd;

/*                                                                          */
typedef struct {
	I08U AGC;
	I08U DCEC;
	I08U CTO;
	I08U CFO;
	I08U TMCC;
	I08U EWSFlag;
} IsdbLock_t;

/*                                                                           */
typedef struct {
	/*                                          */
	I32U pidFilterEnable;
	/*                                               */
	I32U syncByteFilterEnable;
	/*                                                         */
	I32U tsErrorFilterEnable;
	/*                                                      */
	I32U tsErrorInsertEnable;
} Tcc353xStreamFormat_t;

/*                                                                          */
typedef struct {
	/*             */
	I32U Pid[32];
	/*               */
	I32U numberOfPid;
} Tcc353xpidTable_t;

/*                                                                          */
typedef struct {
	I32U cmd;
	I32U word_cnt;
	I32U status;
	I32U data_array[7];
} mailbox_t;

/*                                                                          */

#define SRVTYPE_NONE                0x00
#define SRVTYPE_ISDB_TS             0x01

/*                                                                          */
typedef struct {
	I08U partialReceptionFlag;
	/*
                     
                         
                                
 */

	I16U transParammLayerA;
	/*
                  

                             
             
              
              
                               
 
                              
                          
          
          
          
           
            
               
                              

                                    
        
        
        
        
        
               
                              

                                    
          
         
           
           
               
                              
 */

	I16U transParammLayerB;
	/*
                  
 */

	I16U transParammLayerC;
	/*
                  
 */
} transInformation_t;

typedef struct {
	I08U systemId;
	/*
         
                                       
                        
 */
	
	I08U transParamSwitch;
	/*
                 
                  
                                    
 */
	
	I08U startFlagEmergencyAlarm;
	/*
                        
                       
                              
 */
	
	transInformation_t currentInfo;
	/*
                    
 */

	transInformation_t nextInfo;
	/*
                 
 */
	
	I08U phaseShiftCorrectionValue;
	/*
                          
                                                 
 */
} tmccInfo_t;

/*                                                                          */

typedef enum {
/*
             
                 
                  
                  
             
                                      
*/
	TCC353X_ZERO_IF = 0,
	TCC353X_LOW_IF
} EnumRFType;

typedef enum {
	TCC353X_ISDBT_1_OF_13SEG = 0,
	TCC353X_ISDBT_13SEG,
	TCC353X_ISDBTSB_1SEG,
	TCC353X_ISDBTSB_3SEG,
	TCC353X_ISDBTSB_1_OF_3SEG,
	TCC353X_ISDBTMM
} EnumSegmentType;

typedef enum {
    /*        */
    A_1st_1Seg = 0,
    A_2nd_1Seg,
    A_3rd_1Seg,
    A_4th_1Seg,
    A_5th_1Seg,
    A_6th_1Seg,
    A_7th_1Seg,
    A_1st_13Seg,
    A_2nd_13Seg,

    /*        */
    B_1st_13Seg,
    B_1st_1Seg,
    B_2nd_1Seg,
    B_3rd_1Seg,
    B_4th_1Seg,
    B_5th_1Seg,
    B_6th_1Seg,
    B_7th_1Seg,
    B_2nd_13Seg,

    /*        */
    C_1st_13Seg,
    C_2nd_13Seg,
    C_1st_1Seg,
    C_2nd_1Seg,
    C_3rd_1Seg,
    C_4th_1Seg,
    C_5th_1Seg,
    C_6th_1Seg,
    C_7th_1Seg,

    UserDefine_Tmm1Seg,
    UserDefine_Tmm13Seg
} EnumTmmIdx;

typedef struct {
	/*                  */
	I32S rfIfType;
	/*                       */
	I32S segmentType;
	/*                             */
	I32S tmmSet;
	/*                                  */
	I32S tsbStartSubChannelNum;

	/*                                                       */
	I32U userFifothr;
	/*                                         */
	I32U BandwidthMHz;
} Tcc353xTuneOptions;

/*                                                                     
                           
  
  
                                       
                                     
  
  
              
                                               
                                              
                                             
                                             
                                                       
                               
  
  
         
                               
                                                              
                                                              
                                                              
  
                                                   
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                        
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
  
                                                                      */

typedef enum {
	ENUM_NO_LNA_GAIN_CONTROL = -1,
	ENUM_LNA_GAIN_LOW,
	ENUM_LNA_GAIN_HIGH
} EnumTcc353xLnaGainStatus;
 
#ifdef __cplusplus
	};
#endif

#endif
