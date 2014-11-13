
/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/spmi.h>
#include <linux/qpnp/pwm.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/zwait.h>

#define WLED_MOD_EN_REG(base, n)	(base + 0x60 + n*0x10)
#define WLED_IDAC_DLY_REG(base, n)	(WLED_MOD_EN_REG(base, n) + 0x01)
#define WLED_FULL_SCALE_REG(base, n)	(WLED_IDAC_DLY_REG(base, n) + 0x01)
#define WLED_MOD_SRC_SEL_REG(base, n)	(WLED_FULL_SCALE_REG(base, n) + 0x01)

/* wled control registers */
#define WLED_BRIGHTNESS_CNTL_LSB(base, n)	(base + 0x40 + 2*n)
#define WLED_BRIGHTNESS_CNTL_MSB(base, n)	(base + 0x41 + 2*n)
#define WLED_MOD_CTRL_REG(base)			(base + 0x46)
#define WLED_SYNC_REG(base)			(base + 0x47)
#define WLED_FDBCK_CTRL_REG(base)		(base + 0x48)
#define WLED_SWITCHING_FREQ_REG(base)		(base + 0x4C)
#define WLED_OVP_CFG_REG(base)			(base + 0x4D)
#define WLED_BOOST_LIMIT_REG(base)		(base + 0x4E)
#define WLED_CURR_SINK_REG(base)		(base + 0x4F)
#define WLED_HIGH_POLE_CAP_REG(base)		(base + 0x58)
#define WLED_CURR_SINK_MASK		0xE0
#define WLED_CURR_SINK_SHFT		0x05
#define WLED_DISABLE_ALL_SINKS		0x00
#define WLED_SWITCH_FREQ_MASK		0x0F
#define WLED_OVP_VAL_MASK		0x03
#define WLED_OVP_VAL_BIT_SHFT		0x00
#define WLED_BOOST_LIMIT_MASK		0x07
#define WLED_BOOST_LIMIT_BIT_SHFT	0x00
#define WLED_BOOST_ON			0x80
#define WLED_BOOST_OFF			0x00
#define WLED_EN_MASK			0x80
#define WLED_NO_MASK			0x00
#define WLED_CP_SELECT_MAX		0x03
#define WLED_CP_SELECT_MASK		0x02
#define WLED_USE_EXT_GEN_MOD_SRC	0x01
#define WLED_CTL_DLY_STEP		200
#define WLED_CTL_DLY_MAX		1400
#define WLED_MAX_CURR			25
#define WLED_NO_CURRENT			0x00
#define WLED_OVP_DELAY			1000
#define WLED_MSB_MASK			0x0F
#define WLED_MAX_CURR_MASK		0x1F
#define WLED_OP_FDBCK_MASK		0x07
#define WLED_OP_FDBCK_BIT_SHFT		0x00
#define WLED_OP_FDBCK_DEFAULT		0x00

#define WLED_MAX_LEVEL			4095
#define WLED_8_BIT_MASK			0xFF
#define WLED_4_BIT_MASK			0x0F
#define WLED_8_BIT_SHFT			0x08
#define WLED_MAX_DUTY_CYCLE		0xFFF

#define WLED_SYNC_VAL			0x07
#define WLED_SYNC_RESET_VAL		0x00

#define PMIC_VER_8026			0x04
#define PMIC_VERSION_REG		0x0105

#define WLED_DEFAULT_STRINGS		0x01
#define WLED_DEFAULT_OVP_VAL		0x02
#define WLED_BOOST_LIM_DEFAULT		0x03
#define WLED_CP_SEL_DEFAULT		0x00
#define WLED_CTRL_DLY_DEFAULT		0x00
#define WLED_SWITCH_FREQ_DEFAULT	0x0B

#define FLASH_SAFETY_TIMER(base)	(base + 0x40)
#define FLASH_MAX_CURR(base)		(base + 0x41)
#define FLASH_LED_0_CURR(base)		(base + 0x42)
#define FLASH_LED_1_CURR(base)		(base + 0x43)
#define FLASH_CLAMP_CURR(base)		(base + 0x44)
#define FLASH_LED_TMR_CTRL(base)	(base + 0x48)
#define FLASH_HEADROOM(base)		(base + 0x4A)
#define FLASH_STARTUP_DELAY(base)	(base + 0x4B)
#define FLASH_MASK_ENABLE(base)		(base + 0x4C)
#define FLASH_VREG_OK_FORCE(base)	(base + 0x4F)
#define FLASH_ENABLE_CONTROL(base)	(base + 0x46)
#define FLASH_LED_STROBE_CTRL(base)	(base + 0x47)
#define FLASH_LED_UNLOCK_SECURE(base)	(base + 0xD0)
#define FLASH_LED_TORCH(base)		(base + 0xE4)
#define FLASH_FAULT_DETECT(base)	(base + 0x51)
#define FLASH_PERIPHERAL_SUBTYPE(base)	(base + 0x05)
#define FLASH_CURRENT_RAMP(base)	(base + 0x54)
#ifdef CONFIG_MACH_MSM8974_B1_KR
#define FLASH_LED_DBC_CTRL(base) (base + 0x5A)
#endif

#define FLASH_MAX_LEVEL			0x4F
#define TORCH_MAX_LEVEL			0x0F
#define	FLASH_NO_MASK			0x00

#define FLASH_MASK_1			0x20
#define FLASH_MASK_REG_MASK		0xE0
#define FLASH_HEADROOM_MASK		0x03
#define FLASH_SAFETY_TIMER_MASK		0x7F
#define FLASH_CURRENT_MASK		0xFF
#define FLASH_MAX_CURRENT_MASK		0x7F
#define FLASH_TMR_MASK			0x03
#define FLASH_TMR_WATCHDOG		0x03
#define FLASH_TMR_SAFETY		0x00
#define FLASH_FAULT_DETECT_MASK		0X80
#define FLASH_HW_VREG_OK		0x40
#define FLASH_VREG_MASK			0xC0
#define FLASH_STARTUP_DLY_MASK		0x02
#define FLASH_CURRENT_RAMP_MASK		0xBF

#define FLASH_ENABLE_ALL		0xE0
#define FLASH_ENABLE_MODULE		0x80
#define FLASH_ENABLE_MODULE_MASK	0x80
#define FLASH_DISABLE_ALL		0x00
#define FLASH_ENABLE_MASK		0xE0
#define FLASH_ENABLE_LED_0		0xC0
#define FLASH_ENABLE_LED_1		0xA0
#define FLASH_INIT_MASK			0xE0
#define	FLASH_SELFCHECK_ENABLE		0x80
#define FLASH_RAMP_STEP_27US		0xBF

#define FLASH_STROBE_SW			0xC0
#define FLASH_STROBE_HW			0x04
#define FLASH_STROBE_MASK		0xC7
#define FLASH_LED_0_OUTPUT		0x80
#define FLASH_LED_1_OUTPUT		0x40

#define FLASH_CURRENT_PRGM_MIN		1
#define FLASH_CURRENT_PRGM_SHIFT	1
#define FLASH_CURRENT_MAX		0x4F
#define FLASH_CURRENT_TORCH		0x07

#define FLASH_DURATION_200ms		0x13
#define FLASH_CLAMP_200mA		0x0F

#define FLASH_TORCH_MASK		0x03
#define FLASH_LED_TORCH_ENABLE		0x00
#define FLASH_LED_TORCH_DISABLE		0x03
#define FLASH_UNLOCK_SECURE		0xA5
#define FLASH_SECURE_MASK		0xFF

#define FLASH_SUBTYPE_DUAL		0x01
#define FLASH_SUBTYPE_SINGLE		0x02

#define FLASH_RAMP_UP_DELAY_US		1000
#define FLASH_RAMP_DN_DELAY_US		2160

#define LED_TRIGGER_DEFAULT		"none"

#define RGB_LED_SRC_SEL(base)		(base + 0x45)
#define RGB_LED_EN_CTL(base)		(base + 0x46)
#define RGB_LED_ATC_CTL(base)		(base + 0x47)

#define RGB_MAX_LEVEL			LED_FULL
#define RGB_LED_ENABLE_RED		0x80
#define RGB_LED_ENABLE_GREEN		0x40
#define RGB_LED_ENABLE_BLUE		0x20
#define RGB_LED_SOURCE_VPH_PWR		0x01
#define RGB_LED_ENABLE_MASK		0xE0
#define RGB_LED_SRC_MASK		0x03
#define QPNP_LED_PWM_FLAGS	(PM_PWM_LUT_LOOP | PM_PWM_LUT_RAMP_UP)
#define QPNP_LUT_RAMP_STEP_DEFAULT	255
#define	PWM_LUT_MAX_SIZE		63
#define	PWM_GPLED_LUT_MAX_SIZE		31
#define RGB_LED_DISABLE			0x00

#define KPDBL_MAX_LEVEL			LED_FULL
#define KPDBL_ROW_SRC_SEL(base)		(base + 0x40)
#define KPDBL_ENABLE(base)		(base + 0x46)
#define KPDBL_GLOBAL_ROW_SCAN(base)	(base + 0xB1)
#define KPDBL_PWM_PER_ADJ_LSB(base)	(base + 0xB3)
#define KPDBL_PWM_PER_ADJ_MSB(base)	(base + 0xB4)
#define KPDBL_ROW_SRC(base)		(base + 0xE5)

#define MPP_MAX_LEVEL			LED_FULL
#define LED_MPP_MODE_CTRL(base)		(base + 0x40)
#define LED_MPP_VIN_CTRL(base)		(base + 0x41)
#define LED_MPP_EN_CTRL(base)		(base + 0x46)
#define LED_MPP_SINK_CTRL(base)		(base + 0x4C)

#define LED_MPP_CURRENT_MIN		5
#define LED_MPP_CURRENT_MAX		40
#define LED_MPP_VIN_CTRL_DEFAULT	0
#define LED_MPP_CURRENT_PER_SETTING	5
#define LED_MPP_SOURCE_SEL_DEFAULT	LED_MPP_MODE_ENABLE

#define LED_MPP_SINK_MASK		0x07
#define LED_MPP_MODE_MASK		0x7F
#define LED_MPP_VIN_MASK		0x03
#define LED_MPP_EN_MASK			0x80
#define LED_MPP_SRC_MASK		0x0F
#define LED_MPP_MODE_CTRL_MASK		0x70

#define LED_MPP_MODE_SINK		(0x06 << 4)
#define LED_MPP_MODE_ENABLE		0x01
#define LED_MPP_MODE_OUTPUT		0x10
#define LED_MPP_MODE_DISABLE		0x00
#define LED_MPP_EN_ENABLE		0x80
#define LED_MPP_EN_DISABLE		0x00

#define MPP_SOURCE_DTEST1		0x08

#define KPDBL_ROW_SRC_SEL_VAL_MASK	0x0F
#define KPDBL_ROW_SCAN_EN_MASK		0x80
#define KPDBL_ROW_SCAN_VAL_MASK		0x0F
#define KPDBL_ROW_SCAN_EN_SHIFT		7
#define KPDBL_MODULE_EN			0x80
#define KPDBL_MODULE_DIS		0x00
#define KPDBL_MODULE_EN_MASK		0x80

/**
 * enum qpnp_leds - QPNP supported led ids
 * @QPNP_ID_WLED - White led backlight
 */
enum qpnp_leds {
	QPNP_ID_WLED = 0,
	QPNP_ID_FLASH1_LED0,
	QPNP_ID_FLASH1_LED1,
	QPNP_ID_RGB_RED,
	QPNP_ID_RGB_GREEN,
	QPNP_ID_RGB_BLUE,
	QPNP_ID_LED_MPP,
	QPNP_ID_KPDBL,
	QPNP_ID_MAX,
};

/* current boost limit */
enum wled_current_boost_limit {
	WLED_CURR_LIMIT_105mA,
	WLED_CURR_LIMIT_385mA,
	WLED_CURR_LIMIT_525mA,
	WLED_CURR_LIMIT_805mA,
	WLED_CURR_LIMIT_980mA,
	WLED_CURR_LIMIT_1260mA,
	WLED_CURR_LIMIT_1400mA,
	WLED_CURR_LIMIT_1680mA,
};

/* over voltage protection threshold */
enum wled_ovp_threshold {
	WLED_OVP_35V,
	WLED_OVP_32V,
	WLED_OVP_29V,
	WLED_OVP_37V,
};

enum flash_headroom {
	HEADROOM_250mV = 0,
	HEADROOM_300mV,
	HEADROOM_400mV,
	HEADROOM_500mV,
};

enum flash_startup_dly {
	DELAY_10us = 0,
	DELAY_32us,
	DELAY_64us,
	DELAY_128us,
};

enum led_mode {
	PWM_MODE = 0,
	LPG_MODE,
	MANUAL_MODE,
};

static u8 wled_debug_regs[] = {
	/* common registers */
	0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	/* LED1 */
	0x60, 0x61, 0x62, 0x63, 0x66,
	/* LED2 */
	0x70, 0x71, 0x72, 0x73, 0x76,
	/* LED3 */
	0x80, 0x81, 0x82, 0x83, 0x86,
};

static u8 flash_debug_regs[] = {
	0x40, 0x41, 0x42, 0x43, 0x44, 0x48, 0x49, 0x4b, 0x4c,
	0x4f, 0x46, 0x47,
};

static u8 rgb_pwm_debug_regs[] = {
	0x45, 0x46, 0x47,
};

static u8 mpp_debug_regs[] = {
	0x40, 0x41, 0x42, 0x45, 0x46, 0x4c,
};

static u8 kpdbl_debug_regs[] = {
	0x40, 0x46, 0xb1, 0xb3, 0xb4, 0xe5,
};

/**
 *  pwm_config_data - pwm configuration data
 *  @lut_params - lut parameters to be used by pwm driver
 *  @pwm_device - pwm device
 *  @pwm_channel - pwm channel to be configured for led
 *  @pwm_period_us - period for pwm, in us
 *  @mode - mode the led operates in
 *  @old_duty_pcts - storage for duty pcts that may need to be reused
 *  @default_mode - default mode of LED as set in device tree
 *  @use_blink - use blink sysfs entry
 *  @blinking - device is currently blinking w/LPG mode
 */
struct pwm_config_data {
	struct lut_params	lut_params;
	struct pwm_device	*pwm_dev;
	int			pwm_channel;
#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
	int			leds_on;
#endif
	u32			pwm_period_us;
	struct pwm_duty_cycles	*duty_cycles;
	int	*old_duty_pcts;
	u8	mode;
	u8	default_mode;
	bool use_blink;
	bool blinking;
};

/**
 *  wled_config_data - wled configuration data
 *  @num_strings - number of wled strings supported
 *  @ovp_val - over voltage protection threshold
 *  @boost_curr_lim - boot current limit
 *  @cp_select - high pole capacitance
 *  @ctrl_delay_us - delay in activation of led
 *  @dig_mod_gen_en - digital module generator
 *  @cs_out_en - current sink output enable
 *  @op_fdbck - selection of output as feedback for the boost
 */
struct wled_config_data {
	u8	num_strings;
	u8	ovp_val;
	u8	boost_curr_lim;
	u8	cp_select;
	u8	ctrl_delay_us;
	u8	switch_freq;
	u8	op_fdbck;
	u8	pmic_version;
	bool	dig_mod_gen_en;
	bool	cs_out_en;
};

/**
 *  mpp_config_data - mpp configuration data
 *  @pwm_cfg - device pwm configuration
 *  @current_setting - current setting, 5ma-40ma in 5ma increments
 *  @source_sel - source selection
 *  @mode_ctrl - mode control
 *  @vin_ctrl - input control
 *  @min_brightness - minimum brightness supported
 *  @pwm_mode - pwm mode in use
 */
struct mpp_config_data {
	struct pwm_config_data	*pwm_cfg;
	u8	current_setting;
	u8	source_sel;
	u8	mode_ctrl;
	u8	vin_ctrl;
	u8	min_brightness;
	u8 pwm_mode;
};

/**
 *  flash_config_data - flash configuration data
 *  @current_prgm - current to be programmed, scaled by max level
 *  @clamp_curr - clamp current to use
 *  @headroom - headroom value to use
 *  @duration - duration of the flash
 *  @enable_module - enable address for particular flash
 *  @trigger_flash - trigger flash
 *  @startup_dly - startup delay for flash
 *  @strobe_type - select between sw and hw strobe
 *  @peripheral_subtype - module peripheral subtype
 *  @current_addr - address to write for current
 *  @second_addr - address of secondary flash to be written
 *  @safety_timer - enable safety timer or watchdog timer
 *  @torch_enable - enable flash LED torch mode
 *  @flash_reg_get - flash regulator attached or not
 *  @flash_on - flash status, on or off
 *  @torch_on - torch status, on or off
 *  @flash_boost_reg - boost regulator for flash
 *  @torch_boost_reg - boost regulator for torch
 */
struct flash_config_data {
	u8	current_prgm;
	u8	clamp_curr;
	u8	headroom;
	u8	duration;
	u8	enable_module;
	u8	trigger_flash;
	u8	startup_dly;
	u8	strobe_type;
	u8	peripheral_subtype;
	u16	current_addr;
	u16	second_addr;
	bool	safety_timer;
	bool	torch_enable;
	bool	flash_reg_get;
	bool	flash_on;
	bool	torch_on;
	struct regulator *flash_boost_reg;
	struct regulator *torch_boost_reg;
};

/**
 *  kpdbl_config_data - kpdbl configuration data
 *  @pwm_cfg - device pwm configuration
 *  @mode - running mode: pwm or lut
 *  @row_id - row id of the led
 *  @row_src_vbst - 0 for vph_pwr and 1 for vbst
 *  @row_src_en - enable row source
 *  @always_on - always on row
 *  @lut_params - lut parameters to be used by pwm driver
 *  @duty_cycles - duty cycles for lut
 */
struct kpdbl_config_data {
	struct pwm_config_data	*pwm_cfg;
	u32	row_id;
	bool	row_src_vbst;
	bool	row_src_en;
	bool	always_on;
	struct pwm_duty_cycles  *duty_cycles;
	struct lut_params	lut_params;
};

/**
 *  rgb_config_data - rgb configuration data
 *  @pwm_cfg - device pwm configuration
 *  @enable - bits to enable led
 */
struct rgb_config_data {
	struct pwm_config_data	*pwm_cfg;
	u8	enable;
};

/**
 * struct qpnp_led_data - internal led data structure
 * @led_classdev - led class device
 * @delayed_work - delayed work for turning off the LED
 * @work - workqueue for led
 * @id - led index
 * @base_reg - base register given in device tree
 * @lock - to protect the transactions
 * @reg - cached value of led register
 * @num_leds - number of leds in the module
 * @max_current - maximum current supported by LED
 * @default_on - true: default state max, false, default state 0
 * @turn_off_delay_ms - number of msec before turning off the LED
 */
