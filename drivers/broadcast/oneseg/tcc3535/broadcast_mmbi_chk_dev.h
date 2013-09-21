#ifndef	__BROADCAST_MMBI_CHK_DEV_H__
#define	__BROADCAST_MMBI_CHK_DEV_H__

#ifndef	__BROADCAST_MMBI_CHK_DEV_H__
#include <string.h>
#endif

/*                                                                                                      */
/*                                                                                                      */
/*                                                                                                      */

enum ioctl_command_no {
	IOCTL_DRV_OPEN 				= 0x1001,
	IOCTL_DRV_CLOSE				= 0x1002,
	IOCTL_SET_RF_BAND 			= 0x1003,
	IOCTL_SET_CHANNEL 			= 0x1004,
	IOCTL_SET_MONITOR 			= 0x1005,
	IOCTL_GET_MONITOR 			= 0x1006,	
	IOCTL_SET_MONITOR_APP		= 0x1007,
	IOCTL_SET_UIM_INFO			= 0x1008,
	IOCTL_GET_UIM_INFO			= 0x1009,
	IOCTL_SET_ANTENNA_INFO		= 0x100A
};

/*                                                                                                      */
/*                                                                                                      */
/*                                                                                                      */
struct ioctl_sig_info {
	int cn;
	int rssi;
	int berA;
	int berB;
	int berC;
	int perA;
	int perB;
	int perC;
};
typedef struct ioctl_sig_info ioctl_sig_info_t;

struct ioctl_monitor_info {
	int drv_openning; //                    
	int rf_mode; //                 
	int channel;
	int monitor_app; //                   
	ioctl_sig_info_t sig_info;
};
typedef struct ioctl_monitor_info ioctl_monitor_info_t;

struct mmbi_chk_dev_cmdcontrol {
	ioctl_monitor_info_t MONITOR_INFO;
};
typedef struct mmbi_chk_dev_cmdcontrol mmbi_chk_dev_cmdcontrol_t;

typedef struct mmbi_chk_dev_uim_info
{
	unsigned char iccid[20];
}mmbi_chk_dev_uim_info_t;

//                           
typedef struct mmbi_chk_dev_antenna_info
{
	int antenna_info; //                                         
}mmbi_chk_dev_antenna_info_t;


#endif /*                     */