struct qpnp_led_data {
	struct led_classdev	cdev;
	struct spmi_device	*spmi_dev;
	struct delayed_work	dwork;
	struct work_struct	work;
	int			id;
	u16			base;
	u8			reg;
	u8			num_leds;
	struct mutex		lock;
	struct wled_config_data *wled_cfg;
	struct flash_config_data	*flash_cfg;
	struct kpdbl_config_data	*kpdbl_cfg;
	struct rgb_config_data	*rgb_cfg;
	struct mpp_config_data	*mpp_cfg;
	int			max_current;
	bool			default_on;
	int			turn_off_delay_ms;
};

static int num_kpbl_leds_on;

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
/*  duty pcts array is...
 *   0......62
 *   LUT TABLE
 *   63      64    65    66
 *   START, LENGTH, DUTY, PAUSE [RED]
 *   67     68     69    70
 *   START, LENGTH, DUTY, PAUSE [GREEN]
 *   71      72     73    74
 *   START, LENGTH, DUTY, PAUSE [BLUE]
 *   75   76   77    78
 *   R_FLAG G_FLAG B_FLAG RAMP_STEP_MS
 */

/* Default */
static int leds_pwm_duty_pcts0[79] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	0, 0, 0, 65
};

/* #1 ID_POWER_ON (GB) */
static int leds_pwm_duty_pcts1[79] = {
	0, 6, 11, 17, 22, 28, 34, 39, 45, 50,
	56, 62, 67, 72, 75, 80, 83, 88, 91, 94,
	97, 100, 103, 106, 109, 111, 113, 116, 116, 117,
	121, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 4, 7, 10, 14, 18,
	22, 26, 31, 36, 44, 48, 54, 60, 67, 74, 81, 84, 0,
	1, 1, 0, 0,
	0, 31, 32, 0,
	31, 31, 32, 0,
	71, 71, 71, 33
};

/* #2 ID_LCD_ON (RGB) - A1 not use */
static int leds_pwm_duty_pcts2[79] = {
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 99, 98, 95, 90, 85, 79,
	73, 65, 58, 50, 42, 35, 27, 21, 15, 10,
	5, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 54, 19, 0,
	1, 54, 19, 0,
	1, 54, 19, 0,
	2, 2, 2, 65
};

/* #3 ID_CHARGING (R) */
static int leds_pwm_duty_pcts3[79] = {
#if defined(CONFIG_MACH_MSM8974_G2_VZW) || defined(CONFIG_MACH_MSM8974_G2_ATT) || defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_TMO_US) || defined(CONFIG_MACH_MSM8974_G2_OPEN_COM) || defined(CONFIG_MACH_MSM8974_G2_OPT_AU) || defined(CONFIG_MACH_MSM8974_Z_US) || defined(CONFIG_MACH_MSM8974_G2_CA)
	1, 1, 1, 2, 2, 3, 3, 4, 4, 5,
	5, 5, 6, 6, 7, 7, 7, 8, 8, 9,
	9, 10, 10, 11, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 25,
	27, 28, 29, 31, 32, 34, 35, 37, 39, 41,
	43, 46, 48, 51, 54, 56, 57, 58, 59, 59, 60, 60, 60,
#else
	2, 3, 3, 4, 4, 5, 5, 6, 6, 7,
	7, 8, 9, 10, 11, 12, 13, 14, 14, 15,
	17, 18, 20, 21, 22, 24, 26, 28, 30, 32,
	34, 35, 37, 39, 41, 43, 45, 47, 49, 50,
	53, 56, 58, 61, 64, 67, 70, 74, 78, 82,
	86, 91, 96, 101, 107, 112, 114, 115, 117, 118, 119, 119, 119,
#endif
	0, 61, 256, 600,
	1, 1, 0, 0,
	1, 1, 0, 0,
	79, 79, 79, 44
};

/* #4 ID_CHARGING_FULL (G) */
static int leds_pwm_duty_pcts4[79] = {
#if defined(CONFIG_MACH_MSM8974_G2_VZW) || defined(CONFIG_MACH_MSM8974_G2_ATT) || defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_TMO_US) || defined(CONFIG_MACH_MSM8974_G2_OPEN_COM) || defined(CONFIG_MACH_MSM8974_G2_OPT_AU) || defined(CONFIG_MACH_MSM8974_Z_US) || defined(CONFIG_MACH_MSM8974_G2_CA)
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
#else
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
#endif
	1, 1, 0, 0,
	1, 60, 30, 0,
	1, 1, 0, 0,
	3, 3, 3, 65
};

/* #5 ID_CALENDAR_REMIND (GB) - A1 use framework pattern */
static int leds_pwm_duty_pcts5[79] = {
	100, 99, 98, 97, 95, 93, 90, 88, 84, 81,
	77, 73, 68, 64, 59, 54, 48, 43, 37, 31,
	25, 19, 13, 6, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 100, 99, 98, 97, 95,
	93, 90, 88, 84, 81, 77, 73, 68, 64, 59,
	54, 48, 43, 37, 31, 25, 19, 13, 6, 0, 0, 0, 0,
	1, 60, 0, 0,
	1, 60, 12, 0,
	1, 60, 12, 0,
	2, 2, 2, 65
};

/* #6 ID_POWER_OFF (GB) */
static int leds_pwm_duty_pcts6[79] = {
	0, 6, 11, 17, 22, 28, 34, 39, 45, 50,
	56, 62, 67, 72, 75, 80, 83, 88, 91, 94,
	97, 100, 103, 106, 109, 111, 113, 116, 116, 117,
	121, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 4, 7, 10, 14, 18,
	22, 26, 31, 36, 44, 48, 54, 60, 67, 74, 81, 84, 0,
	1, 1, 0, 0,
	0, 31, 32, 0,
	31, 31, 32, 0,
	71, 71, 71, 33
};

/* #7 ID_MISSED_NOTI (normal(G), DCM(RGB) */
static int leds_pwm_duty_pcts7[79] = {
#if defined(CONFIG_MACH_MSM8974_G2_DCM)
	0, 49, 47, 39, 30, 20, 10, 2, 0, 0, /* red*1.6 */
	0, 0, 49, 47, 39, 30, 20, 10, 2, 0,
	0, 137, 130, 109, 82, 55, 27, 7, 0, 0,
	0, 0, 137, 130, 109, 82, 55, 27, 7, 0,
	0, 77, 73, 61, 46, 31, 15, 4, 0, 0,
	0, 0, 77, 73, 61, 46, 31, 15, 4, 0, 0, 0, 0,
	0, 20, 148, 2280,
	20, 20, 410, 2280,
	40, 20, 230, 2280,
	91, 91, 91, 36
#else
	0, 170, 168, 167, 165, 162, 158, 153, 150, 143,
	138, 131, 124, 116, 109, 100, 92, 82, 73, 63,
	53, 43, 32, 22, 10, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 170, 168, 167, 165,
	162, 158, 153, 150, 143, 138, 131, 124, 116, 109,
	100, 92, 82, 73, 63, 53, 43, 32, 22, 10, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 61, 0, 8000,
	0, 61, 12, 8000,
	0, 61, 0, 8000,
#else
	0, 61, 0, 2280,
	0, 61, 12, 2280,
	0, 61, 0, 2280,
#endif
	91, 91, 91, 12
#endif
};

/* #8 ID_ALARM (RG) - A1 use framework pattern */
static int leds_pwm_duty_pcts8[79] = {
	100, 100, 100, 100, 98, 96, 90, 84, 83, 81,
	80, 77, 73, 69, 64, 60, 55, 50, 45, 40,
	34, 29, 23, 17, 12, 6, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 3, 5, 8, 10, 13,
	18, 23, 25, 27, 29, 31, 33, 34, 35, 30,
	26, 23, 21, 20, 19, 10, 8, 5, 1, 0, 0, 0, 0,
	1, 31, 36, 8,
	32, 31, 36, 8,
	32, 1, 0, 8,
	11, 11, 11, 65
};

/* #12 ID_VOLUME_UP (RB) -A1 not use */
static int leds_pwm_duty_pcts12[79] = {
	100, 100, 99, 98, 96, 94, 91, 88, 84, 81,
	76, 72, 67, 63, 58, 53, 47, 42, 37, 33,
	28, 24, 19, 16, 12, 9, 6, 4, 2, 1,
	0, 71, 70, 70, 69, 68, 66, 64, 62, 60,
	57, 54, 51, 48, 44, 41, 37, 34, 30, 26,
	23, 20, 17, 14, 11, 9, 6, 4, 3, 2, 1, 0, 0,
	1, 31, 26, 0,
	32, 1, 0, 0,
	32, 31, 26, 0,
	2, 2, 2, 65
};

/* #13 ID_VOLUME_DOWN (GB) -A1 not use*/
static int leds_pwm_duty_pcts13[79] = {
	49, 49, 49, 48, 47, 46, 45, 43, 42, 40,
	38, 36, 33, 31, 28, 26, 23, 21, 19, 16,
	14, 12, 10, 8, 6, 4, 3, 2, 1, 1,
	0, 100, 100, 99, 98, 96, 94, 91, 88, 84,
	81, 76, 72, 67, 63, 58, 53, 47, 42, 37,
	33, 28, 24, 19, 16, 12, 9, 6, 4, 2, 1, 0, 0,
	1, 1, 0, 0,
	1, 31, 26, 0,
	32, 31, 26, 0,
	2, 2, 2, 65
};

/* #14 ID_FAVORITE_MISSED_NOTI (RGB) -A1 not use */
static int leds_pwm_duty_pcts14[79] = {
	100, 100, 100, 100, 100, 92, 84, 76, 68, 60,
	52, 44, 36, 28, 20, 100, 100, 100, 100, 100,
	90, 80, 70, 60, 50, 40, 30, 20, 10, 0,
	40, 40, 40, 40, 40, 37, 34, 30, 27, 24,
	21, 18, 14, 11, 8, 40, 40, 40, 40, 40,
	36, 32, 28, 24, 20, 16, 12, 8, 4, 0, 0, 0, 0,
	0, 30, 47, 2590,
	30, 30, 47, 2590,
	30, 30, 47, 2590,
	11, 11, 11, 65
};

/* #17 ID_MISSED_NOTI_PINK (RGB) */
static int leds_pwm_duty_pcts17[79] = {
	0, 136, 129, 109, 82, 54, 27, 7, 0, 0, /* red*1.6 */
	0, 0, 136, 129, 109, 82, 54, 27, 7, 0,
	0, 41, 39, 33, 24, 16, 8, 2, 0, 0,
	0, 0, 41, 39, 33, 24, 16, 8, 2, 0,
	0, 23, 22, 18, 14, 9, 5, 1, 0, 0,
	0, 0, 23, 22, 18, 14, 9, 5, 1, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 20, 408, 8000,
	20, 20, 122, 8000,
	40, 20, 69, 8000,
#else
	0, 20, 408, 2280,
	20, 20, 122, 2280,
	40, 20, 69, 2280,
#endif
	91, 91, 91, 36
};

/* #18 ID_MISSED_NOTI_BLUE (GB) */
static int leds_pwm_duty_pcts18[79] = {
	0, 51, 46, 39, 31, 26, 20, 14, 10, 7,
	3, 0, 0, 0, 0, 51, 46, 39, 31, 26,
	20, 14, 10, 7, 3, 0, 0, 0, 0, 0,
	0, 85, 77, 64, 51, 43, 32, 24, 16, 11,
	5, 0, 0, 0, 0, 85, 77, 64, 51, 43,
	32, 24, 16, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 1, 0, 8000,
	0, 26, 154, 8000,
	30, 26, 255, 8000,
#else
	0, 1, 0, 2280,
	0, 26, 154, 2280,
	30, 26, 255, 2280,
#endif
	91, 91, 91, 30
};

/* #19 ID_MISSED_NOTI_ORANGE (RG) */
static int leds_pwm_duty_pcts19[79] = {
	0, 119, 107, 89, 71, 60, 45, 33, 23, 15, /* red*1.4 */
	7, 0, 0, 0, 0, 119, 107, 89, 71, 60,
	45, 33, 23, 15, 7, 0, 0, 0, 0, 0,
	0, 68, 61, 51, 41, 34, 26, 19, 13, 9,
	4, 0, 0, 0, 0, 68, 61, 51, 41, 34,
	26, 19, 13, 9, 4, 0, 0, 0, 0, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 26, 408, 8000,
	30, 26, 204, 8000,
	0, 1, 0, 8000,
#else
	0, 26, 408, 2280,
	30, 26, 204, 2280,
	0, 1, 0, 2280,
#endif
	91, 91, 91, 30
};

/* #20 ID_MISSED_NOTI_YELLOW (RG) */
static int leds_pwm_duty_pcts20[79] = {
	0, 119, 107, 89, 71, 60, 45, 33, 23, 15, /* red*1.4 */
	7, 0, 0, 0, 0, 119, 107, 89, 71, 60,
	45, 33, 23, 15, 7, 0, 0, 0, 0, 0,
	0, 153, 138, 115, 92, 77, 58, 43, 29, 20,
	9, 0, 0, 0, 0, 153, 138, 115, 92, 77,
	58, 43, 29, 20, 9, 0, 0, 0, 0, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 26, 408, 8000,
	30, 26, 460, 8000,
	0, 1, 0, 8000,
#else
	0, 26, 408, 2280,
	30, 26, 460, 2280,
	0, 1, 0, 2280,
#endif
	91, 91, 91, 30
};

/* #29 ID_MISSED_NOTI_TURQUOISE (GB) */
static int leds_pwm_duty_pcts29[79] = {
	0, 170, 153, 128, 102, 85, 65, 48, 32, 22,
	10, 0, 0, 0, 0, 170, 153, 128, 102, 85,
	65, 48, 32, 22, 10, 0, 0, 0, 0, 0,
	0, 41, 37, 31, 24, 20, 15, 11, 8, 5,
	2, 0, 0, 0, 0, 41, 37, 31, 24, 20,
	15, 11, 8, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 1, 0, 8000,
	0, 26, 510, 8000,
	30, 26, 122, 8000,
#else
	0, 1, 0, 2280,
	0, 26, 510, 2280,
	30, 26, 122, 2280,
#endif
	91, 91, 91, 30
};

/* #30 ID_MISSED_NOTI_PURPLE (RGB) */
static int leds_pwm_duty_pcts30[79] = {
	0, 95, 91, 76, 57, 38, 19, 5, 0, 0, /* red*1.6 */
	0, 0, 95, 91, 76, 57, 38, 19, 5, 0,
	0, 17, 16, 13, 10, 7, 3, 1, 0, 0,
	0, 0, 17, 16, 13, 10, 7, 3, 1, 0,
	0, 85, 81, 68, 51, 34, 17, 4, 0, 0,
	0, 0, 85, 81, 68, 51, 34, 17, 4, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 20, 268, 8000,
	20, 20, 50, 8000,
	40, 20, 255, 8000,
#else
	0, 20, 268, 2280,
	20, 20, 50, 2280,
	40, 20, 255, 2280,
#endif
	91, 91, 91, 36
};

/* #31 ID_MISSED_NOTI_RED (R) */
static int leds_pwm_duty_pcts31[79] = {
	0, 136, 135, 133, 132, 129, 126, 122, 120, 114,
	110, 105, 99, 92, 87, 80, 73, 65, 58, 50,
	42, 34, 26, 18, 8, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 136, 135, 133, 132,
	129, 126, 122, 120, 114, 110, 105, 99, 92, 87,
	80, 73, 65, 58, 50, 42, 34, 26, 18, 8, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 61, 408, 8000,
	0, 61, 0, 8000,
	0, 61, 0, 8000,
#else
	0, 61, 408, 2280,
	0, 61, 0, 2280,
	0, 61, 0, 2280,
#endif
	91, 91, 91, 12
};

/* #32 ID_MISSED_NOTI_LIME (RG) */
static int leds_pwm_duty_pcts32[79] = {
	0, 71, 64, 53, 43, 36, 27, 20, 13, 9, /* red*1.4 */
	4, 0, 0, 0, 0, 71, 64, 53, 43, 36,
	27, 20, 13, 9, 4, 0, 0, 0, 0, 0,
	0, 170, 153, 128, 102, 85, 65, 48, 32, 22,
	10, 0, 0, 0, 0, 170, 153, 128, 102, 85,
	65, 48, 32, 22, 10, 0, 0, 0, 0, 0, 0, 0, 0,
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
	0, 26, 244, 8000,
	30, 26, 510, 8000,
	0, 1, 0, 8000,
#else
	0, 26, 244, 2280,
	30, 26, 510, 2280,
	0, 1, 0, 2280,
#endif
	91, 91, 91, 30
};

/* #101 ID_FELICA_ON (B)  */
static int leds_pwm_duty_pcts101[79] = {
	0, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	26, 27, 28, 29, 30, 31, 32, 33, 33, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	1, 28, 1, 0,
	7, 7, 7, 13
};

/* #102 ID_GPS_ENABLED (B) */
static int leds_pwm_duty_pcts102[79] = {
	0, 1, 1, 2, 3, 3, 4, 5, 5, 6,
	7, 7, 8, 9, 9, 10, 11, 11, 12, 13,
	13, 14, 15, 15, 16, 17, 17, 18, 19, 19,
	20, 21, 21, 22, 23, 23, 24, 25, 25, 26,
	27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
	33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	0, 51, 1, 0,
	7, 7, 7, 20
};

static struct pwm_duty_cycles leds_pwm_duty_cycles = {
	.duty_pcts0 = (int *)&leds_pwm_duty_pcts0,
	.duty_pcts1 = (int *)&leds_pwm_duty_pcts1,
	.duty_pcts2 = (int *)&leds_pwm_duty_pcts2,
	.duty_pcts3 = (int *)&leds_pwm_duty_pcts3,
	.duty_pcts4 = (int *)&leds_pwm_duty_pcts4,
	.duty_pcts5 = (int *)&leds_pwm_duty_pcts5,
	.duty_pcts6 = (int *)&leds_pwm_duty_pcts6,
	.duty_pcts7 = (int *)&leds_pwm_duty_pcts7,
	.duty_pcts8 = (int *)&leds_pwm_duty_pcts8,
	.duty_pcts12 = (int *)&leds_pwm_duty_pcts12,
	.duty_pcts13 = (int *)&leds_pwm_duty_pcts13,
	.duty_pcts14 = (int *)&leds_pwm_duty_pcts14,
	.duty_pcts17 = (int *)&leds_pwm_duty_pcts17,
	.duty_pcts18 = (int *)&leds_pwm_duty_pcts18,
	.duty_pcts19 = (int *)&leds_pwm_duty_pcts19,
	.duty_pcts20 = (int *)&leds_pwm_duty_pcts20,
	.duty_pcts29 = (int *)&leds_pwm_duty_pcts29,
	.duty_pcts30 = (int *)&leds_pwm_duty_pcts30,
	.duty_pcts31 = (int *)&leds_pwm_duty_pcts31,
	.duty_pcts32 = (int *)&leds_pwm_duty_pcts32,
	.duty_pcts101 = (int *)&leds_pwm_duty_pcts101,
	.duty_pcts102 = (int *)&leds_pwm_duty_pcts102,
	.num_duty_pcts = ARRAY_SIZE(leds_pwm_duty_pcts0),
};

struct qpnp_led_data *red_led;
struct qpnp_led_data *green_led;
struct qpnp_led_data *blue_led;
struct qpnp_led_data *kpdbl_lpg1;
struct qpnp_led_data *kpdbl_lpg2;
static int kpdbl_brightness_flag;
static int is_kpdbl_on;

extern void change_led_pattern(int pattern);
void rgb_luts_set(struct qpnp_led_data *led);
#endif

static int
qpnp_led_masked_write(struct qpnp_led_data *led, u16 addr, u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
		addr, &reg, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n", addr, rc);
	}

	reg &= ~mask;
	reg |= val;

	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
		addr, &reg, 1);
	if (rc)
		dev_err(&led->spmi_dev->dev,
			"Unable to write to addr=%x, rc(%d)\n", addr, rc);
	return rc;
}

static void qpnp_dump_regs(struct qpnp_led_data *led, u8 regs[], u8 array_size)
{
	int i;
	u8 val;

	/* pr_debug("===== %s LED register dump start =====\n", led->cdev.name); */
	for (i = 0; i < array_size; i++) {
		spmi_ext_register_readl(led->spmi_dev->ctrl,
					led->spmi_dev->sid,
					led->base + regs[i],
					&val, sizeof(val));
		/*pr_debug("%s: 0x%x = 0x%x\n", led->cdev.name,
					led->base + regs[i], val);*/
	}
	/* pr_debug("===== %s LED register dump end =====\n", led->cdev.name); */
}

static int qpnp_wled_sync(struct qpnp_led_data *led)
{
	int rc;
	u8 val;

	/* sync */
	val = WLED_SYNC_VAL;
	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
		WLED_SYNC_REG(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED set sync reg failed(%d)\n", rc);
		return rc;
	}

	val = WLED_SYNC_RESET_VAL;
	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
		WLED_SYNC_REG(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED reset sync reg failed(%d)\n", rc);
		return rc;
	}
	return 0;
}

static int qpnp_wled_set(struct qpnp_led_data *led)
{
	int rc, duty, level;
	u8 val, i, num_wled_strings, sink_val;

	num_wled_strings = led->wled_cfg->num_strings;

	level = led->cdev.brightness;

	if (level > WLED_MAX_LEVEL)
		level = WLED_MAX_LEVEL;
	if (level == 0) {
		for (i = 0; i < num_wled_strings; i++) {
			rc = qpnp_led_masked_write(led,
				WLED_FULL_SCALE_REG(led->base, i),
				WLED_MAX_CURR_MASK, WLED_NO_CURRENT);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Write max current failure (%d)\n",
					rc);
				return rc;
			}
		}

		rc = qpnp_wled_sync(led);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED sync failed(%d)\n", rc);
			return rc;
		}

		rc = spmi_ext_register_readl(led->spmi_dev->ctrl,
			led->spmi_dev->sid, WLED_CURR_SINK_REG(led->base),
			&sink_val, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED read sink reg failed(%d)\n", rc);
			return rc;
		}

		if (led->wled_cfg->pmic_version == PMIC_VER_8026) {
			val = WLED_DISABLE_ALL_SINKS;
			rc = spmi_ext_register_writel(led->spmi_dev->ctrl,
				led->spmi_dev->sid,
				WLED_CURR_SINK_REG(led->base), &val, 1);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"WLED write sink reg failed(%d)\n", rc);
				return rc;
			}

			usleep(WLED_OVP_DELAY);
		}

		val = WLED_BOOST_OFF;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl,
			led->spmi_dev->sid, WLED_MOD_CTRL_REG(led->base),
			&val, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED write ctrl reg failed(%d)\n", rc);
			return rc;
		}

		for (i = 0; i < num_wled_strings; i++) {
			rc = qpnp_led_masked_write(led,
				WLED_FULL_SCALE_REG(led->base, i),
				WLED_MAX_CURR_MASK, led->max_current);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Write max current failure (%d)\n",
					rc);
				return rc;
			}
		}

		rc = qpnp_wled_sync(led);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED sync failed(%d)\n", rc);
			return rc;
		}

		rc = spmi_ext_register_writel(led->spmi_dev->ctrl,
			led->spmi_dev->sid, WLED_CURR_SINK_REG(led->base),
			&sink_val, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED write sink reg failed(%d)\n", rc);
			return rc;
		}

	} else {
		val = WLED_BOOST_ON;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl,
			led->spmi_dev->sid, WLED_MOD_CTRL_REG(led->base),
			&val, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED write ctrl reg failed(%d)\n", rc);
			return rc;
		}
	}

	duty = (WLED_MAX_DUTY_CYCLE * level) / WLED_MAX_LEVEL;

	/* program brightness control registers */
	for (i = 0; i < num_wled_strings; i++) {
		rc = qpnp_led_masked_write(led,
			WLED_BRIGHTNESS_CNTL_MSB(led->base, i), WLED_MSB_MASK,
			(duty >> WLED_8_BIT_SHFT) & WLED_4_BIT_MASK);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED set brightness MSB failed(%d)\n", rc);
			return rc;
		}
		val = duty & WLED_8_BIT_MASK;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl,
			led->spmi_dev->sid,
			WLED_BRIGHTNESS_CNTL_LSB(led->base, i), &val, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED set brightness LSB failed(%d)\n", rc);
			return rc;
		}
	}

	rc = qpnp_wled_sync(led);
	if (rc) {
		dev_err(&led->spmi_dev->dev, "WLED sync failed(%d)\n", rc);
		return rc;
	}
	return 0;
}

static int qpnp_mpp_set(struct qpnp_led_data *led)
{
	int rc, val;
	int duty_us;

	if (led->cdev.brightness) {
		if (led->cdev.brightness < led->mpp_cfg->min_brightness) {
			dev_warn(&led->spmi_dev->dev,
				"brightness is less than supported..." \
				"set to minimum supported\n");
			led->cdev.brightness = led->mpp_cfg->min_brightness;
		}

		if (led->mpp_cfg->pwm_mode != MANUAL_MODE) {
			if (!led->mpp_cfg->pwm_cfg->blinking) {
				led->mpp_cfg->pwm_cfg->mode =
					led->mpp_cfg->pwm_cfg->default_mode;
				led->mpp_cfg->pwm_mode =
					led->mpp_cfg->pwm_cfg->default_mode;
			}
		}
		if (led->mpp_cfg->pwm_mode == PWM_MODE) {
			pwm_disable(led->mpp_cfg->pwm_cfg->pwm_dev);
			duty_us = (led->mpp_cfg->pwm_cfg->pwm_period_us *
					led->cdev.brightness) / LED_FULL;
			/*config pwm for brightness scaling*/
			rc = pwm_config(led->mpp_cfg->pwm_cfg->pwm_dev,
					duty_us,
					led->mpp_cfg->pwm_cfg->pwm_period_us);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev, "Failed to " \
					"configure pwm for new values\n");
				return rc;
			}
		}

		if (led->mpp_cfg->pwm_mode != MANUAL_MODE)
			pwm_enable(led->mpp_cfg->pwm_cfg->pwm_dev);
		else {
			if (led->cdev.brightness < LED_MPP_CURRENT_MIN)
				led->cdev.brightness = LED_MPP_CURRENT_MIN;

			val = (led->cdev.brightness / LED_MPP_CURRENT_MIN) - 1;

			rc = qpnp_led_masked_write(led,
					LED_MPP_SINK_CTRL(led->base),
					LED_MPP_SINK_MASK, val);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Failed to write sink control reg\n");
				return rc;
			}
		}

		val = (led->mpp_cfg->source_sel & LED_MPP_SRC_MASK) |
			(led->mpp_cfg->mode_ctrl & LED_MPP_MODE_CTRL_MASK);

		rc = qpnp_led_masked_write(led,
			LED_MPP_MODE_CTRL(led->base), LED_MPP_MODE_MASK,
			val);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
					"Failed to write led mode reg\n");
			return rc;
		}

		rc = qpnp_led_masked_write(led,
				LED_MPP_EN_CTRL(led->base), LED_MPP_EN_MASK,
				LED_MPP_EN_ENABLE);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
					"Failed to write led enable " \
					"reg\n");
			return rc;
		}
	} else {
		if (led->mpp_cfg->pwm_mode != MANUAL_MODE) {
			led->mpp_cfg->pwm_cfg->mode =
				led->mpp_cfg->pwm_cfg->default_mode;
			led->mpp_cfg->pwm_mode =
				led->mpp_cfg->pwm_cfg->default_mode;
			pwm_disable(led->mpp_cfg->pwm_cfg->pwm_dev);
		}
		rc = qpnp_led_masked_write(led,
					LED_MPP_MODE_CTRL(led->base),
					LED_MPP_MODE_MASK,
					LED_MPP_MODE_DISABLE);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
					"Failed to write led mode reg\n");
			return rc;
		}

		rc = qpnp_led_masked_write(led,
					LED_MPP_EN_CTRL(led->base),
					LED_MPP_EN_MASK,
					LED_MPP_EN_DISABLE);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
					"Failed to write led enable reg\n");
			return rc;
		}
	}

	if (led->mpp_cfg->pwm_mode != MANUAL_MODE)
		led->mpp_cfg->pwm_cfg->blinking = false;
	qpnp_dump_regs(led, mpp_debug_regs, ARRAY_SIZE(mpp_debug_regs));

	return 0;
}
/*                                                                                          */
#if defined(CONFIG_MACH_MSM8974_G2_KDDI) || defined(CONFIG_MACH_MSM8974_VU3_KR) || defined(CONFIG_MACH_MSM8974_B1_KR) /* QMC original */
static int qpnp_flash_regulator_operate(struct qpnp_led_data *led, bool on)
{
	int rc, i;
	struct qpnp_led_data *led_array;
	bool regulator_on = false;

	led_array = dev_get_drvdata(&led->spmi_dev->dev);
	if (!led_array) {
		dev_err(&led->spmi_dev->dev,
				"Unable to get LED array\n");
		return -EINVAL;
	}

	for (i = 0; i < led->num_leds; i++)
		regulator_on |= led_array[i].flash_cfg->flash_on;

	if (!on)
		goto regulator_turn_off;

	if (!regulator_on && !led->flash_cfg->flash_on) {
		for (i = 0; i < led->num_leds; i++) {
			if (led_array[i].flash_cfg->flash_reg_get) {
				rc = regulator_enable(
					led_array[i].flash_cfg->\
					flash_boost_reg);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
						"Regulator enable failed(%d)\n",
									rc);
					return rc;
				}
				led->flash_cfg->flash_on = true;
			}
			break;
		}
	}

	return 0;

regulator_turn_off:
	if (regulator_on && led->flash_cfg->flash_on) {
		for (i = 0; i < led->num_leds; i++) {
			if (led_array[i].flash_cfg->flash_reg_get) {
				rc = qpnp_led_masked_write(led,
					FLASH_ENABLE_CONTROL(led->base),
					FLASH_ENABLE_MASK,
					FLASH_DISABLE_ALL);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
						"Enable reg write failed(%d)\n",
						rc);
				}

				rc = regulator_disable(led_array[i].flash_cfg->\
							flash_boost_reg);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
						"Regulator disable failed(%d)\n",
									rc);
					return rc;
				}
				led->flash_cfg->flash_on = false;
			}
			break;
		}
	}

	return 0;
}
#else	/* workaround code */
static int qpnp_flash_regulator_operate(struct qpnp_led_data *led, bool on)
{
	u8 buf = 0;
	int rc = 0;
	static bool is_phy_vbus_write;

	if (!led)
		return -EINVAL;

	if (!on || !led->cdev.brightness)
		goto regulator_turn_off;

/* SMBB_USB_SUSP: USB Suspend */
	buf = 0x01;
	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
		0x1347, &buf, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"SMBB_USB_SUSP reg write failed(%d)\n",
			rc);
		return rc;
	}

	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, 0,
			0x13EA, &buf, 1);
	if (rc)
		pr_err("SPMI read failed base:0x13EA rc=%d\n", rc);

	if (buf != 0x2F) {
		is_phy_vbus_write = true;
/* SMBB_USB_SEC_ACCESS */
		buf = 0xA5;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
			0x13D0, &buf, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"SMBB_USB_SEC_ACCESS reg write failed(%d)\n",
				rc);
			return rc;
		}

/* SMBB_USB_COMP_OVR1: overrides USBIN_ULIMIT_OK and USBIN_LLIMIT_OK to 1 and CHG_GONE comparator to 0. */
		buf = 0x2F;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
			0x13EA, &buf, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"SMBB_USB_COMP_OVR1 reg write failed(%d)\n",
				rc);
			return rc;
		}
	}

	return rc;

regulator_turn_off:
	if (is_phy_vbus_write) {
		is_phy_vbus_write = false;
/* SMBB_USB_SEC_ACCESS */
		buf = 0xA5;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
			0x13D0, &buf, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"SMBB_USB_SEC_ACCESS reg write failed(%d)\n",
				rc);
			return rc;
		}

/* SMBB_USB_COMP_OVR1: overrides USBIN_ULIMIT_OK and USBIN_LLIMIT_OK to 1 and CHG_GONE comparator to 0. */
		buf = 0x00;
		rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
			0x13EA, &buf, 1);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"SMBB_USB_COMP_OVR1 reg write failed(%d)\n",
				rc);
			return rc;
		}
	}

/* SMBB_USB_SUSP: USB Suspend */
	buf = 0x00;
	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 0,
		0x1347, &buf, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"SMBB_USB_SUSP reg write failed(%d)\n",
			rc);
		return rc;
	}

	buf = 0x00;
	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, 1,
		0xD346, &buf, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"FLASH_ENABLE reg write failed(%d)\n",
			rc);
		return rc;
	}
	return rc;
}
#endif
/*                                                                                          */

static int qpnp_torch_regulator_operate(struct qpnp_led_data *led, bool on)
{
	int rc;

	if (!on)
		goto regulator_turn_off;

	if (!led->flash_cfg->torch_on) {
		rc = regulator_enable(led->flash_cfg->torch_boost_reg);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Regulator enable failed(%d)\n", rc);
				return rc;
		}
		led->flash_cfg->torch_on = true;
	}
	return 0;

regulator_turn_off:
	if (led->flash_cfg->torch_on) {
		rc = qpnp_led_masked_write(led,	FLASH_ENABLE_CONTROL(led->base),
				FLASH_ENABLE_MODULE_MASK, FLASH_DISABLE_ALL);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Enable reg write failed(%d)\n", rc);
		}

		rc = regulator_disable(led->flash_cfg->torch_boost_reg);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Regulator disable failed(%d)\n", rc);
			return rc;
		}
		led->flash_cfg->torch_on = false;
	}
	return 0;
}

static int qpnp_flash_set(struct qpnp_led_data *led)
{
	int rc, error;
	int val = led->cdev.brightness;

	if (led->flash_cfg->torch_enable)
		led->flash_cfg->current_prgm =
			(val * TORCH_MAX_LEVEL / led->max_current);
	else
		led->flash_cfg->current_prgm =
			(val * FLASH_MAX_LEVEL / led->max_current);

/*                                                                                          */
#if 0
	led->flash_cfg->current_prgm =
		led->flash_cfg->current_prgm >> FLASH_CURRENT_PRGM_SHIFT;
#endif
/*                                                                                          */

	if (!led->flash_cfg->current_prgm)
		led->flash_cfg->current_prgm = FLASH_CURRENT_PRGM_MIN;

	/* Set led current */
	if (val > 0) {
		if (led->flash_cfg->torch_enable) {
			if (led->flash_cfg->peripheral_subtype ==
							FLASH_SUBTYPE_DUAL) {
				rc = qpnp_torch_regulator_operate(led, true);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
					"Torch regulator operate failed(%d)\n",
					rc);
					return rc;
				}
			} else if (led->flash_cfg->peripheral_subtype ==
							FLASH_SUBTYPE_SINGLE) {
				rc = qpnp_flash_regulator_operate(led, true);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
					"Flash regulator operate failed(%d)\n",
					rc);
					goto error_flash_set;
				}
			}

			rc = qpnp_led_masked_write(led,
				FLASH_LED_UNLOCK_SECURE(led->base),
				FLASH_SECURE_MASK, FLASH_UNLOCK_SECURE);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Secure reg write failed(%d)\n", rc);
				goto error_reg_write;
			}

			rc = qpnp_led_masked_write(led,
				FLASH_LED_TORCH(led->base),
				FLASH_TORCH_MASK, FLASH_LED_TORCH_ENABLE);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Torch reg write failed(%d)\n", rc);
				goto error_reg_write;
			}

			/*                                                                                 */
			if (val == 1)
				led->flash_cfg->current_prgm = 0;
			/*                                                                                 */

			rc = qpnp_led_masked_write(led,
				led->flash_cfg->current_addr,
				FLASH_CURRENT_MASK,
				led->flash_cfg->current_prgm);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Current reg write failed(%d)\n", rc);
				goto error_reg_write;
			}

			rc = qpnp_led_masked_write(led,
				led->flash_cfg->second_addr,
				FLASH_CURRENT_MASK,
				led->flash_cfg->current_prgm);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"2nd Current reg write failed(%d)\n",
					rc);
				goto error_reg_write;
			}

			qpnp_led_masked_write(led, FLASH_MAX_CURR(led->base),
				FLASH_CURRENT_MASK,
				TORCH_MAX_LEVEL);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Max current reg write failed(%d)\n",
					rc);
				goto error_reg_write;
			}

			rc = qpnp_led_masked_write(led,
				FLASH_ENABLE_CONTROL(led->base),
				FLASH_ENABLE_MASK,
				led->flash_cfg->enable_module);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Enable reg write failed(%d)\n",
					rc);
				goto error_reg_write;
			}

			rc = qpnp_led_masked_write(led,
				FLASH_LED_STROBE_CTRL(led->base),
				led->flash_cfg->trigger_flash,
				led->flash_cfg->trigger_flash);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"LED %d strobe reg write failed(%d)\n",
					led->id, rc);
				goto error_reg_write;
			}
		} else {
			rc = qpnp_flash_regulator_operate(led, true);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Flash regulator operate failed(%d)\n",
					rc);
				goto error_flash_set;
			}

			/* Set flash safety timer */
			rc = qpnp_led_masked_write(led,
				FLASH_SAFETY_TIMER(led->base),
				FLASH_SAFETY_TIMER_MASK,
				led->flash_cfg->duration);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Safety timer reg write failed(%d)\n",
					rc);
				goto error_flash_set;
			}

			/* Set max current */
			rc = qpnp_led_masked_write(led,
				FLASH_MAX_CURR(led->base), FLASH_CURRENT_MASK,
				FLASH_MAX_LEVEL);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Max current reg write failed(%d)\n",
					rc);
				goto error_flash_set;
			}

			/* Set clamp current */
			rc = qpnp_led_masked_write(led,
				FLASH_CLAMP_CURR(led->base),
				FLASH_CURRENT_MASK,
				led->flash_cfg->clamp_curr);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Clamp current reg write failed(%d)\n",
					rc);
				goto error_flash_set;
			}

			rc = qpnp_led_masked_write(led,
				led->flash_cfg->current_addr,
				FLASH_CURRENT_MASK,
				led->flash_cfg->current_prgm);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Current reg write failed(%d)\n", rc);
				goto error_flash_set;
			}

			rc = qpnp_led_masked_write(led,
				FLASH_ENABLE_CONTROL(led->base),
				led->flash_cfg->enable_module,
				led->flash_cfg->enable_module);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Enable reg write failed(%d)\n", rc);
				goto error_flash_set;
			}

			/*
			 * Add 1ms delay for bharger enter stable state
			 */
			usleep(FLASH_RAMP_UP_DELAY_US);

			if (!led->flash_cfg->strobe_type) {
				rc = qpnp_led_masked_write(led,
					FLASH_LED_STROBE_CTRL(led->base),
					led->flash_cfg->trigger_flash,
					led->flash_cfg->trigger_flash);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
					"LED %d strobe reg write failed(%d)\n",
					led->id, rc);
					goto error_flash_set;
				}
			} else {
				rc = qpnp_led_masked_write(led,
					FLASH_LED_STROBE_CTRL(led->base),
					(led->flash_cfg->trigger_flash |
					FLASH_STROBE_HW),
					(led->flash_cfg->trigger_flash |
					FLASH_STROBE_HW));
				if (rc) {
					dev_err(&led->spmi_dev->dev,
					"LED %d strobe reg write failed(%d)\n",
					led->id, rc);
					goto error_flash_set;
				}
			}
#ifdef CONFIG_MACH_MSM8974_B1_KR
			/* Set VPH_PWR droop debounce time to 64us*/
			rc = qpnp_led_masked_write(led,
				FLASH_LED_DBC_CTRL(led->base),0x3, 0x3);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"VPH_PWR droop debounce time reg write failed(%d)\n",
					rc);
				goto error_flash_set;
			}
			pr_debug("VPH_PWR droop debounce time set to 64us\n");
#endif
		}
	} else {
		rc = qpnp_led_masked_write(led,
			FLASH_LED_STROBE_CTRL(led->base),
			led->flash_cfg->trigger_flash,
			FLASH_DISABLE_ALL);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"LED %d flash write failed(%d)\n", led->id, rc);
			if (led->flash_cfg->torch_enable)
				goto error_torch_set;
			else
				goto error_flash_set;
		}

		if (led->flash_cfg->torch_enable) {
			rc = qpnp_led_masked_write(led,
				FLASH_LED_UNLOCK_SECURE(led->base),
				FLASH_SECURE_MASK, FLASH_UNLOCK_SECURE);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Secure reg write failed(%d)\n", rc);
				goto error_torch_set;
			}

			rc = qpnp_led_masked_write(led,
					FLASH_LED_TORCH(led->base),
					FLASH_TORCH_MASK,
					FLASH_LED_TORCH_DISABLE);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Torch reg write failed(%d)\n", rc);
				goto error_torch_set;
			}

			if (led->flash_cfg->peripheral_subtype ==
							FLASH_SUBTYPE_DUAL) {
				rc = qpnp_torch_regulator_operate(led, false);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
						"Torch regulator operate failed(%d)\n",
						rc);
					return rc;
				}
			} else if (led->flash_cfg->peripheral_subtype ==
							FLASH_SUBTYPE_SINGLE) {
				rc = qpnp_flash_regulator_operate(led, false);
				if (rc) {
					dev_err(&led->spmi_dev->dev,
						"Flash regulator operate failed(%d)\n",
						rc);
					return rc;
				}
			}
		} else {
			/*
			 * Disable module after ramp down complete for stable
			 * behavior
			 */
			usleep(FLASH_RAMP_DN_DELAY_US);

			rc = qpnp_led_masked_write(led,
				FLASH_ENABLE_CONTROL(led->base),
				led->flash_cfg->enable_module &
				~FLASH_ENABLE_MODULE_MASK,
				FLASH_DISABLE_ALL);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Enable reg write failed(%d)\n", rc);
				if (led->flash_cfg->torch_enable)
					goto error_torch_set;
				else
					goto error_flash_set;
			}

			rc = qpnp_flash_regulator_operate(led, false);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Flash regulator operate failed(%d)\n",
					rc);
				return rc;
			}
		}
	}

	qpnp_dump_regs(led, flash_debug_regs, ARRAY_SIZE(flash_debug_regs));

	return 0;

error_reg_write:
	if (led->flash_cfg->peripheral_subtype == FLASH_SUBTYPE_SINGLE)
		goto error_flash_set;

error_torch_set:
	error = qpnp_torch_regulator_operate(led, false);
	if (error) {
		dev_err(&led->spmi_dev->dev,
			"Torch regulator operate failed(%d)\n", rc);
		return error;
	}
	return rc;

error_flash_set:
	error = qpnp_flash_regulator_operate(led, false);
	if (error) {
		dev_err(&led->spmi_dev->dev,
			"Flash regulator operate failed(%d)\n", rc);
		return error;
	}
	return rc;
}

static int qpnp_kpdbl_set(struct qpnp_led_data *led)
{
	int duty_us;
	int rc;

	if (led->cdev.brightness) {
		if (!led->kpdbl_cfg->pwm_cfg->blinking)
			led->kpdbl_cfg->pwm_cfg->mode =
				led->kpdbl_cfg->pwm_cfg->default_mode;
		if (!num_kpbl_leds_on) {
			rc = qpnp_led_masked_write(led, KPDBL_ENABLE(led->base),
					KPDBL_MODULE_EN_MASK, KPDBL_MODULE_EN);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Enable reg write failed(%d)\n", rc);
				return rc;
			}
		}
#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
		led->cdev.brightness = led->cdev.brightness*28/100;
		if (kpdbl_brightness_flag == 0) {
			printk(KERN_INFO "[kpdbl LED] %s: %d\n", led->cdev.name, led->cdev.brightness);
			kpdbl_brightness_flag++;
		}
#endif
#if defined CONFIG_MACH_MSM8974_VU3_KR
		printk(KERN_INFO "qpnp_kpdbl_set :led brightness : %d \n", led->cdev.brightness);
#endif
		if (led->kpdbl_cfg->pwm_cfg->mode == PWM_MODE) {
			duty_us = (led->kpdbl_cfg->pwm_cfg->pwm_period_us *
				led->cdev.brightness) / KPDBL_MAX_LEVEL;
			rc = pwm_config(led->kpdbl_cfg->pwm_cfg->pwm_dev,
					duty_us,
					led->kpdbl_cfg->pwm_cfg->pwm_period_us);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev, "pwm config failed\n");
				return rc;
			}
		}

		rc = pwm_enable(led->kpdbl_cfg->pwm_cfg->pwm_dev);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev, "pwm enable failed\n");
			return rc;
		}
		/*                                                             */
		if (led->kpdbl_cfg->pwm_cfg->mode == LPG_MODE) {
			rc = qpnp_led_masked_write(led, 0xE3C8, 0xFF, 0x03);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Failed to write KPDBL_LUT_RAMP_CONTROL reg(%d)\n", rc);
				return rc;
			}
		}

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
		if(led->kpdbl_cfg->pwm_cfg->leds_on == 0){
			led->kpdbl_cfg->pwm_cfg->leds_on = 1;
			num_kpbl_leds_on++;
		}
#else
		num_kpbl_leds_on++;
#endif

	} else {
		led->kpdbl_cfg->pwm_cfg->mode =
			led->kpdbl_cfg->pwm_cfg->default_mode;

		if (led->kpdbl_cfg->always_on) {
			rc = pwm_config(led->kpdbl_cfg->pwm_cfg->pwm_dev, 0,
					led->kpdbl_cfg->pwm_cfg->pwm_period_us);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
						"pwm config failed\n");
				return rc;
			}

			rc = pwm_enable(led->kpdbl_cfg->pwm_cfg->pwm_dev);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev, "pwm enable failed\n");
				return rc;
			}
		} else
			pwm_disable(led->kpdbl_cfg->pwm_cfg->pwm_dev);

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
		led->kpdbl_cfg->pwm_cfg->leds_on = 0;
#endif
		if (num_kpbl_leds_on > 0)
			num_kpbl_leds_on--;

		if (!num_kpbl_leds_on) {
			rc = qpnp_led_masked_write(led, KPDBL_ENABLE(led->base),
					KPDBL_MODULE_EN_MASK, KPDBL_MODULE_DIS);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
					"Failed to write led enable reg\n");
				return rc;
			}
#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
			printk(KERN_INFO "[kpdbl LED] %s: %d\n", led->cdev.name, led->cdev.brightness);
			kpdbl_brightness_flag = 0;
#endif
		}
	}

	led->kpdbl_cfg->pwm_cfg->blinking = false;

	qpnp_dump_regs(led, kpdbl_debug_regs, ARRAY_SIZE(kpdbl_debug_regs));

	return 0;
}

static int qpnp_rgb_set(struct qpnp_led_data *led)
{
	int duty_us;
	int rc;

	if (led->cdev.brightness) {
		if (!led->rgb_cfg->pwm_cfg->blinking)
			led->rgb_cfg->pwm_cfg->mode =
				led->rgb_cfg->pwm_cfg->default_mode;
		if (led->rgb_cfg->pwm_cfg->mode == PWM_MODE) {
			duty_us = (led->rgb_cfg->pwm_cfg->pwm_period_us *
				led->cdev.brightness) / LED_FULL;
			rc = pwm_config(led->rgb_cfg->pwm_cfg->pwm_dev, duty_us,
					led->rgb_cfg->pwm_cfg->pwm_period_us);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
					"pwm config failed\n");
				return rc;
			}
		}
		rc = qpnp_led_masked_write(led,
			RGB_LED_EN_CTL(led->base),
			led->rgb_cfg->enable, led->rgb_cfg->enable);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Failed to write led enable reg\n");
			return rc;
		}

		rc = pwm_enable(led->rgb_cfg->pwm_cfg->pwm_dev);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev, "pwm enable failed\n");
			return rc;
		}
	} else {
		led->rgb_cfg->pwm_cfg->mode =
			led->rgb_cfg->pwm_cfg->default_mode;
		pwm_disable(led->rgb_cfg->pwm_cfg->pwm_dev);
		rc = qpnp_led_masked_write(led,
			RGB_LED_EN_CTL(led->base),
			led->rgb_cfg->enable, RGB_LED_DISABLE);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Failed to write led enable reg\n");
			return rc;
		}
	}

	led->rgb_cfg->pwm_cfg->blinking = false;
	qpnp_dump_regs(led, rgb_pwm_debug_regs, ARRAY_SIZE(rgb_pwm_debug_regs));

	return 0;
}

static void qpnp_led_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	struct qpnp_led_data *led;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);
	if (value < LED_OFF) {
		dev_err(&led->spmi_dev->dev, "Invalid brightness value\n");
		return;
	}

	if (value > led->cdev.max_brightness)
		value = led->cdev.max_brightness;

	led->cdev.brightness = value;
	schedule_work(&led->work);
}

static void __qpnp_led_work(struct qpnp_led_data *led,
				enum led_brightness value)
{
	int rc;

	mutex_lock(&led->lock);

	switch (led->id) {
	case QPNP_ID_WLED:
		rc = qpnp_wled_set(led);
		if (rc < 0)
			dev_err(&led->spmi_dev->dev,
				"WLED set brightness failed (%d)\n", rc);
		break;
	case QPNP_ID_FLASH1_LED0:
	case QPNP_ID_FLASH1_LED1:
		rc = qpnp_flash_set(led);
		if (rc < 0)
			dev_err(&led->spmi_dev->dev,
				"FLASH set brightness failed (%d)\n", rc);
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
		rgb_luts_set(led);
#else
		rc = qpnp_rgb_set(led);
		if (rc < 0)
			dev_err(&led->spmi_dev->dev,
				"RGB set brightness failed (%d)\n", rc);
#endif
		break;
	case QPNP_ID_LED_MPP:
		rc = qpnp_mpp_set(led);
		if (rc < 0)
			dev_err(&led->spmi_dev->dev,
					"MPP set brightness failed (%d)\n", rc);
		break;
	case QPNP_ID_KPDBL:
		rc = qpnp_kpdbl_set(led);
		if (rc < 0)
			dev_err(&led->spmi_dev->dev,
				"KPDBL set brightness failed (%d)\n", rc);
		break;
	default:
		dev_err(&led->spmi_dev->dev, "Invalid LED(%d)\n", led->id);
		break;
	}
	mutex_unlock(&led->lock);

}

static void qpnp_led_work(struct work_struct *work)
{
	struct qpnp_led_data *led = container_of(work,
					struct qpnp_led_data, work);

	__qpnp_led_work(led, led->cdev.brightness);

	return;
}

static int __devinit qpnp_led_set_max_brightness(struct qpnp_led_data *led)
{
	switch (led->id) {
	case QPNP_ID_WLED:
		led->cdev.max_brightness = WLED_MAX_LEVEL;
		break;
	case QPNP_ID_FLASH1_LED0:
	case QPNP_ID_FLASH1_LED1:
		led->cdev.max_brightness = led->max_current;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		led->cdev.max_brightness = RGB_MAX_LEVEL;
		break;
	case QPNP_ID_LED_MPP:
		if (led->mpp_cfg->pwm_mode == MANUAL_MODE)
			led->cdev.max_brightness = led->max_current;
		else
			led->cdev.max_brightness = MPP_MAX_LEVEL;
		break;
	case QPNP_ID_KPDBL:
		led->cdev.max_brightness = KPDBL_MAX_LEVEL;
		break;
	default:
		dev_err(&led->spmi_dev->dev, "Invalid LED(%d)\n", led->id);
		return -EINVAL;
	}

	return 0;
}

static enum led_brightness qpnp_led_get(struct led_classdev *led_cdev)
{
	struct qpnp_led_data *led;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	return led->cdev.brightness;
}

static void qpnp_led_turn_off_delayed(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_led_data *led
		= container_of(dwork, struct qpnp_led_data, dwork);

	led->cdev.brightness = LED_OFF;
	qpnp_led_set(&led->cdev, led->cdev.brightness);
}

static void qpnp_led_turn_off(struct qpnp_led_data *led)
{
	INIT_DELAYED_WORK(&led->dwork, qpnp_led_turn_off_delayed);
	schedule_delayed_work(&led->dwork,
		msecs_to_jiffies(led->turn_off_delay_ms));
}

static int __devinit qpnp_wled_init(struct qpnp_led_data *led)
{
	int rc, i;
	u8 num_wled_strings;

	num_wled_strings = led->wled_cfg->num_strings;

	/* verify ranges */
	if (led->wled_cfg->ovp_val > WLED_OVP_37V) {
		dev_err(&led->spmi_dev->dev, "Invalid ovp value\n");
		return -EINVAL;
	}

	if (led->wled_cfg->boost_curr_lim > WLED_CURR_LIMIT_1680mA) {
		dev_err(&led->spmi_dev->dev, "Invalid boost current limit\n");
		return -EINVAL;
	}

	if (led->wled_cfg->cp_select > WLED_CP_SELECT_MAX) {
		dev_err(&led->spmi_dev->dev, "Invalid pole capacitance\n");
		return -EINVAL;
	}

	if ((led->max_current > WLED_MAX_CURR)) {
		dev_err(&led->spmi_dev->dev, "Invalid max current\n");
		return -EINVAL;
	}

	if ((led->wled_cfg->ctrl_delay_us % WLED_CTL_DLY_STEP) ||
		(led->wled_cfg->ctrl_delay_us > WLED_CTL_DLY_MAX)) {
		dev_err(&led->spmi_dev->dev, "Invalid control delay\n");
		return -EINVAL;
	}

	/* program over voltage protection threshold */
	rc = qpnp_led_masked_write(led, WLED_OVP_CFG_REG(led->base),
		WLED_OVP_VAL_MASK,
		(led->wled_cfg->ovp_val << WLED_OVP_VAL_BIT_SHFT));
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED OVP reg write failed(%d)\n", rc);
		return rc;
	}

	/* program current boost limit */
	rc = qpnp_led_masked_write(led, WLED_BOOST_LIMIT_REG(led->base),
		WLED_BOOST_LIMIT_MASK, led->wled_cfg->boost_curr_lim);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED boost limit reg write failed(%d)\n", rc);
		return rc;
	}

	/* program output feedback */
	rc = qpnp_led_masked_write(led, WLED_FDBCK_CTRL_REG(led->base),
		WLED_OP_FDBCK_MASK,
		(led->wled_cfg->op_fdbck << WLED_OP_FDBCK_BIT_SHFT));
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED fdbck ctrl reg write failed(%d)\n", rc);
		return rc;
	}

	/* program switch frequency */
	rc = qpnp_led_masked_write(led,
		WLED_SWITCHING_FREQ_REG(led->base),
		WLED_SWITCH_FREQ_MASK, led->wled_cfg->switch_freq);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"WLED switch freq reg write failed(%d)\n", rc);
		return rc;
	}

	/* program current sink */
	if (led->wled_cfg->cs_out_en) {
		rc = qpnp_led_masked_write(led, WLED_CURR_SINK_REG(led->base),
			WLED_CURR_SINK_MASK,
			(((1 << led->wled_cfg->num_strings) - 1)
			<< WLED_CURR_SINK_SHFT));
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED curr sink reg write failed(%d)\n", rc);
			return rc;
		}
	}

	/* program high pole capacitance */
	rc = qpnp_led_masked_write(led, WLED_HIGH_POLE_CAP_REG(led->base),
		WLED_CP_SELECT_MASK, led->wled_cfg->cp_select);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
				"WLED pole cap reg write failed(%d)\n", rc);
		return rc;
	}

	/* program modulator, current mod src and cabc */
	for (i = 0; i < num_wled_strings; i++) {
		rc = qpnp_led_masked_write(led, WLED_MOD_EN_REG(led->base, i),
			WLED_NO_MASK, WLED_EN_MASK);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED mod enable reg write failed(%d)\n", rc);
			return rc;
		}

		if (led->wled_cfg->dig_mod_gen_en) {
			rc = qpnp_led_masked_write(led,
				WLED_MOD_SRC_SEL_REG(led->base, i),
				WLED_NO_MASK, WLED_USE_EXT_GEN_MOD_SRC);
			if (rc) {
				dev_err(&led->spmi_dev->dev,
				"WLED dig mod en reg write failed(%d)\n", rc);
			}
		}

		rc = qpnp_led_masked_write(led,
			WLED_FULL_SCALE_REG(led->base, i), WLED_MAX_CURR_MASK,
			led->max_current);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"WLED max current reg write failed(%d)\n", rc);
			return rc;
		}

	}

	/* dump wled registers */
	qpnp_dump_regs(led, wled_debug_regs, ARRAY_SIZE(wled_debug_regs));

	return 0;
}

static ssize_t led_mode_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	unsigned long state;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;

	ret = kstrtoul(buf, 10, &state);
	if (ret)
		return ret;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	/* '1' to enable torch mode; '0' to switch to flash mode */
	if (state == 1)
		led->flash_cfg->torch_enable = true;
	else
		led->flash_cfg->torch_enable = false;

	return count;
}

static ssize_t led_strobe_type_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	unsigned long state;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;

	ret = kstrtoul(buf, 10, &state);
	if (ret)
		return ret;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	/* '0' for sw strobe; '1' for hw strobe */
	if (state == 1)
		led->flash_cfg->strobe_type = 1;
	else
		led->flash_cfg->strobe_type = 0;

	return count;
}

static int qpnp_pwm_init(struct pwm_config_data *pwm_cfg,
					struct spmi_device *spmi_dev,
					const char *name)
{
	int rc, start_idx, idx_len;

	if (pwm_cfg->pwm_channel != -1) {
		pwm_cfg->pwm_dev =
			pwm_request(pwm_cfg->pwm_channel, name);

		if (IS_ERR_OR_NULL(pwm_cfg->pwm_dev)) {
			dev_err(&spmi_dev->dev,
				"could not acquire PWM Channel %d, " \
				"error %ld\n",
				pwm_cfg->pwm_channel,
				PTR_ERR(pwm_cfg->pwm_dev));
			pwm_cfg->pwm_dev = NULL;
			return -ENODEV;
		}

		if (pwm_cfg->mode == LPG_MODE) {
			start_idx =
			pwm_cfg->duty_cycles->start_idx;
			idx_len =
			pwm_cfg->duty_cycles->num_duty_pcts;

			if (idx_len >= PWM_LUT_MAX_SIZE &&
					start_idx) {
				dev_err(&spmi_dev->dev,
					"Wrong LUT size or index\n");
				return -EINVAL;
			}
			if ((start_idx + idx_len) >
					PWM_LUT_MAX_SIZE) {
				dev_err(&spmi_dev->dev,
					"Exceed LUT limit\n");
				return -EINVAL;
			}
			rc = pwm_lut_config(pwm_cfg->pwm_dev,
				PM_PWM_PERIOD_MIN, /* ignored by hardware */
				pwm_cfg->duty_cycles->duty_pcts,
				pwm_cfg->lut_params);
			if (rc < 0) {
				dev_err(&spmi_dev->dev, "Failed to " \
					"configure pwm LUT\n");
				return rc;
			}
		}
	} else {
		dev_err(&spmi_dev->dev,
			"Invalid PWM channel\n");
		return -EINVAL;
	}

	return 0;
}

static ssize_t pwm_us_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 pwm_us;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_pwm_us;
	struct pwm_config_data *pwm_cfg;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	ret = kstrtou32(buf, 10, &pwm_us);
	if (ret)
		return ret;

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for pwm_us\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_pwm_us = pwm_cfg->pwm_period_us;

	pwm_cfg->pwm_period_us = pwm_us;
	pwm_free(pwm_cfg->pwm_dev);
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->pwm_period_us = previous_pwm_us;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new pwm_us value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t pause_lo_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 pause_lo;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_pause_lo;
	struct pwm_config_data *pwm_cfg;

	ret = kstrtou32(buf, 10, &pause_lo);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for pause lo\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_pause_lo = pwm_cfg->lut_params.lut_pause_lo;

	pwm_free(pwm_cfg->pwm_dev);
	pwm_cfg->lut_params.lut_pause_lo = pause_lo;
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->lut_params.lut_pause_lo = previous_pause_lo;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new pause lo value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t pause_hi_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 pause_hi;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_pause_hi;
	struct pwm_config_data *pwm_cfg;

	ret = kstrtou32(buf, 10, &pause_hi);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for pause hi\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_pause_hi = pwm_cfg->lut_params.lut_pause_hi;

	pwm_free(pwm_cfg->pwm_dev);
	pwm_cfg->lut_params.lut_pause_hi = pause_hi;
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->lut_params.lut_pause_hi = previous_pause_hi;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new pause hi value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t start_idx_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 start_idx;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_start_idx;
	struct pwm_config_data *pwm_cfg;

	ret = kstrtou32(buf, 10, &start_idx);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for start idx\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_start_idx = pwm_cfg->duty_cycles->start_idx;
	pwm_cfg->duty_cycles->start_idx = start_idx;
	pwm_cfg->lut_params.start_idx = pwm_cfg->duty_cycles->start_idx;
	pwm_free(pwm_cfg->pwm_dev);
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->duty_cycles->start_idx = previous_start_idx;
		pwm_cfg->lut_params.start_idx = pwm_cfg->duty_cycles->start_idx;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new start idx value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t ramp_step_ms_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 ramp_step_ms;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_ramp_step_ms;
	struct pwm_config_data *pwm_cfg;

	ret = kstrtou32(buf, 10, &ramp_step_ms);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for ramp step\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_ramp_step_ms = pwm_cfg->lut_params.ramp_step_ms;

	pwm_free(pwm_cfg->pwm_dev);
	pwm_cfg->lut_params.ramp_step_ms = ramp_step_ms;
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->lut_params.ramp_step_ms = previous_ramp_step_ms;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new ramp step value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t lut_flags_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	u32 lut_flags;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 previous_lut_flags;
	struct pwm_config_data *pwm_cfg;

	ret = kstrtou32(buf, 10, &lut_flags);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for lut flags\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	previous_lut_flags = pwm_cfg->lut_params.flags;

	pwm_free(pwm_cfg->pwm_dev);
	pwm_cfg->lut_params.flags = lut_flags;
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret) {
		pwm_cfg->lut_params.flags = previous_lut_flags;
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm with new lut flags value\n");
		return ret;
	}
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;
}

static ssize_t duty_pcts_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	int num_duty_pcts = 0;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	char *buffer;
	ssize_t ret;
	int i = 0;
	int max_duty_pcts;
	struct pwm_config_data *pwm_cfg;
	u32 previous_num_duty_pcts;
	int value;
	int *previous_duty_pcts;

	led = container_of(led_cdev, struct qpnp_led_data, cdev);

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		pwm_cfg = led->mpp_cfg->pwm_cfg;
		max_duty_pcts = PWM_LUT_MAX_SIZE;
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		pwm_cfg = led->rgb_cfg->pwm_cfg;
		max_duty_pcts = PWM_LUT_MAX_SIZE;
		break;
	default:
		dev_err(&led->spmi_dev->dev,
			"Invalid LED id type for duty pcts\n");
		return -EINVAL;
	}

	if (pwm_cfg->mode == LPG_MODE)
		pwm_cfg->blinking = true;

	buffer = (char *)buf;

	for (i = 0; i < max_duty_pcts; i++) {
		if (buffer == NULL)
			break;
		ret = sscanf((const char *)buffer, "%u,%s", &value, buffer);
		pwm_cfg->old_duty_pcts[i] = value;
		num_duty_pcts++;
		if (ret <= 1)
			break;
	}

	if (num_duty_pcts >= max_duty_pcts) {
		dev_err(&led->spmi_dev->dev,
			"Number of duty pcts given exceeds max (%d)\n",
			max_duty_pcts);
		return -EINVAL;
	}

	previous_num_duty_pcts = pwm_cfg->duty_cycles->num_duty_pcts;
	previous_duty_pcts = pwm_cfg->duty_cycles->duty_pcts;

	pwm_cfg->duty_cycles->num_duty_pcts = num_duty_pcts;
	pwm_cfg->duty_cycles->duty_pcts = pwm_cfg->old_duty_pcts;
	pwm_cfg->old_duty_pcts = previous_duty_pcts;
	pwm_cfg->lut_params.idx_len = pwm_cfg->duty_cycles->num_duty_pcts;

	pwm_free(pwm_cfg->pwm_dev);
	ret = qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	if (ret)
		goto restore;

	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return count;

restore:
	dev_err(&led->spmi_dev->dev,
		"Failed to initialize pwm with new duty pcts value\n");
	pwm_cfg->duty_cycles->num_duty_pcts = previous_num_duty_pcts;
	pwm_cfg->old_duty_pcts = pwm_cfg->duty_cycles->duty_pcts;
	pwm_cfg->duty_cycles->duty_pcts = previous_duty_pcts;
	pwm_cfg->lut_params.idx_len = pwm_cfg->duty_cycles->num_duty_pcts;
	pwm_free(pwm_cfg->pwm_dev);
	qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
	qpnp_led_set(&led->cdev, led->cdev.brightness);
	return ret;
}

static void led_blink(struct qpnp_led_data *led,
			struct pwm_config_data *pwm_cfg)
{
	if (pwm_cfg->use_blink) {
		if (led->cdev.brightness) {
			pwm_cfg->blinking = true;
			if (led->id == QPNP_ID_LED_MPP)
				led->mpp_cfg->pwm_mode = LPG_MODE;
			pwm_cfg->mode = LPG_MODE;
		} else {
			pwm_cfg->blinking = false;
			pwm_cfg->mode = pwm_cfg->default_mode;
			if (led->id == QPNP_ID_LED_MPP)
				led->mpp_cfg->pwm_mode = pwm_cfg->default_mode;
		}
		pwm_free(pwm_cfg->pwm_dev);
		qpnp_pwm_init(pwm_cfg, led->spmi_dev, led->cdev.name);
		qpnp_led_set(&led->cdev, led->cdev.brightness);
	}
}

static ssize_t blink_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct qpnp_led_data *led;
	unsigned long blinking;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;

	ret = kstrtoul(buf, 10, &blinking);
	if (ret)
		return ret;
	led = container_of(led_cdev, struct qpnp_led_data, cdev);
	led->cdev.brightness = blinking ? led->cdev.max_brightness : 0;

	switch (led->id) {
	case QPNP_ID_LED_MPP:
		led_blink(led, led->mpp_cfg->pwm_cfg);
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		led_blink(led, led->rgb_cfg->pwm_cfg);
		break;
	default:
		dev_err(&led->spmi_dev->dev, "Invalid LED id type for blink\n");
		return -EINVAL;
	}
	return count;
}

static DEVICE_ATTR(led_mode, 0664, NULL, led_mode_store);
static DEVICE_ATTR(strobe, 0664, NULL, led_strobe_type_store);
static DEVICE_ATTR(pwm_us, 0664, NULL, pwm_us_store);
static DEVICE_ATTR(pause_lo, 0664, NULL, pause_lo_store);
static DEVICE_ATTR(pause_hi, 0664, NULL, pause_hi_store);
static DEVICE_ATTR(start_idx, 0664, NULL, start_idx_store);
static DEVICE_ATTR(ramp_step_ms, 0664, NULL, ramp_step_ms_store);
static DEVICE_ATTR(lut_flags, 0664, NULL, lut_flags_store);
static DEVICE_ATTR(duty_pcts, 0664, NULL, duty_pcts_store);
static DEVICE_ATTR(blink, 0664, NULL, blink_store);

static struct attribute *led_attrs[] = {
	&dev_attr_led_mode.attr,
	&dev_attr_strobe.attr,
	NULL
};

static const struct attribute_group led_attr_group = {
	.attrs = led_attrs,
};

static struct attribute *pwm_attrs[] = {
	&dev_attr_pwm_us.attr,
	NULL
};

static struct attribute *lpg_attrs[] = {
	&dev_attr_pause_lo.attr,
	&dev_attr_pause_hi.attr,
	&dev_attr_start_idx.attr,
	&dev_attr_ramp_step_ms.attr,
	&dev_attr_lut_flags.attr,
	&dev_attr_duty_pcts.attr,
	NULL
};

static struct attribute *blink_attrs[] = {
	&dev_attr_blink.attr,
	NULL
};

static const struct attribute_group pwm_attr_group = {
	.attrs = pwm_attrs,
};

static const struct attribute_group lpg_attr_group = {
	.attrs = lpg_attrs,
};

static const struct attribute_group blink_attr_group = {
	.attrs = blink_attrs,
};

static int __devinit qpnp_flash_init(struct qpnp_led_data *led)
{
	int rc;

	led->flash_cfg->flash_on = false;
	led->flash_cfg->torch_on = false;

	rc = qpnp_led_masked_write(led,
		FLASH_LED_STROBE_CTRL(led->base),
		FLASH_STROBE_MASK, FLASH_DISABLE_ALL);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"LED %d flash write failed(%d)\n", led->id, rc);
		return rc;
	}

	/* Disable flash LED module */
	rc = qpnp_led_masked_write(led, FLASH_ENABLE_CONTROL(led->base),
		FLASH_ENABLE_MASK, FLASH_DISABLE_ALL);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Enable reg write failed(%d)\n", rc);
		return rc;
	}

	if (led->flash_cfg->torch_enable)
		return 0;

	/* Set headroom */
	rc = qpnp_led_masked_write(led, FLASH_HEADROOM(led->base),
		FLASH_HEADROOM_MASK, led->flash_cfg->headroom);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Headroom reg write failed(%d)\n", rc);
		return rc;
	}

	/* Set startup delay */
	rc = qpnp_led_masked_write(led,
		FLASH_STARTUP_DELAY(led->base), FLASH_STARTUP_DLY_MASK,
		led->flash_cfg->startup_dly);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Startup delay reg write failed(%d)\n", rc);
		return rc;
	}

	/* Set timer control - safety or watchdog */
	if (led->flash_cfg->safety_timer) {
		rc = qpnp_led_masked_write(led,
			FLASH_LED_TMR_CTRL(led->base),
			FLASH_TMR_MASK, FLASH_TMR_SAFETY);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"LED timer ctrl reg write failed(%d)\n",
				rc);
			return rc;
		}
	}

	/* Set Vreg force */
	rc = qpnp_led_masked_write(led,	FLASH_VREG_OK_FORCE(led->base),
		FLASH_VREG_MASK, FLASH_HW_VREG_OK);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Vreg OK reg write failed(%d)\n", rc);
		return rc;
	}

	/* Set self fault check */
	rc = qpnp_led_masked_write(led, FLASH_FAULT_DETECT(led->base),
		FLASH_FAULT_DETECT_MASK, FLASH_SELFCHECK_ENABLE);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Fault detect reg write failed(%d)\n", rc);
		return rc;
	}

	/* Set mask enable */
	rc = qpnp_led_masked_write(led, FLASH_MASK_ENABLE(led->base),
		FLASH_MASK_REG_MASK, FLASH_MASK_1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Mask enable reg write failed(%d)\n", rc);
		return rc;
	}

	/* Set current ramp */
	rc = qpnp_led_masked_write(led, FLASH_CURRENT_RAMP(led->base),
		FLASH_CURRENT_RAMP_MASK, FLASH_RAMP_STEP_27US);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Current ramp reg write failed(%d)\n", rc);
		return rc;
	}

/*                                                                                         */
	/* Set FLASH_VPH_PWR_DROOP
	 * 7  	: 	0 = do not us this feature, 1 = enable this feature
	 * 6:4	:	000 = 2.5V       011 = 2.8V           110 = 3.1V
	 *     		001 = 2.6V       100 = 2.9V           111 = 3.2V
	 *     		010 = 2.7V       101 = 3.0V
	 * 1:0	:	00 = 0us, 01 = 10us, 10 = 32us, 11 = 64us
	 */
	rc = qpnp_led_masked_write(led, 0xD35A, 0xFF, 0xC2);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"FLASH_VPH_PWR_DROOP reg write failed(%d)\n", rc);
		return rc;
	}
/*                                                                                         */

	led->flash_cfg->strobe_type = 0;

	/* dump flash registers */
	qpnp_dump_regs(led, flash_debug_regs, ARRAY_SIZE(flash_debug_regs));

	return 0;
}

static int __devinit qpnp_kpdbl_init(struct qpnp_led_data *led)
{
	int rc;
	u8 val;

	/* workaround for GPLED pwm mode */
	rc = qpnp_led_masked_write(led, 0xE2B1, 0xFF, 0x80);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
		"workaround for GPLED pwm mode111(%d)\n", rc);
		return rc;
	}

	/* workaround for GPLED pwm mode */
	rc = qpnp_led_masked_write(led, 0xE2B4, 0xFF, 0x04);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"workaround for GPLED pwm mode222(%d)\n", rc);
		return rc;
	}

	/* select row source - vbst or vph */
	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
				KPDBL_ROW_SRC_SEL(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n",
			KPDBL_ROW_SRC_SEL(led->base), rc);
		return rc;
	}

	if (led->kpdbl_cfg->row_src_vbst)
		val |= 1 << led->kpdbl_cfg->row_id;
	else
		val &= ~(1 << led->kpdbl_cfg->row_id);

	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
				KPDBL_ROW_SRC_SEL(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n",
			KPDBL_ROW_SRC_SEL(led->base), rc);
		return rc;
	}

	/* row source enable */
	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
				KPDBL_ROW_SRC(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n",
			KPDBL_ROW_SRC(led->base), rc);
		return rc;
	}

	if (led->kpdbl_cfg->row_src_en)
		val |= KPDBL_ROW_SCAN_EN_MASK | (1 << led->kpdbl_cfg->row_id);
	else
		val &= ~(1 << led->kpdbl_cfg->row_id);

	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
		KPDBL_ROW_SRC(led->base), &val, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to write to addr=%x, rc(%d)\n",
			KPDBL_ROW_SRC(led->base), rc);
		return rc;
	}

#ifndef CONFIG_LEDS_PM8941_EMOTIONAL
	/* enable module */
	rc = qpnp_led_masked_write(led, KPDBL_ENABLE(led->base),
		KPDBL_MODULE_EN_MASK, KPDBL_MODULE_EN);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Enable module write failed(%d)\n", rc);
		return rc;
	}
#endif
	rc = qpnp_pwm_init(led->kpdbl_cfg->pwm_cfg, led->spmi_dev,
				led->cdev.name);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm\n");
		return rc;
	}

	/* dump kpdbl registers */
	qpnp_dump_regs(led, kpdbl_debug_regs, ARRAY_SIZE(kpdbl_debug_regs));

	return 0;
}

static int __devinit qpnp_rgb_init(struct qpnp_led_data *led)
{
	int rc;

	rc = qpnp_led_masked_write(led, RGB_LED_SRC_SEL(led->base),
		RGB_LED_SRC_MASK, RGB_LED_SOURCE_VPH_PWR);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Failed to write led source select register\n");
		return rc;
	}

	rc = qpnp_pwm_init(led->rgb_cfg->pwm_cfg, led->spmi_dev,
				led->cdev.name);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Failed to initialize pwm\n");
		return rc;
	}
	/* Initialize led for use in auto trickle charging mode */
	rc = qpnp_led_masked_write(led, RGB_LED_ATC_CTL(led->base),
		led->rgb_cfg->enable, led->rgb_cfg->enable);

	return 0;
}

static int __devinit qpnp_mpp_init(struct qpnp_led_data *led)
{
	int rc, val;


	if (led->max_current < LED_MPP_CURRENT_MIN ||
		led->max_current > LED_MPP_CURRENT_MAX) {
		dev_err(&led->spmi_dev->dev,
			"max current for mpp is not valid\n");
		return -EINVAL;
	}

	val = (led->mpp_cfg->current_setting / LED_MPP_CURRENT_PER_SETTING) - 1;

	if (val < 0)
		val = 0;

	rc = qpnp_led_masked_write(led, LED_MPP_VIN_CTRL(led->base),
		LED_MPP_VIN_MASK, led->mpp_cfg->vin_ctrl);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Failed to write led vin control reg\n");
		return rc;
	}

	rc = qpnp_led_masked_write(led, LED_MPP_SINK_CTRL(led->base),
		LED_MPP_SINK_MASK, val);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Failed to write sink control reg\n");
		return rc;
	}

	if (led->mpp_cfg->pwm_mode != MANUAL_MODE) {
		rc = qpnp_pwm_init(led->mpp_cfg->pwm_cfg, led->spmi_dev,
					led->cdev.name);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Failed to initialize pwm\n");
			return rc;
		}
	}

	return 0;
}

static int __devinit qpnp_led_initialize(struct qpnp_led_data *led)
{
	int rc = 0;

	switch (led->id) {
	case QPNP_ID_WLED:
		rc = qpnp_wled_init(led);
		if (rc)
			dev_err(&led->spmi_dev->dev,
				"WLED initialize failed(%d)\n", rc);
		break;
	case QPNP_ID_FLASH1_LED0:
	case QPNP_ID_FLASH1_LED1:
		rc = qpnp_flash_init(led);
		if (rc)
			dev_err(&led->spmi_dev->dev,
				"FLASH initialize failed(%d)\n", rc);
		break;
	case QPNP_ID_RGB_RED:
	case QPNP_ID_RGB_GREEN:
	case QPNP_ID_RGB_BLUE:
		rc = qpnp_rgb_init(led);
		if (rc)
			dev_err(&led->spmi_dev->dev,
				"RGB initialize failed(%d)\n", rc);
		break;
	case QPNP_ID_LED_MPP:
		rc = qpnp_mpp_init(led);
		if (rc)
			dev_err(&led->spmi_dev->dev,
				"MPP initialize failed(%d)\n", rc);
		break;
	case QPNP_ID_KPDBL:
		rc = qpnp_kpdbl_init(led);
		if (rc)
			dev_err(&led->spmi_dev->dev,
				"KPDBL initialize failed(%d)\n", rc);
		break;
	default:
		dev_err(&led->spmi_dev->dev, "Invalid LED(%d)\n", led->id);
		return -EINVAL;
	}

	return rc;
}

static int __devinit qpnp_get_common_configs(struct qpnp_led_data *led,
				struct device_node *node)
{
	int rc;
	u32 val;
	const char *temp_string;

	led->cdev.default_trigger = LED_TRIGGER_DEFAULT;
	rc = of_property_read_string(node, "linux,default-trigger",
		&temp_string);
	if (!rc)
		led->cdev.default_trigger = temp_string;
	else if (rc != -EINVAL)
		return rc;

	led->default_on = false;
	rc = of_property_read_string(node, "qcom,default-state",
		&temp_string);
	if (!rc) {
		if (strncmp(temp_string, "on", sizeof("on")) == 0)
			led->default_on = true;
	} else if (rc != -EINVAL)
		return rc;

	led->turn_off_delay_ms = 0;
	rc = of_property_read_u32(node, "qcom,turn-off-delay-ms", &val);
	if (!rc)
		led->turn_off_delay_ms = val;
	else if (rc != -EINVAL)
		return rc;

	return 0;
}

/*
 * Handlers for alternative sources of platform_data
 */
static int __devinit qpnp_get_config_wled(struct qpnp_led_data *led,
				struct device_node *node)
{
	u32 val;
	int rc;

	led->wled_cfg = devm_kzalloc(&led->spmi_dev->dev,
				sizeof(struct wled_config_data), GFP_KERNEL);
	if (!led->wled_cfg) {
		dev_err(&led->spmi_dev->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
		PMIC_VERSION_REG, &led->wled_cfg->pmic_version, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read pmic ver, rc(%d)\n", rc);
	}

	led->wled_cfg->num_strings = WLED_DEFAULT_STRINGS;
	rc = of_property_read_u32(node, "qcom,num-strings", &val);
	if (!rc)
		led->wled_cfg->num_strings = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->ovp_val = WLED_DEFAULT_OVP_VAL;
	rc = of_property_read_u32(node, "qcom,ovp-val", &val);
	if (!rc)
		led->wled_cfg->ovp_val = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->boost_curr_lim = WLED_BOOST_LIM_DEFAULT;
	rc = of_property_read_u32(node, "qcom,boost-curr-lim", &val);
	if (!rc)
		led->wled_cfg->boost_curr_lim = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->cp_select = WLED_CP_SEL_DEFAULT;
	rc = of_property_read_u32(node, "qcom,cp-sel", &val);
	if (!rc)
		led->wled_cfg->cp_select = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->ctrl_delay_us = WLED_CTRL_DLY_DEFAULT;
	rc = of_property_read_u32(node, "qcom,ctrl-delay-us", &val);
	if (!rc)
		led->wled_cfg->ctrl_delay_us = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->op_fdbck = WLED_OP_FDBCK_DEFAULT;
	rc = of_property_read_u32(node, "qcom,op-fdbck", &val);
	if (!rc)
		led->wled_cfg->op_fdbck = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->switch_freq = WLED_SWITCH_FREQ_DEFAULT;
	rc = of_property_read_u32(node, "qcom,switch-freq", &val);
	if (!rc)
		led->wled_cfg->switch_freq = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->wled_cfg->dig_mod_gen_en =
		of_property_read_bool(node, "qcom,dig-mod-gen-en");

	led->wled_cfg->cs_out_en =
		of_property_read_bool(node, "qcom,cs-out-en");

	return 0;
}

static int __devinit qpnp_get_config_flash(struct qpnp_led_data *led,
				struct device_node *node, bool *reg_set)
{
	int rc;
	u32 val;

	led->flash_cfg = devm_kzalloc(&led->spmi_dev->dev,
				sizeof(struct flash_config_data), GFP_KERNEL);
	if (!led->flash_cfg) {
		dev_err(&led->spmi_dev->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
			FLASH_PERIPHERAL_SUBTYPE(led->base),
			&led->flash_cfg->peripheral_subtype, 1);
	if (rc) {
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n",
			FLASH_PERIPHERAL_SUBTYPE(led->base), rc);
	}

	led->flash_cfg->torch_enable =
		of_property_read_bool(node, "qcom,torch-enable");

	if (led->id == QPNP_ID_FLASH1_LED0) {
		led->flash_cfg->enable_module = FLASH_ENABLE_LED_0;
		led->flash_cfg->current_addr = FLASH_LED_0_CURR(led->base);
		led->flash_cfg->trigger_flash = FLASH_LED_0_OUTPUT;
		if (!*reg_set) {
			led->flash_cfg->flash_boost_reg =
				regulator_get(&led->spmi_dev->dev,
							"flash-boost");
			if (IS_ERR(led->flash_cfg->flash_boost_reg)) {
				rc = PTR_ERR(led->flash_cfg->flash_boost_reg);
				dev_err(&led->spmi_dev->dev,
					"Regulator get failed(%d)\n", rc);
				goto error_get_flash_reg;
			}
			led->flash_cfg->flash_reg_get = true;
			*reg_set = true;
		} else
			led->flash_cfg->flash_reg_get = false;

		if (led->flash_cfg->torch_enable) {
			led->flash_cfg->second_addr =
						FLASH_LED_1_CURR(led->base);
		}
	} else if (led->id == QPNP_ID_FLASH1_LED1) {
		led->flash_cfg->enable_module = FLASH_ENABLE_LED_1;
		led->flash_cfg->current_addr = FLASH_LED_1_CURR(led->base);
		led->flash_cfg->trigger_flash = FLASH_LED_1_OUTPUT;
		if (!*reg_set) {
			led->flash_cfg->flash_boost_reg =
					regulator_get(&led->spmi_dev->dev,
								"flash-boost");
			if (IS_ERR(led->flash_cfg->flash_boost_reg)) {
				rc = PTR_ERR(led->flash_cfg->flash_boost_reg);
				dev_err(&led->spmi_dev->dev,
					"Regulator get failed(%d)\n", rc);
				goto error_get_flash_reg;
			}
			led->flash_cfg->flash_reg_get = true;
			*reg_set = true;
		} else
			led->flash_cfg->flash_reg_get = false;

		if (led->flash_cfg->torch_enable) {
			led->flash_cfg->second_addr =
						FLASH_LED_0_CURR(led->base);
		}
	} else {
		dev_err(&led->spmi_dev->dev, "Unknown flash LED name given\n");
		return -EINVAL;
	}

	if (led->flash_cfg->torch_enable) {
		if (of_find_property(of_get_parent(node), "torch-boost-supply",
									NULL)) {
			led->flash_cfg->torch_boost_reg =
				regulator_get(&led->spmi_dev->dev,
								"torch-boost");
			if (IS_ERR(led->flash_cfg->torch_boost_reg)) {
				rc = PTR_ERR(led->flash_cfg->torch_boost_reg);
				dev_err(&led->spmi_dev->dev,
					"Torch regulator get failed(%d)\n", rc);
				goto error_get_torch_reg;
			}
			led->flash_cfg->enable_module = FLASH_ENABLE_MODULE;
		} else
			led->flash_cfg->enable_module = FLASH_ENABLE_ALL;
		led->flash_cfg->trigger_flash = FLASH_STROBE_SW;
	}

	rc = of_property_read_u32(node, "qcom,current", &val);
	if (!rc) {
		if (led->flash_cfg->torch_enable) {
			led->flash_cfg->current_prgm = (val *
				TORCH_MAX_LEVEL / led->max_current);
			return 0;
		} else
			led->flash_cfg->current_prgm = (val *
				FLASH_MAX_LEVEL / led->max_current);
	} else
		if (led->flash_cfg->torch_enable)
			goto error_get_torch_reg;
		else
			goto error_get_flash_reg;

	rc = of_property_read_u32(node, "qcom,headroom", &val);
	if (!rc)
		led->flash_cfg->headroom = (u8) val;
	else if (rc == -EINVAL)
		led->flash_cfg->headroom = HEADROOM_500mV;
	else
		goto error_get_flash_reg;

	rc = of_property_read_u32(node, "qcom,duration", &val);
	if (!rc)
		led->flash_cfg->duration = (u8)((val - 10) / 10);
	else if (rc == -EINVAL)
		led->flash_cfg->duration = FLASH_DURATION_200ms;
	else
		goto error_get_flash_reg;

	rc = of_property_read_u32(node, "qcom,clamp-curr", &val);
	if (!rc)
		led->flash_cfg->clamp_curr = (val *
				FLASH_MAX_LEVEL / led->max_current);
	else if (rc == -EINVAL)
		led->flash_cfg->clamp_curr = FLASH_CLAMP_200mA;
	else
		goto error_get_flash_reg;

	rc = of_property_read_u32(node, "qcom,startup-dly", &val);
	if (!rc)
		led->flash_cfg->startup_dly = (u8) val;
	else if (rc == -EINVAL)
		led->flash_cfg->startup_dly = DELAY_128us;
	else
		goto error_get_flash_reg;

	led->flash_cfg->safety_timer =
		of_property_read_bool(node, "qcom,safety-timer");

	return 0;

error_get_torch_reg:
	regulator_put(led->flash_cfg->torch_boost_reg);

error_get_flash_reg:
	regulator_put(led->flash_cfg->flash_boost_reg);
	return rc;

}

static int __devinit qpnp_get_config_pwm(struct pwm_config_data *pwm_cfg,
				struct spmi_device *spmi_dev,
				struct device_node *node)
{
	struct property *prop;
	int rc, i;
	u32 val;
	u8 *temp_cfg;

	rc = of_property_read_u32(node, "qcom,pwm-channel", &val);
	if (!rc)
		pwm_cfg->pwm_channel = val;
	else
		return rc;

/*                                          */
#if 0
	if (pwm_cfg->mode == PWM_MODE) {
#endif
		rc = of_property_read_u32(node, "qcom,pwm-us", &val);
		if (!rc)
			pwm_cfg->pwm_period_us = val;
		else
			return rc;
#if 0
	}
#endif

	pwm_cfg->use_blink =
		of_property_read_bool(node, "qcom,use-blink");

	if (pwm_cfg->mode == LPG_MODE || pwm_cfg->use_blink) {
		pwm_cfg->duty_cycles =
			devm_kzalloc(&spmi_dev->dev,
			sizeof(struct pwm_duty_cycles), GFP_KERNEL);
		if (!pwm_cfg->duty_cycles) {
			dev_err(&spmi_dev->dev,
				"Unable to allocate memory\n");
			rc = -ENOMEM;
			goto bad_lpg_params;
		}

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
		pwm_cfg->duty_cycles->num_duty_pcts =
			leds_pwm_duty_cycles.num_duty_pcts;
		pwm_cfg->duty_cycles->duty_pcts0 =
			leds_pwm_duty_cycles.duty_pcts0;
		pwm_cfg->duty_cycles->duty_pcts1 =
			leds_pwm_duty_cycles.duty_pcts1;
		pwm_cfg->duty_cycles->duty_pcts2 =
			leds_pwm_duty_cycles.duty_pcts2;
		pwm_cfg->duty_cycles->duty_pcts3 =
			leds_pwm_duty_cycles.duty_pcts3;
		pwm_cfg->duty_cycles->duty_pcts4 =
			leds_pwm_duty_cycles.duty_pcts4;
		pwm_cfg->duty_cycles->duty_pcts5 =
			leds_pwm_duty_cycles.duty_pcts5;
		pwm_cfg->duty_cycles->duty_pcts6 =
			leds_pwm_duty_cycles.duty_pcts6;
		pwm_cfg->duty_cycles->duty_pcts7 =
			leds_pwm_duty_cycles.duty_pcts7;
		pwm_cfg->duty_cycles->duty_pcts8 =
			leds_pwm_duty_cycles.duty_pcts8;
		pwm_cfg->duty_cycles->duty_pcts12 =
			leds_pwm_duty_cycles.duty_pcts12;
		pwm_cfg->duty_cycles->duty_pcts13 =
			leds_pwm_duty_cycles.duty_pcts13;
		pwm_cfg->duty_cycles->duty_pcts14 =
			leds_pwm_duty_cycles.duty_pcts14;
		pwm_cfg->duty_cycles->duty_pcts17 =
			leds_pwm_duty_cycles.duty_pcts17;
		pwm_cfg->duty_cycles->duty_pcts18 =
			leds_pwm_duty_cycles.duty_pcts18;
		pwm_cfg->duty_cycles->duty_pcts19 =
			leds_pwm_duty_cycles.duty_pcts19;
		pwm_cfg->duty_cycles->duty_pcts20 =
			leds_pwm_duty_cycles.duty_pcts20;
		pwm_cfg->duty_cycles->duty_pcts29 =
			leds_pwm_duty_cycles.duty_pcts29;
		pwm_cfg->duty_cycles->duty_pcts30 =
			leds_pwm_duty_cycles.duty_pcts30;
		pwm_cfg->duty_cycles->duty_pcts31 =
			leds_pwm_duty_cycles.duty_pcts31;
		pwm_cfg->duty_cycles->duty_pcts32 =
			leds_pwm_duty_cycles.duty_pcts32;
		pwm_cfg->duty_cycles->duty_pcts101 =
			leds_pwm_duty_cycles.duty_pcts101;
		pwm_cfg->duty_cycles->duty_pcts102 =
			leds_pwm_duty_cycles.duty_pcts102;
#endif

		prop = of_find_property(node, "qcom,duty-pcts",
			&pwm_cfg->duty_cycles->num_duty_pcts);
		if (!prop) {
			dev_err(&spmi_dev->dev, "Looking up property " \
				"node qcom,duty-pcts failed\n");
			rc =  -ENODEV;
			goto bad_lpg_params;
		} else if (!pwm_cfg->duty_cycles->num_duty_pcts) {
			dev_err(&spmi_dev->dev, "Invalid length of " \
				"duty pcts\n");
			rc =  -EINVAL;
			goto bad_lpg_params;
		}

		pwm_cfg->duty_cycles->duty_pcts =
			devm_kzalloc(&spmi_dev->dev,
			sizeof(int) * PWM_LUT_MAX_SIZE,
			GFP_KERNEL);
		if (!pwm_cfg->duty_cycles->duty_pcts) {
			dev_err(&spmi_dev->dev,
				"Unable to allocate memory\n");
			rc = -ENOMEM;
			goto bad_lpg_params;
		}

		pwm_cfg->old_duty_pcts =
			devm_kzalloc(&spmi_dev->dev,
			sizeof(int) * PWM_LUT_MAX_SIZE,
			GFP_KERNEL);
		if (!pwm_cfg->old_duty_pcts) {
			dev_err(&spmi_dev->dev,
				"Unable to allocate memory\n");
			rc = -ENOMEM;
			goto bad_lpg_params;
		}

		temp_cfg = devm_kzalloc(&spmi_dev->dev,
				pwm_cfg->duty_cycles->num_duty_pcts *
				sizeof(u8), GFP_KERNEL);
		if (!temp_cfg) {
			dev_err(&spmi_dev->dev, "Failed to allocate " \
				"memory for duty pcts\n");
			rc = -ENOMEM;
			goto bad_lpg_params;
		}

		memcpy(temp_cfg, prop->value,
			pwm_cfg->duty_cycles->num_duty_pcts);

		for (i = 0; i < pwm_cfg->duty_cycles->num_duty_pcts; i++)
			pwm_cfg->duty_cycles->duty_pcts[i] =
				(int) temp_cfg[i];

		rc = of_property_read_u32(node, "qcom,start-idx", &val);
		if (!rc) {
			pwm_cfg->lut_params.start_idx = val;
			pwm_cfg->duty_cycles->start_idx = val;
		} else
			goto bad_lpg_params;

		pwm_cfg->lut_params.lut_pause_hi = 0;
		rc = of_property_read_u32(node, "qcom,pause-hi", &val);
		if (!rc)
			pwm_cfg->lut_params.lut_pause_hi = val;
		else if (rc != -EINVAL)
			goto bad_lpg_params;

		pwm_cfg->lut_params.lut_pause_lo = 0;
		rc = of_property_read_u32(node, "qcom,pause-lo", &val);
		if (!rc)
			pwm_cfg->lut_params.lut_pause_lo = val;
		else if (rc != -EINVAL)
			goto bad_lpg_params;

		pwm_cfg->lut_params.ramp_step_ms =
				QPNP_LUT_RAMP_STEP_DEFAULT;
		rc = of_property_read_u32(node, "qcom,ramp-step-ms", &val);
		if (!rc)
			pwm_cfg->lut_params.ramp_step_ms = val;
		else if (rc != -EINVAL)
			goto bad_lpg_params;

		pwm_cfg->lut_params.flags = QPNP_LED_PWM_FLAGS;
		rc = of_property_read_u32(node, "qcom,lut-flags", &val);
		if (!rc)
			pwm_cfg->lut_params.flags = (u8) val;
		else if (rc != -EINVAL)
			goto bad_lpg_params;

		pwm_cfg->lut_params.idx_len =
			pwm_cfg->duty_cycles->num_duty_pcts;
	}
	return 0;

bad_lpg_params:
	pwm_cfg->use_blink = false;
	if (pwm_cfg->mode == PWM_MODE) {
		dev_err(&spmi_dev->dev, "LPG parameters not set for" \
			" blink mode, defaulting to PWM mode\n");
		return 0;
	}
	return rc;
};

static int qpnp_led_get_mode(const char *mode)
{
	if (strncmp(mode, "manual", strlen(mode)) == 0)
		return MANUAL_MODE;
	else if (strncmp(mode, "pwm", strlen(mode)) == 0)
		return PWM_MODE;
	else if (strncmp(mode, "lpg", strlen(mode)) == 0)
		return LPG_MODE;
	else
		return -EINVAL;
};

static int __devinit qpnp_get_config_kpdbl(struct qpnp_led_data *led,
				struct device_node *node)
{
	int rc;
	u32 val;
	u8 led_mode;
	const char *mode;

	led->kpdbl_cfg = devm_kzalloc(&led->spmi_dev->dev,
				sizeof(struct kpdbl_config_data), GFP_KERNEL);
	if (!led->kpdbl_cfg) {
		dev_err(&led->spmi_dev->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	rc = of_property_read_string(node, "qcom,mode", &mode);
	if (!rc) {
		led_mode = qpnp_led_get_mode(mode);
		if ((led_mode == MANUAL_MODE) || (led_mode == -EINVAL)) {
			dev_err(&led->spmi_dev->dev, "Selected mode not " \
				"supported for kpdbl.\n");
			return -EINVAL;
		}
		led->kpdbl_cfg->pwm_cfg = devm_kzalloc(&led->spmi_dev->dev,
					sizeof(struct pwm_config_data),
					GFP_KERNEL);
		if (!led->kpdbl_cfg->pwm_cfg) {
			dev_err(&led->spmi_dev->dev,
				"Unable to allocate memory\n");
			return -ENOMEM;
		}
		led->kpdbl_cfg->pwm_cfg->mode = led_mode;
		led->kpdbl_cfg->pwm_cfg->default_mode = led_mode;
	} else
		return rc;

	rc = qpnp_get_config_pwm(led->kpdbl_cfg->pwm_cfg, led->spmi_dev,  node);
	if (rc < 0)
		return rc;

	rc = of_property_read_u32(node, "qcom,row-id", &val);
	if (!rc)
		led->kpdbl_cfg->row_id = val;
	else
		return rc;

	led->kpdbl_cfg->row_src_vbst =
			of_property_read_bool(node, "qcom,row-src-vbst");

	led->kpdbl_cfg->row_src_en =
			of_property_read_bool(node, "qcom,row-src-en");

	led->kpdbl_cfg->always_on =
			of_property_read_bool(node, "qcom,always-on");

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
	led->kpdbl_cfg->pwm_cfg->mode = led->kpdbl_cfg->pwm_cfg->default_mode;
	if (led->kpdbl_cfg->pwm_cfg->pwm_channel == 8)
		kpdbl_lpg1 = led;
	else if (led->kpdbl_cfg->pwm_cfg->pwm_channel == 9)
		kpdbl_lpg2 = led;
#endif

	return 0;
}

static int __devinit qpnp_get_config_rgb(struct qpnp_led_data *led,
				struct device_node *node)
{
	int rc;
	u8 led_mode;
	const char *mode;

	led->rgb_cfg = devm_kzalloc(&led->spmi_dev->dev,
				sizeof(struct rgb_config_data), GFP_KERNEL);
	if (!led->rgb_cfg) {
		dev_err(&led->spmi_dev->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	if (led->id == QPNP_ID_RGB_RED)
		led->rgb_cfg->enable = RGB_LED_ENABLE_RED;
	else if (led->id == QPNP_ID_RGB_GREEN)
		led->rgb_cfg->enable = RGB_LED_ENABLE_GREEN;
	else if (led->id == QPNP_ID_RGB_BLUE)
		led->rgb_cfg->enable = RGB_LED_ENABLE_BLUE;
	else
		return -EINVAL;

	rc = of_property_read_string(node, "qcom,mode", &mode);
	if (!rc) {
		led_mode = qpnp_led_get_mode(mode);
		if ((led_mode == MANUAL_MODE) || (led_mode == -EINVAL)) {
			dev_err(&led->spmi_dev->dev, "Selected mode not " \
				"supported for rgb.\n");
			return -EINVAL;
		}
		led->rgb_cfg->pwm_cfg = devm_kzalloc(&led->spmi_dev->dev,
					sizeof(struct pwm_config_data),
					GFP_KERNEL);
		if (!led->rgb_cfg->pwm_cfg) {
			dev_err(&led->spmi_dev->dev,
				"Unable to allocate memory\n");
			return -ENOMEM;
		}
		led->rgb_cfg->pwm_cfg->mode = led_mode;
		led->rgb_cfg->pwm_cfg->default_mode = led_mode;
	} else
		return rc;

	rc = qpnp_get_config_pwm(led->rgb_cfg->pwm_cfg, led->spmi_dev, node);
	if (rc < 0)
		return rc;

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
	if (led->id == QPNP_ID_RGB_RED)
		red_led = led;
	else if (led->id == QPNP_ID_RGB_GREEN)
		green_led = led;
	else if (led->id == QPNP_ID_RGB_BLUE)
		blue_led = led;
#endif
	return 0;
}

#ifdef CONFIG_LEDS_PM8941_EMOTIONAL
void change_led_pattern(int pattern)
{
	int *duty_pcts_red = NULL;
	int *duty_pcts_green = NULL;
	int *duty_pcts_blue = NULL;
	struct lut_params rgb_lut_params;
	int preview_pause_lo = 0;

	/* 1. set all leds brightness to 0 */
	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	/* 2. run work-function, as brightness 0, all led turn off
	 * qpnp_rgb_set(red_led);
	 * qpnp_rgb_set(green_led);
	 * qpnp_rgb_set(blue_led); */

	if (pattern >= 1000) {
		pattern = pattern - 1000;
		preview_pause_lo = 1000;
	}

	/* 3. change LUT structure in platform device. */
	switch (pattern) {
	case 0:
		duty_pcts_red	= red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;
		duty_pcts_blue	= blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;
		break;
	case 1:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts1;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts1;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts1;
		break;
	case 2:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts2;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts2;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts2;
		break;
	case 3:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts3;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts3;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts3;
		break;
	case 4:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts4;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts4;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts4;
		break;
	case 5:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts5;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts5;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts5;
		break;
	case 6:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts6;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts6;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts6;
		break;
	case 7:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts7;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts7;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts7;
		break;
	case 8:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts8;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts8;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts8;
		break;
	case 12:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts12;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts12;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts12;
		break;
	case 13:
		duty_pcts_red	= red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts13;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts13;
		duty_pcts_blue	= blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts13;
		break;
	case 14:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts14;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts14;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts14;
		break;
	case 17:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts17;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts17;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts17;
		break;
	case 18:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts18;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts18;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts18;
		break;
	case 19:
		duty_pcts_red	= red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts19;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts19;
		duty_pcts_blue	= blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts19;
		break;
	case 20:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts20;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts20;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts20;
		break;
	case 29:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts29;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts29;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts29;
		break;
	case 30:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts30;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts30;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts30;
		break;
	case 31:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts31;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts31;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts31;
		break;
	case 32:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts32;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts32;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts32;
		break;
	case 101:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts101;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts101;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts101;
		break;
	case 102:
		duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts102;
		duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts102;
		duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts102;
		break;
	default:
		return;
	}

	/* 4. lut disable, so we can edit LUT table after done this. */
	pwm_disable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	/* 5. lut config(red led). */
	rgb_lut_params.start_idx = duty_pcts_red[63];
	rgb_lut_params.idx_len = duty_pcts_red[64];
	rgb_lut_params.lut_pause_hi = duty_pcts_red[66];
	rgb_lut_params.ramp_step_ms = duty_pcts_red[78];
	rgb_lut_params.flags = duty_pcts_red[75];

	if (rgb_lut_params.flags & 0x10) { /* for first missed noti delay */
		rgb_lut_params.lut_pause_lo = 1000;
#if defined(CONFIG_MACH_MSM8974_G2_SPR) || defined(CONFIG_MACH_MSM8974_G2_VZW)
		if (preview_pause_lo == 0) /* normal missed noti pattern */
			rgb_lut_params.lut_pause_lo = 4000;
#endif
	}
	pwm_lut_config(red_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[duty_pcts_red[63]], rgb_lut_params);

	/* 6. lut config(green led). */
	rgb_lut_params.start_idx = duty_pcts_red[67];
	rgb_lut_params.idx_len = duty_pcts_red[68];
	rgb_lut_params.lut_pause_hi = duty_pcts_red[70];
	rgb_lut_params.flags = duty_pcts_red[76];
	pwm_lut_config(green_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[duty_pcts_red[67]], rgb_lut_params);

	/* 7. lut config(blue led). */
	rgb_lut_params.start_idx = duty_pcts_blue[71];
	rgb_lut_params.idx_len = duty_pcts_blue[72];
	rgb_lut_params.lut_pause_hi = duty_pcts_blue[74];
	rgb_lut_params.flags = duty_pcts_blue[77];
	pwm_lut_config(blue_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[duty_pcts_red[71]], rgb_lut_params);

	/* 8. lut enable, so we can run led after done this. */
	pwm_enable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	/* 9. set leds brightness to 255 */
	red_led->cdev.brightness = 255;
	green_led->cdev.brightness = 255;
	blue_led->cdev.brightness = 255;

	if (duty_pcts_red[65] == 0)
		red_led->cdev.brightness = 0;
	if (duty_pcts_red[69] == 0)
		green_led->cdev.brightness = 0;
	if (duty_pcts_red[73] == 0)
		blue_led->cdev.brightness = 0;

	/* 10. run work-function, as brightness 255, all led turn on */
	qpnp_rgb_set(red_led);
	qpnp_rgb_set(green_led);
	qpnp_rgb_set(blue_led);

}

void make_input_led_pattern(int patterns[],
			int red_start, int red_length, int red_duty,
			int red_pause, int green_start, int green_length,
			int green_duty, int green_pause, int blue_start,
			int blue_length, int blue_duty, int blue_pause,
			int red_flag, int green_flag,
			int blue_flag, int period)
{
	int *duty_pcts_red = NULL;
	int *duty_pcts_green = NULL;
	int *duty_pcts_blue = NULL;
	int input_patterns[79];
	int i;

	struct lut_params rgb_lut_params;

	for (i = 0; i < 63; i++)
		input_patterns[i] = patterns[i];
	input_patterns[i++] = red_start;
	input_patterns[i++] = red_length;
	input_patterns[i++] = red_duty;
	input_patterns[i++] = red_pause;
	input_patterns[i++] = green_start;
	input_patterns[i++] = green_length;
	input_patterns[i++] = green_duty;
	input_patterns[i++] = green_pause;
	input_patterns[i++] = blue_start;
	input_patterns[i++] = blue_length;
	input_patterns[i++] = blue_duty;
	input_patterns[i++] = blue_pause;
	input_patterns[i++] = red_flag;
	input_patterns[i++] = green_flag;
	input_patterns[i++] = blue_flag;
	input_patterns[i++] = period;

	/* 1. set all leds brightness to 0 */
	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	/*  2. run work-function, as brightness 0, all led turn off */
	/* qpnp_rgb_set(red_led); */
	/* qpnp_rgb_set(green_led); */
	/* qpnp_rgb_set(blue_led); */

	/* 3. change LUT structure in platform device. */
	red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0 = (int *)&input_patterns;
	green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0 = (int *)&input_patterns;
	blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0 = (int *)&input_patterns;

	duty_pcts_red   = red_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;
	duty_pcts_green = green_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;
	duty_pcts_blue  = blue_led->rgb_cfg->pwm_cfg->duty_cycles->duty_pcts0;

	/* 4. lut disable, so we can edit LUT table after done this. */
	pwm_disable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	/* 5. lut config(red led). */
	/* rgb_lut_params.start_idx = duty_pcts_red[63]+1; */
	rgb_lut_params.start_idx = red_start;
	rgb_lut_params.idx_len = red_length;
	rgb_lut_params.lut_pause_hi = red_pause;
	rgb_lut_params.lut_pause_lo = 0;
	rgb_lut_params.ramp_step_ms = period;
	rgb_lut_params.flags = red_flag;
	pwm_lut_config(red_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[red_start], rgb_lut_params);

	/* 6. lut config(green led). */
	/* rgb_lut_params.start_idx = duty_pcts_red[67]+1; */
	rgb_lut_params.start_idx = green_start;
	rgb_lut_params.idx_len = green_length;
	rgb_lut_params.lut_pause_hi = green_pause;
	rgb_lut_params.flags = green_flag;
	pwm_lut_config(green_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[green_start], rgb_lut_params);

	/* 7. lut config(blue led). */
	/* rgb_lut_params.start_idx = duty_pcts_blue[71]+1 */
	rgb_lut_params.start_idx = blue_start;
	rgb_lut_params.idx_len = blue_length;
	rgb_lut_params.lut_pause_hi = blue_pause;
	rgb_lut_params.flags = blue_flag;
	pwm_lut_config(blue_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&duty_pcts_red[blue_start], rgb_lut_params);

	/* 8. lut enable, so we can run led after done this. */
	pwm_enable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	/* 9. set leds brightness to 255 */
	red_led->cdev.brightness = 255;
	green_led->cdev.brightness = 255;
	blue_led->cdev.brightness = 255;
	if (red_duty == 0)
		red_led->cdev.brightness = 0;
	if (green_duty == 0)
		green_led->cdev.brightness = 0;
	if (blue_duty == 0)
		blue_led->cdev.brightness = 0;
	/* 10. run work-function, as brightness 255, all led turn on */
	qpnp_rgb_set(red_led);
	qpnp_rgb_set(green_led);
	qpnp_rgb_set(blue_led);
}

void make_blink_led_pattern(int rgb, int delay_on, int delay_off)
{
	int blink_pattern[6] = {0, 0, 0, 0, 0, 0};
	struct lut_params rgb_lut_params;
	int rgb_devider;

	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	pwm_disable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	rgb_lut_params.idx_len = 2;
	rgb_lut_params.lut_pause_hi = delay_off/2;
	rgb_lut_params.lut_pause_lo = delay_on/2;
	rgb_lut_params.ramp_step_ms = 1;
	rgb_lut_params.flags = 89;

	rgb_devider = (((rgb >> 16) & 0xFF) && 1) +
		(((rgb >> 8) & 0xFF) && 1) + ((rgb & 0xFF) && 1);
	printk(KERN_INFO "[RGB LED] rgb_devider = %d\n", rgb_devider);

	if (rgb_devider) {
#if defined(CONFIG_MACH_MSM8974_G2_DCM)
		blink_pattern[0] = (((rgb >> 16) & 0xFF)*2)/rgb_devider;
		blink_pattern[2] = ((((rgb >> 8) & 0xFF)*2)*6/10)/rgb_devider;
#else
		blink_pattern[0] = ((((rgb >> 16) & 0xFF)*2)*7/10)/rgb_devider;
		blink_pattern[2] = (((rgb >> 8) & 0xFF)*2)/rgb_devider;
#endif
		blink_pattern[4] = (rgb & 0xFF)/rgb_devider;
	}

	rgb_lut_params.start_idx = 0;
	pwm_lut_config(red_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		blink_pattern, rgb_lut_params);

	rgb_lut_params.start_idx = 2;
	pwm_lut_config(green_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&blink_pattern[2], rgb_lut_params);

	rgb_lut_params.start_idx = 4;
	pwm_lut_config(blue_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&blink_pattern[4], rgb_lut_params);

	pwm_enable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	red_led->cdev.brightness = (rgb >> 16) & 0xFF;
	green_led->cdev.brightness = (rgb >> 8) & 0xFF;
	blue_led->cdev.brightness = rgb & 0xFF;

	printk(KERN_INFO "[RGB LED] brightness R:%d G:%d B:%d\n", red_led->cdev.brightness,
		green_led->cdev.brightness, blue_led->cdev.brightness);

	qpnp_rgb_set(red_led);
	qpnp_rgb_set(green_led);
	qpnp_rgb_set(blue_led);
}

void make_onoff_led_pattern(int rgb)
{
	int onoff_pattern[6] = {100, 100, 100, 100, 100, 100};
	struct lut_params rgb_lut_params;

	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	pwm_disable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_disable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	rgb_lut_params.idx_len = 2;
	rgb_lut_params.lut_pause_hi = 0;
	rgb_lut_params.lut_pause_lo = 0;
	rgb_lut_params.ramp_step_ms = 0;
	rgb_lut_params.flags = 65;

	/* tuning RGB input from framework.
	     PM8921 can use 512 resolution
	     R : (rgb*1.4)/3     G : (rgb*2)/3     B : rgb/3
	*/
#if defined(CONFIG_MACH_MSM8974_G2_DCM)
	onoff_pattern[0] = (((rgb >> 16) & 0xFF)*2)/3;
	onoff_pattern[2] = (((rgb >> 8) & 0xFF)*2*6/10)/3;
#else
	onoff_pattern[0] = (((rgb >> 16) & 0xFF)*2*7/10)/3;
	onoff_pattern[2] = (((rgb >> 8) & 0xFF)*2)/3;
#endif
	onoff_pattern[1] = onoff_pattern[0];
	onoff_pattern[3] = onoff_pattern[2];
	onoff_pattern[4] = (rgb & 0xFF)/3;
	onoff_pattern[5] = onoff_pattern[4];

	rgb_lut_params.start_idx = 0;
	pwm_lut_config(red_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		onoff_pattern, rgb_lut_params);

	rgb_lut_params.start_idx = 2;
	pwm_lut_config(green_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&onoff_pattern[2], rgb_lut_params);

	rgb_lut_params.start_idx = 4;
	pwm_lut_config(blue_led->rgb_cfg->pwm_cfg->pwm_dev, 200,
		&onoff_pattern[4], rgb_lut_params);

	pwm_enable(red_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(green_led->rgb_cfg->pwm_cfg->pwm_dev);
	pwm_enable(blue_led->rgb_cfg->pwm_cfg->pwm_dev);

	red_led->cdev.brightness = (rgb >> 16) & 0xFF;
	green_led->cdev.brightness = (rgb >> 8) & 0xFF;
	blue_led->cdev.brightness = rgb & 0xFF;

	qpnp_rgb_set(red_led);
	qpnp_rgb_set(green_led);
	qpnp_rgb_set(blue_led);

}

/* below function is for aat... */
void rgb_luts_set(struct qpnp_led_data *led)
{
#if defined(CONFIG_MACH_MSM8974_B1_KR)
	int rgb_brightness = 0;
	rgb_brightness = (red_led->cdev.brightness << 16) + (green_led->cdev.brightness << 8) + blue_led->cdev.brightness;
	if (rgb_brightness > 0)
		make_onoff_led_pattern(rgb_brightness);
#else
	if (led->id == 3 && led->cdev.brightness > 0)
		make_onoff_led_pattern(0xFF0000);
	else if (led->id == 4 && led->cdev.brightness > 0)
		make_onoff_led_pattern(0xFF00);
	else if (led->id == 5 && led->cdev.brightness > 0)
		make_onoff_led_pattern(0xFF);
#endif
	else
		make_onoff_led_pattern(0);

	printk(KERN_INFO "[RGB LED] brightness R:%d G:%d B:%d\n", red_led->cdev.brightness,
	green_led->cdev.brightness, blue_led->cdev.brightness);
}

void set_kpdbl_pattern(int pattern)
{
	int duty_pcts_kpdbl[30] = {
			14, 17, 18, 21, 23, 25, 27, 29, 31, 34,
			36, 38, 40, 42, 44, 46, 48, 51, 53, 55,
			57, 59, 61, 64, 66, 68, 70, 71, 71, 71};

	int duty_pcts_kpdbl_36[30] = {
			0, 170, 165, 158, 150, 138, 124, 109, 92, 73,
			53, 32, 10, 0, 0, 0, 170, 165, 158, 150,
			138, 124, 109, 92, 73, 53, 32, 10, 0, 0,};

	struct lut_params kpdbl_lut_params;

	if (pattern > 1000)
		pattern = pattern - 1000;

	if (!pattern && is_kpdbl_on == 1) {
		pwm_disable(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev);
		pwm_disable(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev);

		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->mode = PWM_MODE;
		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->default_mode = PWM_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->mode = PWM_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->default_mode = PWM_MODE;

		qpnp_led_masked_write(kpdbl_lpg1, 0xE3C8, 0x00, 0x00);
		qpnp_led_masked_write(kpdbl_lpg2, 0xE3C8, 0x00, 0x00);

		is_kpdbl_on = 0;
	} else if (pattern == 35 && is_kpdbl_on == 0) {
		pwm_disable(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev);
		pwm_disable(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev);

		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->mode = LPG_MODE;
		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->default_mode = LPG_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->mode = LPG_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->default_mode = LPG_MODE;

		kpdbl_lut_params.start_idx = -1;
		kpdbl_lut_params.idx_len = 30;
		kpdbl_lut_params.lut_pause_hi = 700;
		kpdbl_lut_params.lut_pause_lo = 400;
		kpdbl_lut_params.ramp_step_ms = 24;
		kpdbl_lut_params.flags = 95;

		pwm_lut_config(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev, 200,
			duty_pcts_kpdbl, kpdbl_lut_params);
		pwm_lut_config(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev, 200,
			duty_pcts_kpdbl, kpdbl_lut_params);

		pwm_enable(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev);
		pwm_enable(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev);

		kpdbl_lpg1->cdev.brightness = 127;
		kpdbl_lpg2->cdev.brightness = 127;

		qpnp_kpdbl_set(kpdbl_lpg1);
		qpnp_kpdbl_set(kpdbl_lpg2);

		is_kpdbl_on = 1;
	} else if (pattern == 36 && is_kpdbl_on == 0) {
		pwm_disable(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev);
		pwm_disable(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev);

		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->mode = LPG_MODE;
		kpdbl_lpg1->kpdbl_cfg->pwm_cfg->default_mode = LPG_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->mode = LPG_MODE;
		kpdbl_lpg2->kpdbl_cfg->pwm_cfg->default_mode = LPG_MODE;

		kpdbl_lut_params.start_idx = -1;
		kpdbl_lut_params.idx_len = 30;
		kpdbl_lut_params.lut_pause_hi = 2280;
		kpdbl_lut_params.lut_pause_lo = 1000;
		kpdbl_lut_params.ramp_step_ms = 24;
		kpdbl_lut_params.flags = 91;

		pwm_lut_config(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev, 200,
			duty_pcts_kpdbl_36, kpdbl_lut_params);
		pwm_lut_config(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev, 200,
			duty_pcts_kpdbl_36, kpdbl_lut_params);

		pwm_enable(kpdbl_lpg1->kpdbl_cfg->pwm_cfg->pwm_dev);
		pwm_enable(kpdbl_lpg2->kpdbl_cfg->pwm_cfg->pwm_dev);

		kpdbl_lpg1->cdev.brightness = 127;
		kpdbl_lpg2->cdev.brightness = 127;

		qpnp_kpdbl_set(kpdbl_lpg1);
		qpnp_kpdbl_set(kpdbl_lpg2);

		is_kpdbl_on = 1;
	}
}
#endif

static int __devinit qpnp_get_config_mpp(struct qpnp_led_data *led,
		struct device_node *node)
{
	int rc;
	u32 val;
	u8 led_mode;
	const char *mode;

	led->mpp_cfg = devm_kzalloc(&led->spmi_dev->dev,
			sizeof(struct mpp_config_data), GFP_KERNEL);
	if (!led->mpp_cfg) {
		dev_err(&led->spmi_dev->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	led->mpp_cfg->current_setting = LED_MPP_CURRENT_MIN;
	rc = of_property_read_u32(node, "qcom,current-setting", &val);
	if (!rc) {
		if (led->mpp_cfg->current_setting < LED_MPP_CURRENT_MIN)
			led->mpp_cfg->current_setting = LED_MPP_CURRENT_MIN;
		else if (led->mpp_cfg->current_setting > LED_MPP_CURRENT_MAX)
			led->mpp_cfg->current_setting = LED_MPP_CURRENT_MAX;
		else
			led->mpp_cfg->current_setting = (u8) val;
	} else if (rc != -EINVAL)
		return rc;

	led->mpp_cfg->source_sel = LED_MPP_SOURCE_SEL_DEFAULT;
	rc = of_property_read_u32(node, "qcom,source-sel", &val);
	if (!rc)
		led->mpp_cfg->source_sel = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->mpp_cfg->mode_ctrl = LED_MPP_MODE_SINK;
	rc = of_property_read_u32(node, "qcom,mode-ctrl", &val);
	if (!rc)
		led->mpp_cfg->mode_ctrl = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->mpp_cfg->vin_ctrl = LED_MPP_VIN_CTRL_DEFAULT;
	rc = of_property_read_u32(node, "qcom,vin-ctrl", &val);
	if (!rc)
		led->mpp_cfg->vin_ctrl = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	led->mpp_cfg->min_brightness = 0;
	rc = of_property_read_u32(node, "qcom,min-brightness", &val);
	if (!rc)
		led->mpp_cfg->min_brightness = (u8) val;
	else if (rc != -EINVAL)
		return rc;

	rc = of_property_read_string(node, "qcom,mode", &mode);
	if (!rc) {
		led_mode = qpnp_led_get_mode(mode);
		led->mpp_cfg->pwm_mode = led_mode;
		if (led_mode == MANUAL_MODE)
			return MANUAL_MODE;
		else if (led_mode == -EINVAL) {
			dev_err(&led->spmi_dev->dev, "Selected mode not " \
				"supported for mpp.\n");
			return -EINVAL;
		}
		led->mpp_cfg->pwm_cfg = devm_kzalloc(&led->spmi_dev->dev,
					sizeof(struct pwm_config_data),
					GFP_KERNEL);
		if (!led->mpp_cfg->pwm_cfg) {
			dev_err(&led->spmi_dev->dev,
				"Unable to allocate memory\n");
			return -ENOMEM;
		}
		led->mpp_cfg->pwm_cfg->mode = led_mode;
		led->mpp_cfg->pwm_cfg->default_mode = led_mode;
	} else
		return rc;

	rc = qpnp_get_config_pwm(led->mpp_cfg->pwm_cfg, led->spmi_dev, node);
	if (rc < 0)
		return rc;

	return 0;
}

static int __devinit qpnp_leds_probe(struct spmi_device *spmi)
{
	struct qpnp_led_data *led, *led_array;
	struct resource *led_resource;
	struct device_node *node, *temp;
	int rc, i, num_leds = 0, parsed_leds = 0;
	const char *led_label;
	bool regulator_probe = false;

	node = spmi->dev.of_node;
	if (node == NULL)
		return -ENODEV;

	temp = NULL;
	while ((temp = of_get_next_child(node, temp)))
		num_leds++;

	if (!num_leds)
		return -ECHILD;

	led_array = devm_kzalloc(&spmi->dev,
		(sizeof(struct qpnp_led_data) * num_leds), GFP_KERNEL);
	if (!led_array) {
		dev_err(&spmi->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	for_each_child_of_node(node, temp) {
		led = &led_array[parsed_leds];
		led->num_leds = num_leds;
		led->spmi_dev = spmi;

		led_resource = spmi_get_resource(spmi, NULL, IORESOURCE_MEM, 0);
		if (!led_resource) {
			dev_err(&spmi->dev, "Unable to get LED base address\n");
			rc = -ENXIO;
			goto fail_id_check;
		}
		led->base = led_resource->start;

		rc = of_property_read_string(temp, "label", &led_label);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev,
				"Failure reading label, rc = %d\n", rc);
			goto fail_id_check;
		}

		rc = of_property_read_string(temp, "linux,name",
			&led->cdev.name);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev,
				"Failure reading led name, rc = %d\n", rc);
			goto fail_id_check;
		}

		rc = of_property_read_u32(temp, "qcom,max-current",
			&led->max_current);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev,
				"Failure reading max_current, rc =  %d\n", rc);
			goto fail_id_check;
		}

		rc = of_property_read_u32(temp, "qcom,id", &led->id);
		if (rc < 0) {
			dev_err(&led->spmi_dev->dev,
				"Failure reading led id, rc =  %d\n", rc);
			goto fail_id_check;
		}

		rc = qpnp_get_common_configs(led, temp);
		if (rc) {
			dev_err(&led->spmi_dev->dev,
				"Failure reading common led configuration," \
				" rc = %d\n", rc);
			goto fail_id_check;
		}

		led->cdev.brightness_set    = qpnp_led_set;
		led->cdev.brightness_get    = qpnp_led_get;

		if (strncmp(led_label, "wled", sizeof("wled")) == 0) {
			rc = qpnp_get_config_wled(led, temp);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
					"Unable to read wled config data\n");
				goto fail_id_check;
			}
		} else if (strncmp(led_label, "flash", sizeof("flash"))
				== 0) {
			if (!of_find_property(node, "flash-boost-supply", NULL))
				regulator_probe = true;
			rc = qpnp_get_config_flash(led, temp, &regulator_probe);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
					"Unable to read flash config data\n");
				goto fail_id_check;
			}
		} else if (strncmp(led_label, "rgb", sizeof("rgb")) == 0) {
			rc = qpnp_get_config_rgb(led, temp);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
					"Unable to read rgb config data\n");
				goto fail_id_check;
			}
		} else if (strncmp(led_label, "mpp", sizeof("mpp")) == 0) {
			rc = qpnp_get_config_mpp(led, temp);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
						"Unable to read mpp config data\n");
				goto fail_id_check;
			}
		} else if (strncmp(led_label, "kpdbl", sizeof("kpdbl")) == 0) {
			num_kpbl_leds_on = 0;
			rc = qpnp_get_config_kpdbl(led, temp);
			if (rc < 0) {
				dev_err(&led->spmi_dev->dev,
					"Unable to read kpdbl config data\n");
				goto fail_id_check;
			}
		} else {
			dev_err(&led->spmi_dev->dev, "No LED matching label\n");
			rc = -EINVAL;
			goto fail_id_check;
		}

		mutex_init(&led->lock);
		INIT_WORK(&led->work, qpnp_led_work);

		rc =  qpnp_led_initialize(led);
		if (rc < 0)
			goto fail_id_check;

		rc = qpnp_led_set_max_brightness(led);
		if (rc < 0)
			goto fail_id_check;

		rc = led_classdev_register(&spmi->dev, &led->cdev);
		if (rc) {
			dev_err(&spmi->dev, "unable to register led %d,rc=%d\n",
						 led->id, rc);
			goto fail_id_check;
		}

		if (led->id == QPNP_ID_FLASH1_LED0 ||
			led->id == QPNP_ID_FLASH1_LED1) {
			rc = sysfs_create_group(&led->cdev.dev->kobj,
							&led_attr_group);
			if (rc)
				goto fail_id_check;

		}

		if (led->id == QPNP_ID_LED_MPP) {
			if (!led->mpp_cfg->pwm_cfg)
				break;
			if (led->mpp_cfg->pwm_cfg->mode == PWM_MODE) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&pwm_attr_group);
				if (rc)
					goto fail_id_check;
			}
			if (led->mpp_cfg->pwm_cfg->use_blink) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&blink_attr_group);
				if (rc)
					goto fail_id_check;

				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&lpg_attr_group);
				if (rc)
					goto fail_id_check;
			} else if (led->mpp_cfg->pwm_cfg->mode == LPG_MODE) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&lpg_attr_group);
				if (rc)
					goto fail_id_check;
			}
		} else if ((led->id == QPNP_ID_RGB_RED) ||
			(led->id == QPNP_ID_RGB_GREEN) ||
			(led->id == QPNP_ID_RGB_BLUE)) {
			if (led->rgb_cfg->pwm_cfg->mode == PWM_MODE) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&pwm_attr_group);
				if (rc)
					goto fail_id_check;
			}
			if (led->rgb_cfg->pwm_cfg->use_blink) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&blink_attr_group);
				if (rc)
					goto fail_id_check;

				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&lpg_attr_group);
				if (rc)
					goto fail_id_check;
			} else if (led->rgb_cfg->pwm_cfg->mode == LPG_MODE) {
				rc = sysfs_create_group(&led->cdev.dev->kobj,
					&lpg_attr_group);
				if (rc)
					goto fail_id_check;
			}
		}

		/* configure default state */
		if (led->default_on) {
			led->cdev.brightness = led->cdev.max_brightness;
			__qpnp_led_work(led, led->cdev.brightness);
			if (led->turn_off_delay_ms > 0)
				qpnp_led_turn_off(led);
		} else
			led->cdev.brightness = LED_OFF;

		parsed_leds++;
	}
	dev_set_drvdata(&spmi->dev, led_array);

	return 0;

fail_id_check:
	for (i = 0; i < parsed_leds; i++) {
		mutex_destroy(&led_array[i].lock);
		led_classdev_unregister(&led_array[i].cdev);
	}

	return rc;
}

static int __devexit qpnp_leds_remove(struct spmi_device *spmi)
{
	struct qpnp_led_data *led_array  = dev_get_drvdata(&spmi->dev);
	int i, parsed_leds = led_array->num_leds;

	for (i = 0; i < parsed_leds; i++) {
		cancel_work_sync(&led_array[i].work);
		mutex_destroy(&led_array[i].lock);
		led_classdev_unregister(&led_array[i].cdev);
		switch (led_array[i].id) {
		case QPNP_ID_WLED:
			break;
		case QPNP_ID_FLASH1_LED0:
		case QPNP_ID_FLASH1_LED1:
			if (led_array[i].flash_cfg->flash_reg_get)
				regulator_put(led_array[i].flash_cfg-> \
							flash_boost_reg);
			if (led_array[i].flash_cfg->torch_enable)
				regulator_put(led_array[i].flash_cfg->\
							torch_boost_reg);
			sysfs_remove_group(&led_array[i].cdev.dev->kobj,
							&led_attr_group);
			break;
		case QPNP_ID_RGB_RED:
		case QPNP_ID_RGB_GREEN:
		case QPNP_ID_RGB_BLUE:
			if (led_array[i].rgb_cfg->pwm_cfg->mode == PWM_MODE)
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &pwm_attr_group);
			if (led_array[i].rgb_cfg->pwm_cfg->use_blink) {
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &blink_attr_group);
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &lpg_attr_group);
			} else if (led_array[i].rgb_cfg->pwm_cfg->mode\
					== LPG_MODE)
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &lpg_attr_group);
			break;
		case QPNP_ID_LED_MPP:
			if (!led_array[i].mpp_cfg->pwm_cfg)
				break;
			if (led_array[i].mpp_cfg->pwm_cfg->mode == PWM_MODE)
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &pwm_attr_group);
			if (led_array[i].mpp_cfg->pwm_cfg->use_blink) {
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &blink_attr_group);
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &lpg_attr_group);
			} else if (led_array[i].mpp_cfg->pwm_cfg->mode\
					== LPG_MODE)
				sysfs_remove_group(&led_array[i].cdev.dev->\
					kobj, &lpg_attr_group);
			break;
		default:
			dev_err(&led_array[i].spmi_dev->dev,
					"Invalid LED(%d)\n",
					led_array[i].id);
			return -EINVAL;
		}
	}

	return 0;
}
static struct of_device_id spmi_match_table[] = {
	{	.compatible = "qcom,leds-qpnp",
	}
};

static struct spmi_driver qpnp_leds_driver = {
	.driver		= {
		.name	= "qcom,leds-qpnp",
		.of_match_table = spmi_match_table,
	},
	.probe		= qpnp_leds_probe,
	.remove		= __devexit_p(qpnp_leds_remove),
};

static int __init qpnp_led_init(void)
{
	return spmi_driver_register(&qpnp_leds_driver);
}
module_init(qpnp_led_init);

static void __exit qpnp_led_exit(void)
{
	spmi_driver_unregister(&qpnp_leds_driver);
}
module_exit(qpnp_led_exit);

MODULE_DESCRIPTION("QPNP LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("leds:leds-qpnp");

