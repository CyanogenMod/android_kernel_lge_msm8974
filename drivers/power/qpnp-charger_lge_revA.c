/* Copyright (c) 2012-2013 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/spmi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/radix-tree.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/qpnp/qpnp-adc.h>
#include <linux/power_supply.h>
#include <linux/bitops.h>
/*                                               */
#include "../../arch/arm/mach-msm/smd_private.h"
/*                                               */
#ifdef CONFIG_LGE_PM
#include <mach/board_lge.h>
#include <linux/max17048_battery.h>
#include <linux/qpnp/qpnp-charger.h>
#endif



/* Interrupt offsets */
#define INT_RT_STS(base)			(base + 0x10)
#define INT_SET_TYPE(base)			(base + 0x11)
#define INT_POLARITY_HIGH(base)			(base + 0x12)
#define INT_POLARITY_LOW(base)			(base + 0x13)
#define INT_LATCHED_CLR(base)			(base + 0x14)
#define INT_EN_SET(base)			(base + 0x15)
#define INT_EN_CLR(base)			(base + 0x16)
#define INT_LATCHED_STS(base)			(base + 0x18)
#define INT_PENDING_STS(base)			(base + 0x19)
#define INT_MID_SEL(base)			(base + 0x1A)
#define INT_PRIORITY(base)			(base + 0x1B)

/* Peripheral register offsets */
#define CHGR_CHG_OPTION				0x08
#define CHGR_ATC_STATUS				0x0A
#define CHGR_VBAT_STATUS			0x0B
#define CHGR_IBAT_BMS				0x0C
#define CHGR_IBAT_STS				0x0D
#define CHGR_VDD_MAX				0x40
#define CHGR_VDD_SAFE				0x41
#define CHGR_VDD_MAX_STEP			0x42
#define CHGR_IBAT_MAX				0x44
#define CHGR_IBAT_SAFE				0x45
#define CHGR_VIN_MIN				0x47
#define CHGR_VIN_MIN_STEP			0x48
#define CHGR_CHG_CTRL				0x49
#define CHGR_CHG_FAILED				0x4A
#define CHGR_ATC_CTRL				0x4B
#define CHGR_ATC_FAILED				0x4C
#define CHGR_VBAT_TRKL				0x50
#define CHGR_VBAT_WEAK				0x52
#define CHGR_IBAT_ATC_A				0x54
#define CHGR_IBAT_ATC_B				0x55
#define CHGR_IBAT_TERM_CHGR			0x5B
#define CHGR_IBAT_TERM_BMS			0x5C
#define CHGR_VBAT_DET				0x5D
#define CHGR_TTRKL_MAX				0x5F
#define CHGR_TTRKL_MAX_EN			0x60
#define CHGR_TCHG_MAX				0x61
#define CHGR_CHG_WDOG_TIME			0x62
#define CHGR_CHG_WDOG_DLY			0x63
#define CHGR_CHG_WDOG_PET			0x64
#define CHGR_CHG_WDOG_EN			0x65
#define CHGR_IR_DROP_COMPEN			0x67
#define CHGR_I_MAX_REG			0x44
#define CHGR_USB_USB_SUSP			0x47
#define CHGR_USB_USB_OTG_CTL			0x48
#define CHGR_USB_ENUM_T_STOP			0x4E
#define CHGR_CHG_TEMP_THRESH			0x66
#define CHGR_BAT_IF_PRES_STATUS			0x08
#define CHGR_STATUS				0x09
#define CHGR_BAT_IF_VCP				0x42
#define CHGR_BAT_IF_BATFET_CTRL1		0x90
#define CHGR_MISC_BOOT_DONE			0x42
#define CHGR_BUCK_COMPARATOR_OVRIDE_3		0xED
#define CHGR_BUCK_BCK_VBAT_REG_MODE		0x74
#define MISC_REVISION2				0x01
#define USB_OVP_CTL				0x42
#define SEC_ACCESS				0xD0

#define REG_OFFSET_PERP_SUBTYPE			0x05
/* SMBB peripheral subtype values */
#define SMBB_CHGR_SUBTYPE			0x01
#define SMBB_BUCK_SUBTYPE			0x02
#define SMBB_BAT_IF_SUBTYPE			0x03
#define SMBB_USB_CHGPTH_SUBTYPE			0x04
#define SMBB_DC_CHGPTH_SUBTYPE			0x05
#define SMBB_BOOST_SUBTYPE			0x06
#define SMBB_MISC_SUBTYPE			0x07

/* SMBB peripheral subtype values */
#define SMBBP_CHGR_SUBTYPE			0x31
#define SMBBP_BUCK_SUBTYPE			0x32
#define SMBBP_BAT_IF_SUBTYPE			0x33
#define SMBBP_USB_CHGPTH_SUBTYPE		0x34
#define SMBBP_BOOST_SUBTYPE			0x36
#define SMBBP_MISC_SUBTYPE			0x37

#define QPNP_CHARGER_DEV_NAME	"qcom,qpnp-charger"

/* Status bits and masks */
#define CHGR_BOOT_DONE			BIT(7)
#define CHGR_CHG_EN			BIT(7)
#define CHGR_ON_BAT_FORCE_BIT		BIT(0)
#define USB_VALID_DEB_20MS		0x03
#define BUCK_VBAT_REG_NODE_SEL_BIT	BIT(0)

/* Interrupt definitions */
/* smbb_chg_interrupts */
#define CHG_DONE_IRQ			BIT(7)
#define CHG_FAILED_IRQ			BIT(6)
#define FAST_CHG_ON_IRQ			BIT(5)
#define TRKL_CHG_ON_IRQ			BIT(4)
#define STATE_CHANGE_ON_IR		BIT(3)
#define CHGWDDOG_IRQ			BIT(2)
#define VBAT_DET_HI_IRQ			BIT(1)
#define VBAT_DET_LOW_IRQ		BIT(0)

/* smbb_buck_interrupts */
#define VDD_LOOP_IRQ			BIT(6)
#define IBAT_LOOP_IRQ			BIT(5)
#define ICHG_LOOP_IRQ			BIT(4)
#define VCHG_LOOP_IRQ			BIT(3)
#define OVERTEMP_IRQ			BIT(2)
#define VREF_OV_IRQ			BIT(1)
#define VBAT_OV_IRQ			BIT(0)

/* smbb_bat_if_interrupts */
#define PSI_IRQ				BIT(4)
#define VCP_ON_IRQ			BIT(3)
#define BAT_FET_ON_IRQ			BIT(2)
#define BAT_TEMP_OK_IRQ			BIT(1)
#define BATT_PRES_IRQ			BIT(0)

/* smbb_usb_interrupts */
#define CHG_GONE_IRQ			BIT(2)
#define USBIN_VALID_IRQ			BIT(1)
#define COARSE_DET_USB_IRQ		BIT(0)

/* smbb_dc_interrupts */
#define DCIN_VALID_IRQ			BIT(1)
#define COARSE_DET_DC_IRQ		BIT(0)

/* smbb_boost_interrupts */
#define LIMIT_ERROR_IRQ			BIT(1)
#define BOOST_PWR_OK_IRQ		BIT(0)

/* smbb_misc_interrupts */
#define TFTWDOG_IRQ			BIT(0)

/* Workaround flags */
#define CHG_FLAGS_VCP_WA		BIT(0)

/**
 * struct qpnp_chg_chip - device information
 * @dev:			device pointer to access the parent
 * @spmi:			spmi pointer to access spmi information
 * @chgr_base:			charger peripheral base address
 * @buck_base:			buck  peripheral base address
 * @bat_if_base:		battery interface  peripheral base address
 * @usb_chgpth_base:		USB charge path peripheral base address
 * @dc_chgpth_base:		DC charge path peripheral base address
 * @boost_base:			boost peripheral base address
 * @misc_base:			misc peripheral base address
 * @freq_base:			freq peripheral base address
 * @chg_done:			indicates that charging is completed
 * @usb_present:		present status of usb
 * @dc_present:			present status of dc
 * @use_default_batt_values:	flag to report default battery properties
 * @max_voltage_mv:		the max volts the batt should be charged up to
 * @min_voltage_mv:		min battery voltage before turning the FET on
 * @resume_voltage_mv:		voltage at which the battery resumes charging
 * @term_current:		the charging based term current
 * @safe_current:		battery safety current setting
 * @maxinput_usb_ma:		Maximum Input current USB
 * @maxinput_dc_ma:		Maximum Input current DC
 * @revision:			PMIC revision
 * @thermal_levels		amount of thermal mitigation levels
 * @thermal_mitigation		thermal mitigation level values
 * @therm_lvl_sel		thermal mitigation level selection
 * @dc_psy			power supply to export information to userspace
 * @usb_psy			power supply to export information to userspace
 * @bms_psy			power supply to export information to userspace
 * @batt_psy:			power supply to export information to userspace
 * @flags:			flags to activate specific workarounds
 *				throughout the driver
 *
 */
struct qpnp_chg_chip {
	struct device			*dev;
	struct spmi_device		*spmi;
	u16				chgr_base;
	u16				buck_base;
	u16				bat_if_base;
	u16				usb_chgpth_base;
	u16				dc_chgpth_base;
	u16				boost_base;
	u16				misc_base;
	u16				freq_base;
	unsigned int			usbin_valid_irq;
#ifndef CONFIG_LGE_PM
	unsigned int			dcin_valid_irq;
#endif
	unsigned int			chg_done_irq;
	unsigned int			chg_failed_irq;
	bool				chg_done;
	bool				usb_present;
	bool				dc_present;
	bool				charging_disabled;
	bool				use_default_batt_values;
	unsigned int			max_bat_chg_current;
	unsigned int			safe_voltage_mv;
	unsigned int			max_voltage_mv;
	unsigned int			min_voltage_mv;
	unsigned int			resume_voltage_mv;
	unsigned int			term_current;
	unsigned int			maxinput_usb_ma;
	unsigned int			maxinput_dc_ma;
	unsigned int			safe_current;
	unsigned int			revision;
	unsigned int			thermal_levels;
	unsigned int			therm_lvl_sel;
	unsigned int			*thermal_mitigation;
	struct power_supply		dc_psy;
	struct power_supply		*usb_psy;
	struct power_supply		*bms_psy;
	struct power_supply		batt_psy;
	uint32_t			flags;
/*                                                             */
#ifdef CONFIG_LGE_PM
	unsigned int                    ac_online;
	unsigned int                    current_max;
#endif
/*                                        */
	struct qpnp_vadc_chip       *vadc_dev;
};

/*                                                                                   */
#ifdef CONFIG_LGE_PM
static unsigned int cur_max_user;
module_param(cur_max_user, uint, S_IRUGO | S_IWUSR);
#endif
/*                                     */

/*                                                                      */
#ifdef CONFIG_LGE_PM
#define LT_CABLE_56K                6
#define LT_CABLE_130K               7
#define LT_CABLE_910K		    	11
#endif
/*                                        */

static struct of_device_id qpnp_charger_match_table[] = {
	{ .compatible = QPNP_CHARGER_DEV_NAME, },
	{}
};

/*                                                                      */
#ifdef CONFIG_LGE_PM
static struct qpnp_chg_chip *qpnp_chg;
static unsigned int cable_type;
static bool is_factory_cable(void)
{
	unsigned int cable_info;
	cable_info = lge_pm_get_cable_type();

	if ((cable_type == CABLE_56K ||
		cable_type == CABLE_130K ||
		cable_type == CABLE_910K)||
		(cable_info == LT_CABLE_56K ||
		cable_info == LT_CABLE_130K ||
		cable_info == LT_CABLE_910K))
		return 1;
	else
		return 0;
}
/*                                        */

int32_t qpnp_charger_is_ready(void)
{
	struct qpnp_chg_chip *chg = qpnp_chg;

	if (!chg)
		return -EPROBE_DEFER;
	return 0;
}
EXPORT_SYMBOL(qpnp_charger_is_ready);
#endif

static int
qpnp_chg_read(struct qpnp_chg_chip *chip, u8 *val,
			u16 base, int count)
{
	int rc;
	struct spmi_device *spmi = chip->spmi;

	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, base, val,
					count);
	if (rc) {
		pr_err("SPMI read failed base=0x%02x sid=0x%02x rc=%d\n", base,
				spmi->sid, rc);
		return rc;
	}
	return 0;
}

static int
qpnp_chg_write(struct qpnp_chg_chip *chip, u8 *val,
			u16 base, int count)
{
	int rc;
	struct spmi_device *spmi = chip->spmi;

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, base, val,
					count);
	if (rc) {
		pr_err("write failed base=0x%02x sid=0x%02x rc=%d\n",
			base, spmi->sid, rc);
		return rc;
	}

	return 0;
}

static int
qpnp_chg_masked_write(struct qpnp_chg_chip *chip, u16 base,
						u8 mask, u8 val, int count)
{
	int rc;
	u8 reg;

	rc = qpnp_chg_read(chip, &reg, base, count);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n", base, rc);
		return rc;
	}
	pr_debug("addr = 0x%x read 0x%x\n", base, reg);

	reg &= ~mask;
	reg |= val & mask;

	pr_debug("Writing 0x%x\n", reg);

	rc = qpnp_chg_write(chip, &reg, base, count);
	if (rc) {
		pr_err("spmi write failed: addr=%03X, rc=%d\n", base, rc);
		return rc;
	}

	return 0;
}

#define USB_OTG_EN_BIT	BIT(0)
static int
qpnp_chg_is_otg_en_set(struct qpnp_chg_chip *chip)
{
	u8 usb_otg_en;
	int rc;

	rc = qpnp_chg_read(chip, &usb_otg_en,
				 chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
				 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + CHGR_STATUS, rc);
		return rc;
	}
	pr_debug("usb otg en 0x%x\n", usb_otg_en);

	return (usb_otg_en & USB_OTG_EN_BIT) ? 1 : 0;
}

#define USB_VALID_BIT	BIT(7)
static int
qpnp_chg_is_usb_chg_plugged_in(struct qpnp_chg_chip *chip)
{
	u8 usbin_valid_rt_sts;
	int rc;

	rc = qpnp_chg_read(chip, &usbin_valid_rt_sts,
				 chip->usb_chgpth_base + CHGR_STATUS , 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + CHGR_STATUS, rc);
		return rc;
	}
	pr_debug("chgr usb sts 0x%x\n", usbin_valid_rt_sts);

	return (usbin_valid_rt_sts & USB_VALID_BIT) ? 1 : 0;
}

#ifndef CONFIG_LGE_PM
static int
qpnp_chg_is_dc_chg_plugged_in(struct qpnp_chg_chip *chip)
{
	u8 dcin_valid_rt_sts;
	int rc;

	if (!chip->dc_chgpth_base)
		return 0;

	rc = qpnp_chg_read(chip, &dcin_valid_rt_sts,
				 INT_RT_STS(chip->dc_chgpth_base), 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->dc_chgpth_base), rc);
		return rc;
	}

	return (dcin_valid_rt_sts & DCIN_VALID_IRQ) ? 1 : 0;
}
#endif

#define QPNP_CHG_I_MAX_MIN_100		100
#define QPNP_CHG_I_MAX_MIN_150		150
#define QPNP_CHG_I_MAX_MIN_MA		200
#define QPNP_CHG_I_MAX_MAX_MA		2500
#define QPNP_CHG_I_MAXSTEP_MA		100
static int
qpnp_chg_idcmax_set(struct qpnp_chg_chip *chip, int mA)
{
	int rc = 0;
	u8 dc = 0;

	if (mA < QPNP_CHG_I_MAX_MIN_100
			|| mA > QPNP_CHG_I_MAX_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", mA);
		return -EINVAL;
	}

	if (mA == QPNP_CHG_I_MAX_MIN_100) {
		dc = 0x00;
		pr_debug("current=%d setting %02x\n", mA, dc);
		return qpnp_chg_write(chip, &dc,
			chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);
	} else if (mA == QPNP_CHG_I_MAX_MIN_150) {
		dc = 0x01;
		pr_debug("current=%d setting %02x\n", mA, dc);
		return qpnp_chg_write(chip, &dc,
			chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);
	}

	dc = mA / QPNP_CHG_I_MAXSTEP_MA;

	pr_debug("current=%d setting 0x%x\n", mA, dc);
	rc = qpnp_chg_write(chip, &dc,
		chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);

	return rc;
}

static int
qpnp_chg_iusbmax_set(struct qpnp_chg_chip *chip, int mA)
{
	int rc = 0;
	u8 usb_reg = 0, temp = 8;

	if (mA < QPNP_CHG_I_MAX_MIN_100
			|| mA > QPNP_CHG_I_MAX_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", mA);
		return -EINVAL;
	}

	if (mA == QPNP_CHG_I_MAX_MIN_100) {
		usb_reg = 0x00;
		pr_debug("current=%d setting %02x\n", mA, usb_reg);
		return qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	} else if (mA == QPNP_CHG_I_MAX_MIN_150) {
		usb_reg = 0x01;
		pr_debug("current=%d setting %02x\n", mA, usb_reg);
		return qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	}

	/* Impose input current limit */
	if (chip->maxinput_usb_ma)
		mA = (chip->maxinput_usb_ma) <= mA ? chip->maxinput_usb_ma : mA;

	usb_reg = mA / QPNP_CHG_I_MAXSTEP_MA;

	if (chip->flags & CHG_FLAGS_VCP_WA) {
		temp = 0xA5;
		rc =  qpnp_chg_write(chip, &temp,
			chip->buck_base + SEC_ACCESS, 1);
		rc =  qpnp_chg_masked_write(chip,
			chip->buck_base + CHGR_BUCK_COMPARATOR_OVRIDE_3,
			0x0C, 0x0C, 1);
	}

	pr_debug("current=%d setting 0x%x\n", mA, usb_reg);
	rc = qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);

	if (chip->flags & CHG_FLAGS_VCP_WA) {
		temp = 0xA5;
		udelay(200);
		rc =  qpnp_chg_write(chip, &temp,
			chip->buck_base + SEC_ACCESS, 1);
		rc =  qpnp_chg_masked_write(chip,
			chip->buck_base + CHGR_BUCK_COMPARATOR_OVRIDE_3,
			0x0C, 0x00, 1);
	}

	return rc;
}

#define USB_SUSPEND_BIT	BIT(0)
static int
qpnp_chg_usb_suspend_enable(struct qpnp_chg_chip *chip, int enable)
{
	return qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_SUSP,
			USB_SUSPEND_BIT,
			enable ? USB_SUSPEND_BIT : 0, 1);
}

#define ENUM_T_STOP_BIT		BIT(0)
static irqreturn_t
qpnp_chg_usb_usbin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int usb_present, host_mode;

	usb_present = qpnp_chg_is_usb_chg_plugged_in(chip);
	host_mode = qpnp_chg_is_otg_en_set(chip);
	pr_debug("usbin-valid triggered: %d host_mode: %d\n",
		usb_present, host_mode);

	/* In host mode notifications cmoe from USB supply */
	if (host_mode)
		return IRQ_HANDLED;

	if (chip->usb_present ^ usb_present) {
		chip->usb_present = usb_present;
		power_supply_set_present(chip->usb_psy,
			chip->usb_present);
	}

	return IRQ_HANDLED;
}

#ifndef CONFIG_LGE_PM
static irqreturn_t
qpnp_chg_dc_dcin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int dc_present;

	dc_present = qpnp_chg_is_dc_chg_plugged_in(chip);
	pr_debug("dcin-valid triggered: %d\n", dc_present);

	if (chip->dc_present ^ dc_present) {
		chip->dc_present = dc_present;
		power_supply_changed(&chip->dc_psy);
	}

	return IRQ_HANDLED;
}
#endif

#define CHGR_CHG_FAILED_BIT	BIT(7)
static irqreturn_t
qpnp_chg_chgr_chg_failed_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int rc;

	rc = qpnp_chg_masked_write(chip,
		chip->chgr_base + CHGR_CHG_FAILED,
		CHGR_CHG_FAILED_BIT,
		CHGR_CHG_FAILED_BIT, 1);
	if (rc)
		pr_err("Failed to write chg_fail clear bit!\n");

	return IRQ_HANDLED;
}

static irqreturn_t
qpnp_chg_chgr_chg_done_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;

	pr_debug("CHG_DONE IRQ triggered\n");
	chip->chg_done = true;

	return IRQ_HANDLED;
}

static int
qpnp_batt_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		return 1;
	default:
		break;
	}

	return 0;
}

static int
qpnp_chg_charge_en(struct qpnp_chg_chip *chip, int enable)
{
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_CHG_CTRL,
			CHGR_CHG_EN,
			enable ? CHGR_CHG_EN : 0, 1);
}

static int
qpnp_chg_force_run_on_batt(struct qpnp_chg_chip *chip, int disable)
{
	/* Don't run on battery for batteryless hardware */
	if (chip->use_default_batt_values)
		return 0;

	/* This bit forces the charger to run off of the battery rather
	 * than a connected charger */
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_CHG_CTRL,
			CHGR_ON_BAT_FORCE_BIT,
			disable ? CHGR_ON_BAT_FORCE_BIT : 0, 1);
}

static int
qpnp_chg_buck_control(struct qpnp_chg_chip *chip, int enable)
{
	int rc;

	if (chip->charging_disabled && enable) {
		pr_debug("Charging disabled\n");
		return 0;
	}

	rc = qpnp_chg_charge_en(chip, enable);
	if (rc) {
		pr_err("Failed to control charging %d\n", rc);
		return rc;
	}

	rc = qpnp_chg_force_run_on_batt(chip, !enable);
	if (rc)
		pr_err("Failed to control charging %d\n", rc);

	return rc;
}

static
int switch_usb_to_charge_mode(struct qpnp_chg_chip *chip)
{
	int rc;

	pr_debug("switch to charge mode\n");
	if (!qpnp_chg_is_otg_en_set(chip))
		return 0;

	/* enable usb ovp fet */
	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
			USB_OTG_EN_BIT,
			0, 1);
	if (rc) {
		pr_err("Failed to turn on usb ovp rc = %d\n", rc);
		return rc;
	}

	rc = qpnp_chg_force_run_on_batt(chip, chip->charging_disabled);
	if (rc) {
		pr_err("Failed re-enable charging rc = %d\n", rc);
		return rc;
	}

	return 0;
}

static
int switch_usb_to_host_mode(struct qpnp_chg_chip *chip)
{
	int rc;

	pr_debug("switch to host mode\n");
	if (qpnp_chg_is_otg_en_set(chip))
		return 0;

	rc = qpnp_chg_force_run_on_batt(chip, 1);
	if (rc) {
		pr_err("Failed to disable charging rc = %d\n", rc);
		return rc;
	}

	/* force usb ovp fet off */
	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
			USB_OTG_EN_BIT,
			USB_OTG_EN_BIT, 1);
	if (rc) {
		pr_err("Failed to turn off usb ovp rc = %d\n", rc);
		return rc;
	}

	return 0;
}

static enum power_supply_property pm_power_props_mains[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
#ifdef CONFIG_LGE_PM
	POWER_SUPPLY_PROP_CURRENT_MAX,
#endif
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL,
};

static char *pm_power_supplied_to[] = {
	"battery",
};

#define USB_WALL_THRESHOLD_MA	500
static int
qpnp_power_get_property_mains(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								dc_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		if (chip->charging_disabled)
			return 0;
#ifdef CONFIG_LGE_PM
		val->intval = chip->ac_online;
#else
		val->intval = qpnp_chg_is_dc_chg_plugged_in(chip);
#endif
		break;
#ifdef CONFIG_LGE_PM
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = chip->current_max;
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

#ifdef CONFIG_LGE_PM
static int
qpnp_power_set_property_mains(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								dc_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		chip->ac_online = val->intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		/*                                                                                 */
		if (cur_max_user > 0)
			chip->current_max = cur_max_user * 1000;
		else
			chip->current_max = val->intval;
		break;
	default:
		return -EINVAL;
	}

	power_supply_changed(&chip->dc_psy);
	return 0;
}
#endif

#ifndef CONFIG_LGE_PM
static int
get_prop_battery_voltage_now(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	if (chip->revision > 0) {
		rc = qpnp_vadc_read(chip->vadc_dev, VBAT_SNS, &results);
		if (rc) {
			pr_err("Unable to read vbat rc=%d\n", rc);
			return 0;
		}
		return results.physical;
	} else {
		pr_err("vbat reading not supported for 1.0 rc=%d\n", rc);
		return 0;
	}
}
/* QCT origin get_prop_battery_voltage_now */
#else
static int
get_prop_battery_voltage_now_bms(struct qpnp_chg_chip *chip)
{
#ifdef CONFIG_QPNP_BMS
	int rc = 0;
	struct qpnp_vadc_result results;

	if (chip->revision > 0) {
		rc = qpnp_vadc_read(chip->vadc_dev, VBAT_SNS, &results);
		if (rc) {
			pr_err("Unable to read vbat rc=%d\n", rc);
			return 0;
		}
		return results.physical;
	} else {
		pr_err("vbat reading not supported for 1.0 rc=%d\n", rc);
		return 0;
	}
#else
	pr_err("NOT INIT BMS, Cannot get voltage now.\n");
	return 4000;
#endif
}

static int
get_prop_battery_voltage_now_max17048(struct qpnp_chg_chip *chip)
{

#ifdef CONFIG_MAX17048_FUELGAUGE
	int voltage = 0;
	voltage = max17048_get_voltage();
	return voltage;
#else
	pr_err("NOT INIT max17048, Cannot get voltage now.\n");
	return 4000;
#endif
}
#endif

#define BATT_PRES_BIT BIT(7)
static int
get_prop_batt_present(struct qpnp_chg_chip *chip)
{
	u8 batt_present;
	int rc;

	rc = qpnp_chg_read(chip, &batt_present,
				chip->bat_if_base + CHGR_BAT_IF_PRES_STATUS, 1);
	if (rc) {
		pr_err("Couldn't read battery status read failed rc=%d\n", rc);
		return 0;
	};
	return (batt_present & BATT_PRES_BIT) ? 1 : 0;
}

#define BATT_TEMP_HOT	BIT(6)
#define BATT_TEMP_OK	BIT(7)
static int
get_prop_batt_health(struct qpnp_chg_chip *chip)
{
	u8 batt_health;
	int rc;

	rc = qpnp_chg_read(chip, &batt_health,
				chip->bat_if_base + CHGR_STATUS, 1);
	if (rc) {
		pr_err("Couldn't read battery health read failed rc=%d\n", rc);
		return POWER_SUPPLY_HEALTH_UNKNOWN;
	};

	if (BATT_TEMP_OK & batt_health)
		return POWER_SUPPLY_HEALTH_GOOD;
	if (BATT_TEMP_HOT & batt_health)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else
		return POWER_SUPPLY_HEALTH_COLD;
}

static int
get_prop_charge_type(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 chgr_sts;

	if (!get_prop_batt_present(chip))
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

	rc = qpnp_chg_read(chip, &chgr_sts,
				INT_RT_STS(chip->chgr_base), 1);
	if (rc) {
		pr_err("failed to read interrupt sts %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	if (chgr_sts & TRKL_CHG_ON_IRQ)
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	if (chgr_sts & FAST_CHG_ON_IRQ)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

static int
get_prop_batt_status(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 chgr_sts;

	rc = qpnp_chg_read(chip, &chgr_sts,
				INT_RT_STS(chip->chgr_base), 1);
	if (rc) {
		pr_err("failed to read interrupt sts %d\n", rc);
		return POWER_SUPPLY_STATUS_DISCHARGING;
	}

	pr_debug("chgr sts 0x%x\n", chgr_sts);
	if (chgr_sts & CHG_DONE_IRQ || chip->chg_done)
		return POWER_SUPPLY_STATUS_FULL;
	else
		chip->chg_done = false;

	if (chgr_sts & TRKL_CHG_ON_IRQ)
		return POWER_SUPPLY_STATUS_CHARGING;
	if (chgr_sts & FAST_CHG_ON_IRQ)
		return POWER_SUPPLY_STATUS_CHARGING;

	return POWER_SUPPLY_STATUS_DISCHARGING;
}

static int
get_prop_current_now(struct qpnp_chg_chip *chip)
{
#ifdef CONFIG_QPNP_BMS
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_CURRENT_NOW, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
#else
	pr_err("NOT INIT BMS, Cannot get current now.\n");
	return 199;
#endif
}

static int
get_prop_full_design(struct qpnp_chg_chip *chip)
{
#ifdef CONFIG_QPNP_BMS
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
#else
	pr_err("NOT INIT BMS, Cannot get full design.\n");
	return 2540;
#endif
}

#define DEFAULT_CAPACITY	50

#ifndef CONFIG_LGE_PM
static int
get_prop_capacity(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->use_default_batt_values || !get_prop_batt_present(chip))
		return DEFAULT_CAPACITY;

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_CAPACITY, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 50\n");
	}

	/* return default capacity to avoid userspace
	 * from shutting down unecessarily */
	return DEFAULT_CAPACITY;
}
/* QCT origin get_prop_capacity */
#else
static int
get_prop_capacity_bms(struct qpnp_chg_chip *chip)
{
#ifdef CONFIG_QPNP_BMS
	union power_supply_propval ret = {0,};

	if (chip->use_default_batt_values || !get_prop_batt_present(chip))
		return DEFAULT_CAPACITY;
	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_CAPACITY, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 50\n");
	}

	/* return default capacity to avoid userspace
	 * from shutting down unecessarily */
	return DEFAULT_CAPACITY;
#else
	pr_err("NOT INIT BMS, Cannot get capacity.\n");
	return 51;
#endif
}

static int
get_prop_capacity_max17048(struct qpnp_chg_chip *chip)
{
#ifdef CONFIG_MAX17048_FUELGAUGE
	int capacity = 0;
	capacity = max17048_get_capacity();
	return capacity;
#else
	pr_err("NOT INIT max17048, Cannot get capacity.\n");
	return 51;
#endif
}
#endif

#define DEFAULT_TEMP		250
#define MAX_TOLERABLE_BATT_TEMP_DDC	680
static int
get_prop_batt_temp(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	if (chip->use_default_batt_values || !get_prop_batt_present(chip))
		return DEFAULT_TEMP;

	if (chip->revision > 0) {
		rc = qpnp_vadc_read(chip->vadc_dev, LR_MUX1_BATT_THERM, &results);
		if (rc) {
			pr_debug("Unable to read batt temperature rc=%d\n", rc);
			return 0;
		}
		pr_debug("get_bat_temp %d %lld\n",
			results.adc_code, results.physical);
		return (int)results.physical;
	} else {
		pr_debug("batt temp not supported for PMIC 1.0 rc=%d\n", rc);
	}
	/* return default temperature to avoid userspace
	 * from shutting down unecessarily */
	return DEFAULT_TEMP;
}


#ifdef CONFIG_LGE_PM
typedef enum{
	MAX17048_TYPE,
	BMS_TYPE,
	GAUGE_IC_TYPE_MAX
}gauge_ic_type;

int (*get_prop_capacity)(struct qpnp_chg_chip *chip);
int (*get_prop_battery_voltage_now)(struct qpnp_chg_chip *chip);

struct	gauge_ic_func {
	int 	(*get_prop_cap_func)(struct qpnp_chg_chip *chip);
	int 	(*get_prop_battery_vol_func)(struct qpnp_chg_chip *chip);
};

static struct gauge_ic_func gauge_ic_func_array[GAUGE_IC_TYPE_MAX] = {
	{
		.get_prop_cap_func = get_prop_capacity_max17048,
		.get_prop_battery_vol_func = get_prop_battery_voltage_now_max17048,
	},
	{
		.get_prop_cap_func = get_prop_capacity_bms,
		.get_prop_battery_vol_func = get_prop_battery_voltage_now_bms,
	},
};
#endif

static void
qpnp_batt_external_power_changed(struct power_supply *psy)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);
	union power_supply_propval ret = {0,};
#ifdef CONFIG_QPNP_BMS
	if (!chip->bms_psy)
		chip->bms_psy = power_supply_get_by_name("bms");
#endif
	chip->usb_psy->get_property(chip->usb_psy,
			  POWER_SUPPLY_PROP_SCOPE, &ret);
	if (ret.intval) {
		if ((ret.intval == POWER_SUPPLY_SCOPE_SYSTEM)
				&& !qpnp_chg_is_otg_en_set(chip)) {
			switch_usb_to_host_mode(chip);
			return;
		}
		if ((ret.intval == POWER_SUPPLY_SCOPE_DEVICE)
				&& qpnp_chg_is_otg_en_set(chip)) {
			switch_usb_to_charge_mode(chip);
			return;
		}
	}

/*                                                                                       */
#ifdef CONFIG_LGE_PM
	if(is_factory_cable()){
		qpnp_chg_iusbmax_set(chip, QPNP_CHG_I_MAX_MAX_MA);
		qpnp_chg_usb_suspend_enable(chip, 0);
		power_supply_changed(&chip->batt_psy);
		return;
	}
#endif
/*                                        */

	chip->usb_psy->get_property(chip->usb_psy,
			  POWER_SUPPLY_PROP_ONLINE, &ret);

	if (ret.intval && qpnp_chg_is_usb_chg_plugged_in(chip)) {
		chip->usb_psy->get_property(chip->usb_psy,
			  POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
		qpnp_chg_iusbmax_set(chip, ret.intval / 1000);
		if ((ret.intval / 1000) <= QPNP_CHG_I_MAX_MIN_MA)
			qpnp_chg_usb_suspend_enable(chip, 1);
		else
			qpnp_chg_usb_suspend_enable(chip, 0);
/*                                                                 */
#ifdef CONFIG_LGE_PM
	} else if (chip->ac_online && qpnp_chg_is_usb_chg_plugged_in(chip)) {
		qpnp_chg_iusbmax_set(chip, chip->current_max / 1000);
		if ((chip->current_max / 1000) <= QPNP_CHG_I_MAX_MIN_MA)
			qpnp_chg_usb_suspend_enable(chip, 1);
		else
			qpnp_chg_usb_suspend_enable(chip, 0);
#endif
/*                                        */
	} else {
		qpnp_chg_iusbmax_set(chip, QPNP_CHG_I_MAX_MIN_100);
		qpnp_chg_usb_suspend_enable(chip, 0);
	}

	pr_debug("end of power supply changed\n");
	power_supply_changed(&chip->batt_psy);
}

static int
qpnp_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:					/*From chg*/
		val->intval = get_prop_batt_status(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:				/*From chg*/
		val->intval = get_prop_charge_type(chip);
		break;
	case POWER_SUPPLY_PROP_HEALTH:					/*From chg*/
		val->intval = get_prop_batt_health(chip);
		break;
	case POWER_SUPPLY_PROP_PRESENT:					/*From chg*/
		val->intval = get_prop_batt_present(chip);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:			/*From dts file*/
		val->intval = chip->max_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:			/*From dts file*/
		val->intval = chip->min_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:				/*From gauge ic*/
		val->intval = get_prop_battery_voltage_now(chip);
		break;
	case POWER_SUPPLY_PROP_TEMP:					/*From chg*/
		val->intval = get_prop_batt_temp(chip);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:				/*From gauge ic*/
		val->intval = get_prop_capacity(chip);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:				/*From bms only*/
		val->intval = get_prop_current_now(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:			/*From bms only*/
		val->intval = get_prop_full_design(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !(chip->charging_disabled);
		break;
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		val->intval = chip->therm_lvl_sel;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define QPNP_CHG_VINMIN_MIN_MV		3400
#define QPNP_CHG_VINMIN_HIGH_MIN_MV	5600
#define QPNP_CHG_VINMIN_HIGH_MIN_VAL	0x2B
#define QPNP_CHG_VINMIN_MAX_MV		9600
#define QPNP_CHG_VINMIN_STEP_MV		50
#define QPNP_CHG_VINMIN_STEP_HIGH_MV	200
#define QPNP_CHG_VINMIN_MASK		0x1F
static int
qpnp_chg_vinmin_set(struct qpnp_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < QPNP_CHG_VINMIN_MIN_MV
			|| voltage > QPNP_CHG_VINMIN_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	if (voltage >= QPNP_CHG_VINMIN_HIGH_MIN_MV) {
		temp = QPNP_CHG_VINMIN_HIGH_MIN_VAL;
		temp += (voltage - QPNP_CHG_VINMIN_MIN_MV)
			/ QPNP_CHG_VINMIN_STEP_HIGH_MV;
	} else {
		temp = (voltage - QPNP_CHG_VINMIN_MIN_MV)
			/ QPNP_CHG_VINMIN_STEP_MV;
	}

	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_VIN_MIN,
			QPNP_CHG_VINMIN_MASK, temp, 1);
}

#define QPNP_CHG_IBATSAFE_MIN_MA		100
#define QPNP_CHG_IBATSAFE_MAX_MA		3250
#define QPNP_CHG_I_STEP_MA		50
#define QPNP_CHG_I_MIN_MA		100
#define QPNP_CHG_I_MASK			0x3F
static int
qpnp_chg_ibatsafe_set(struct qpnp_chg_chip *chip, int safe_current)
{
	u8 temp;

	if (safe_current < QPNP_CHG_IBATSAFE_MIN_MA
			|| safe_current > QPNP_CHG_IBATSAFE_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", safe_current);
		return -EINVAL;
	}

	temp = (safe_current - QPNP_CHG_IBATSAFE_MIN_MA)
				/ QPNP_CHG_I_STEP_MA;
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_IBAT_SAFE,
			QPNP_CHG_I_MASK, temp, 1);
}

#define QPNP_CHG_ITERM_MIN_MA		100
#define QPNP_CHG_ITERM_MAX_MA		250
#define QPNP_CHG_ITERM_STEP_MA		50
#define QPNP_CHG_ITERM_MASK			0x03
static int
qpnp_chg_ibatterm_set(struct qpnp_chg_chip *chip, int term_current)
{
	u8 temp;

	if (term_current < QPNP_CHG_ITERM_MIN_MA
			|| term_current > QPNP_CHG_ITERM_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", term_current);
		return -EINVAL;
	}

	temp = (term_current - QPNP_CHG_ITERM_MIN_MA)
				/ QPNP_CHG_ITERM_STEP_MA;
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_IBAT_TERM_CHGR,
			QPNP_CHG_ITERM_MASK, temp, 1);
}

#define QPNP_CHG_IBATMAX_MIN	100
#define QPNP_CHG_IBATMAX_MAX	3250
static int
qpnp_chg_ibatmax_set(struct qpnp_chg_chip *chip, int chg_current)
{
	u8 temp;

	if (chg_current < QPNP_CHG_IBATMAX_MIN
			|| chg_current > QPNP_CHG_IBATMAX_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = (chg_current - QPNP_CHG_I_MIN_MA) / QPNP_CHG_I_STEP_MA;
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_IBAT_MAX,
			QPNP_CHG_I_MASK, temp, 1);
}

#define QPNP_CHG_VBATDET_MIN_MV	3240
#define QPNP_CHG_VBATDET_MAX_MV	5780
#define QPNP_CHG_VBATDET_STEP_MV	20
static int
qpnp_chg_vbatdet_set(struct qpnp_chg_chip *chip, int vbatdet_mv)
{
	u8 temp;

	if (vbatdet_mv < QPNP_CHG_VBATDET_MIN_MV
			|| vbatdet_mv > QPNP_CHG_VBATDET_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", vbatdet_mv);
		return -EINVAL;
	}
	temp = (vbatdet_mv - QPNP_CHG_VBATDET_MIN_MV)
			/ QPNP_CHG_VBATDET_STEP_MV;

	pr_debug("voltage=%d setting %02x\n", vbatdet_mv, temp);
	return qpnp_chg_write(chip, &temp,
		chip->chgr_base + CHGR_VBAT_DET, 1);
}

#define QPNP_CHG_V_MIN_MV	3240
#define QPNP_CHG_V_MAX_MV	4500
#define QPNP_CHG_V_STEP_MV	10
static int
qpnp_chg_vddsafe_set(struct qpnp_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < QPNP_CHG_V_MIN_MV
			|| voltage > QPNP_CHG_V_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - QPNP_CHG_V_MIN_MV) / QPNP_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return qpnp_chg_write(chip, &temp,
		chip->chgr_base + CHGR_VDD_SAFE, 1);
}

#define QPNP_CHG_VDDMAX_MIN	3400
static int
qpnp_chg_vddmax_set(struct qpnp_chg_chip *chip, int voltage)
{
	u8 temp = 0;

	if (voltage < QPNP_CHG_VDDMAX_MIN
			|| voltage > QPNP_CHG_V_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	temp = (voltage - QPNP_CHG_V_MIN_MV) / QPNP_CHG_V_STEP_MV;

	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return qpnp_chg_write(chip, &temp,
		chip->chgr_base + CHGR_VDD_MAX, 1);
}

static void
qpnp_chg_set_appropriate_battery_current(struct qpnp_chg_chip *chip)
{
	unsigned int chg_current = chip->max_bat_chg_current;

	if (chip->therm_lvl_sel != 0 && chip->thermal_mitigation)
		chg_current = min(chg_current,
			chip->thermal_mitigation[chip->therm_lvl_sel]);

	pr_debug("setting %d mA\n", chg_current);
	qpnp_chg_ibatmax_set(chip, chg_current);
}

static void
qpnp_batt_system_temp_level_set(struct qpnp_chg_chip *chip, int lvl_sel)
{
	if (lvl_sel >= 0 && lvl_sel < chip->thermal_levels) {
		chip->therm_lvl_sel = lvl_sel;
		if (lvl_sel == (chip->thermal_levels - 1)) {
			/* disable charging if highest value selected */
			qpnp_chg_buck_control(chip, 0);
		} else {
			qpnp_chg_buck_control(chip, 1);
			qpnp_chg_set_appropriate_battery_current(chip);
		}
	} else {
		pr_err("Unsupported level selected %d\n", lvl_sel);
	}
}

static int
qpnp_batt_power_set_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		chip->charging_disabled = !(val->intval);
		qpnp_chg_charge_en(chip, !chip->charging_disabled);
		qpnp_chg_force_run_on_batt(chip, chip->charging_disabled);
		break;
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		qpnp_batt_system_temp_level_set(chip, val->intval);
		break;
	default:
		return -EINVAL;
	}

	power_supply_changed(&chip->batt_psy);
	return 0;
}

static void
qpnp_chg_setup_flags(struct qpnp_chg_chip *chip)
{
	if (chip->revision > 0)
		chip->flags |= CHG_FLAGS_VCP_WA;
}

#define WDOG_EN_BIT	BIT(7)
static int
qpnp_chg_hwinit(struct qpnp_chg_chip *chip, u8 subtype,
				struct spmi_resource *spmi_resource)
{
	int rc = 0;
	u8 reg;

	switch (subtype) {
	case SMBB_CHGR_SUBTYPE:
	case SMBBP_CHGR_SUBTYPE:
		chip->chg_done_irq = spmi_get_irq_byname(chip->spmi,
						spmi_resource, "chg-done");
		if (chip->chg_done_irq < 0) {
			pr_err("Unable to get chg_done irq\n");
			return -ENXIO;
		}

		chip->chg_failed_irq = spmi_get_irq_byname(chip->spmi,
						spmi_resource, "chg-failed");
		if (chip->chg_failed_irq < 0) {
			pr_err("Unable to get chg_failed irq\n");
			return -ENXIO;
		}
		rc |= devm_request_irq(chip->dev, chip->chg_done_irq,
				qpnp_chg_chgr_chg_done_irq_handler,
				IRQF_TRIGGER_RISING, "chg_done", chip);
		if (rc < 0) {
			pr_err("Can't request %d chg_done for chg: %d\n",
						chip->chg_done_irq, rc);
			return -ENXIO;
		}
		rc |= devm_request_irq(chip->dev, chip->chg_failed_irq,
				qpnp_chg_chgr_chg_failed_irq_handler,
				IRQF_TRIGGER_RISING, "chg_failed", chip);
		if (rc < 0) {
			pr_err("Can't request %d chg_failed chg: %d\n",
						chip->chg_failed_irq, rc);
			return -ENXIO;
		}

		rc = qpnp_chg_vinmin_set(chip, chip->min_voltage_mv);
		if (rc) {
			pr_debug("failed setting  min_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_vddmax_set(chip, chip->max_voltage_mv);
		if (rc) {
			pr_debug("failed setting max_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_vddsafe_set(chip, chip->safe_voltage_mv);
		if (rc) {
			pr_debug("failed setting safe_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_vbatdet_set(chip, chip->resume_voltage_mv);
		if (rc) {
			pr_debug("failed setting resume_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_ibatmax_set(chip, chip->max_bat_chg_current);
		if (rc) {
			pr_debug("failed setting ibatmax rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_ibatterm_set(chip, chip->term_current);
		if (rc) {
			pr_debug("failed setting ibatterm rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_ibatsafe_set(chip, chip->safe_current);
		if (rc) {
			pr_debug("failed setting ibat_Safe rc=%d\n", rc);
			return rc;
		}
		/* HACK: Disable wdog */
		rc = qpnp_chg_masked_write(chip, chip->chgr_base + 0x62,
			0xFF, 0xA0, 1);

		/* HACK: use analog EOC */
		rc = qpnp_chg_masked_write(chip, chip->chgr_base +
			CHGR_IBAT_TERM_CHGR,
			0x80, 0x80, 1);

		enable_irq_wake(chip->chg_done_irq);
		break;
	case SMBB_BUCK_SUBTYPE:
	case SMBBP_BUCK_SUBTYPE:
		rc = qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_BUCK_BCK_VBAT_REG_MODE,
			BUCK_VBAT_REG_NODE_SEL_BIT,
			BUCK_VBAT_REG_NODE_SEL_BIT, 1);
		if (rc) {
			pr_debug("failed to enable IR drop comp rc=%d\n", rc);
			return rc;
		}
		break;
	case SMBB_BAT_IF_SUBTYPE:
	case SMBBP_BAT_IF_SUBTYPE:
		break;
	case SMBB_USB_CHGPTH_SUBTYPE:
	case SMBBP_USB_CHGPTH_SUBTYPE:
		chip->usbin_valid_irq = spmi_get_irq_byname(chip->spmi,
						spmi_resource, "usbin-valid");
		if (chip->usbin_valid_irq < 0) {
			pr_err("Unable to get usbin irq\n");
			return -ENXIO;
		}
		rc = devm_request_irq(chip->dev, chip->usbin_valid_irq,
				qpnp_chg_usb_usbin_valid_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"chg_usbin_valid", chip);
		if (rc < 0) {
			pr_err("Can't request %d usbinvalid  for chg: %d\n",
						chip->usbin_valid_irq, rc);
			return -ENXIO;
		}

		enable_irq_wake(chip->usbin_valid_irq);
		chip->usb_present = qpnp_chg_is_usb_chg_plugged_in(chip);
		if (chip->usb_present) {
			rc = qpnp_chg_masked_write(chip,
				chip->usb_chgpth_base + CHGR_USB_ENUM_T_STOP,
				ENUM_T_STOP_BIT,
				ENUM_T_STOP_BIT, 1);
			if (rc) {
				pr_err("failed to write enum stop rc=%d\n", rc);
				return -ENXIO;
			}
		}

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + USB_OVP_CTL,
			USB_VALID_DEB_20MS,
			USB_VALID_DEB_20MS, 1);

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_ENUM_T_STOP,
			ENUM_T_STOP_BIT,
			ENUM_T_STOP_BIT, 1);

		break;
	case SMBB_DC_CHGPTH_SUBTYPE:
#ifndef CONFIG_LGE_PM
		chip->dcin_valid_irq = spmi_get_irq_byname(chip->spmi,
						spmi_resource, "dcin-valid");
		if (chip->dcin_valid_irq < 0) {
			pr_err("Unable to get dcin irq\n");
			return -ENXIO;
		}
		rc = devm_request_irq(chip->dev, chip->dcin_valid_irq,
				qpnp_chg_dc_dcin_valid_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"chg_dcin_valid", chip);
		if (rc < 0) {
			pr_err("Can't request %d dcinvalid  for chg: %d\n",
						chip->dcin_valid_irq, rc);
			return -ENXIO;
		}

		enable_irq_wake(chip->dcin_valid_irq);
#endif
		break;
	case SMBB_BOOST_SUBTYPE:
	case SMBBP_BOOST_SUBTYPE:
		break;
	case SMBB_MISC_SUBTYPE:
	case SMBBP_MISC_SUBTYPE:
		pr_debug("Setting BOOT_DONE\n");
		rc = qpnp_chg_masked_write(chip,
			chip->misc_base + CHGR_MISC_BOOT_DONE,
			CHGR_BOOT_DONE, CHGR_BOOT_DONE, 1);
		rc = qpnp_chg_read(chip, &reg,
				 chip->misc_base + MISC_REVISION2, 1);
		if (rc) {
			pr_err("failed to read revision register rc=%d\n", rc);
			return rc;
		}

		chip->revision = reg;
		break;
	default:
		pr_err("Invalid peripheral subtype\n");
	}
	return rc;
}

#ifdef CONFIG_LGE_PM
/*                                               */
static unsigned int cable_smem_size;
unsigned int lge_chg_cable_type(void)
{
	return cable_type;
}
EXPORT_SYMBOL(lge_chg_cable_type);
/*                                               */
#endif

static int __devinit
qpnp_charger_probe(struct spmi_device *spmi)
{
	u8 subtype;
	struct qpnp_chg_chip	*chip;
	struct resource *resource;
	struct spmi_resource *spmi_resource;
	int rc = 0;
#ifdef CONFIG_LGE_PM
	/*                                               */
	unsigned int *p_cable_type = (unsigned int *)
		(smem_get_entry(SMEM_ID_VENDOR1, &cable_smem_size));

	if (p_cable_type)
		cable_type = *p_cable_type;
	else
		cable_type = 0;

	printk(KERN_INFO "cable_type is = %d\n", cable_type);
	/*                                               */
#endif

	chip = kzalloc(sizeof *chip, GFP_KERNEL);
	if (chip == NULL) {
		pr_err("kzalloc() failed.\n");
		return -ENOMEM;
	}

	chip->dev = &(spmi->dev);
	chip->spmi = spmi;

	chip->usb_psy = power_supply_get_by_name("usb");
	if (!chip->usb_psy) {
		pr_err("usb supply not found deferring probe\n");
		rc = -EPROBE_DEFER;
		goto fail_chg_enable;
	}
/*                                                                         */
#ifdef CONFIG_LGE_PM
	get_cable_data_from_dt(spmi->dev.of_node);
#endif
/*                                        */

	/* Get the vddmax property */
	rc = of_property_read_u32(spmi->dev.of_node, "qcom,vddmax-mv",
						&chip->max_voltage_mv);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading vddmax property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the vinmin property */
	rc = of_property_read_u32(spmi->dev.of_node, "qcom,vinmin-mv",
						&chip->min_voltage_mv);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading vddmax property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the vddmax property */
	rc = of_property_read_u32(spmi->dev.of_node, "qcom,vddsafe-mv",
						&chip->safe_voltage_mv);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading vddsave property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the ibatsafe property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,vbatdet-mv",
				&chip->resume_voltage_mv);
	if (rc) {
		pr_err("Error reading vbatdet property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the ibatsafe property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,ibatsafe-ma",
				&chip->safe_current);
	if (rc) {
		pr_err("Error reading ibatsafe property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the ibatterm property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,ibatterm-ma",
				&chip->term_current);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading ibatterm property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the ibatmax property */
	rc = of_property_read_u32(spmi->dev.of_node, "qcom,ibatmax-ma",
						&chip->max_bat_chg_current);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading ibatmax property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the ibatsafe property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,vbatdet-mv",
				&chip->resume_voltage_mv);
	if (rc) {
		pr_err("Error reading vbatdet property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the maxinput-dc-ma property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,maxinput-dc-ma",
				&chip->maxinput_dc_ma);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading maxinput-dc-ma property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the maxinput-usb-ma property */
	rc = of_property_read_u32(spmi->dev.of_node,
				"qcom,maxinput-usb-ma",
				&chip->maxinput_usb_ma);
	if (rc && rc != -EINVAL) {
		pr_err("Error reading maxinput-usb-ma property %d\n", rc);
		goto fail_chg_enable;
	}

	/* Get the charging-disabled property */
	chip->charging_disabled = of_property_read_bool(spmi->dev.of_node,
					"qcom,charging-disabled");

	/* Get the fake-batt-values property */
	chip->use_default_batt_values = of_property_read_bool(spmi->dev.of_node,
					"qcom,use-default-batt-values");

	of_get_property(spmi->dev.of_node, "qcom,thermal-mitigation",
		&(chip->thermal_levels));

	if (chip->thermal_levels > sizeof(int)) {
		chip->thermal_mitigation = kzalloc(
			chip->thermal_levels,
			GFP_KERNEL);

		if (chip->thermal_mitigation == NULL) {
			pr_err("thermal mitigation kzalloc() failed.\n");
			goto fail_chg_enable;
		}

		chip->thermal_levels /= sizeof(int);
		rc = of_property_read_u32_array(spmi->dev.of_node,
				"qcom,thermal-mitigation",
				chip->thermal_mitigation, chip->thermal_levels);
		if (rc) {
			pr_err("qcom,thermal-mitigation missing in dt\n");
			goto fail_chg_enable;
		}
	}

	/* Disable charging when faking battery values */
	if (chip->use_default_batt_values)
		chip->charging_disabled = true;

	spmi_for_each_container_dev(spmi_resource, spmi) {
		if (!spmi_resource) {
			pr_err("qpnp_chg: spmi resource absent\n");
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
						IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			pr_err("node %s IO resource absent!\n",
				spmi->dev.of_node->full_name);
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		rc = qpnp_chg_read(chip, &subtype,
				resource->start + REG_OFFSET_PERP_SUBTYPE, 1);
		if (rc) {
			pr_err("Peripheral subtype read failed rc=%d\n", rc);
			goto fail_chg_enable;
		}

		switch (subtype) {
		case SMBB_CHGR_SUBTYPE:
		case SMBBP_CHGR_SUBTYPE:
			chip->chgr_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_BUCK_SUBTYPE:
		case SMBBP_BUCK_SUBTYPE:
			chip->buck_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_BAT_IF_SUBTYPE:
		case SMBBP_BAT_IF_SUBTYPE:
			chip->bat_if_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_USB_CHGPTH_SUBTYPE:
		case SMBBP_USB_CHGPTH_SUBTYPE:
			chip->usb_chgpth_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_DC_CHGPTH_SUBTYPE:
			chip->dc_chgpth_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_BOOST_SUBTYPE:
		case SMBBP_BOOST_SUBTYPE:
			chip->boost_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_MISC_SUBTYPE:
		case SMBBP_MISC_SUBTYPE:
			chip->misc_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype=0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		default:
			pr_err("Invalid peripheral subtype=0x%x\n", subtype);
			rc = -EINVAL;
			goto fail_chg_enable;
		}
	}
	dev_set_drvdata(&spmi->dev, chip);
	device_init_wakeup(&spmi->dev, 1);
/*                                                                                       */
#ifdef CONFIG_LGE_PM
	if(is_factory_cable()){
		pr_info("Factory cable is detected, set IUSB to MAX.\n");
		qpnp_chg_iusbmax_set(chip, QPNP_CHG_I_MAX_MAX_MA);
	}
/*                                        */

#ifdef CONFIG_LGE_PM
/*                                                                        
                                             */
	if ( HW_REV_A <= lge_get_board_revno()) {
		get_prop_capacity
			= gauge_ic_func_array[MAX17048_TYPE].get_prop_cap_func;
		get_prop_battery_voltage_now
			= gauge_ic_func_array[MAX17048_TYPE].get_prop_battery_vol_func;
	}
	else{
		get_prop_capacity
			= gauge_ic_func_array[BMS_TYPE].get_prop_cap_func;
		get_prop_battery_voltage_now
			= gauge_ic_func_array[BMS_TYPE].get_prop_battery_vol_func;
	}
/*                                      */
#endif
#endif

	chip->vadc_dev = qpnp_get_vadc(chip->dev, "chg");
	if (IS_ERR(chip->vadc_dev)) {
		rc = PTR_ERR(chip->vadc_dev);
		if (rc != -EPROBE_DEFER)
			pr_err("vadc property missing\n");
		goto fail_chg_enable;
	}

	if (chip->bat_if_base) {
		chip->batt_psy.name = "battery";
		chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY;
		chip->batt_psy.properties = msm_batt_power_props;
		chip->batt_psy.num_properties =
			ARRAY_SIZE(msm_batt_power_props);
		chip->batt_psy.get_property = qpnp_batt_power_get_property;
		chip->batt_psy.set_property = qpnp_batt_power_set_property;
		chip->batt_psy.property_is_writeable =
				qpnp_batt_property_is_writeable;
		chip->batt_psy.external_power_changed =
				qpnp_batt_external_power_changed;

		rc = power_supply_register(chip->dev, &chip->batt_psy);
		if (rc < 0) {
			pr_err("batt failed to register rc = %d\n", rc);
			goto fail_chg_enable;
		}
	}

	if (chip->dc_chgpth_base) {
		/*                                                                                 */
#ifdef CONFIG_LGE_PM
		chip->dc_psy.name = "ac";
#else
		chip->dc_psy.name = "qpnp-dc";
#endif
		chip->dc_psy.type = POWER_SUPPLY_TYPE_MAINS;
		chip->dc_psy.supplied_to = pm_power_supplied_to;
		chip->dc_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to);
		chip->dc_psy.properties = pm_power_props_mains;
		chip->dc_psy.num_properties = ARRAY_SIZE(pm_power_props_mains);
#ifdef CONFIG_LGE_PM
		chip->dc_psy.set_property = qpnp_power_set_property_mains;
#endif
		chip->dc_psy.get_property = qpnp_power_get_property_mains;

		rc = power_supply_register(chip->dev, &chip->dc_psy);
		if (rc < 0) {
			pr_err("power_supply_register dc failed rc=%d\n", rc);
			goto unregister_batt;
		}
	}


	/* Turn on appropriate workaround flags */
	qpnp_chg_setup_flags(chip);

	power_supply_set_present(chip->usb_psy,
			qpnp_chg_is_usb_chg_plugged_in(chip));

	if (chip->maxinput_dc_ma && chip->dc_chgpth_base) {
		rc = qpnp_chg_idcmax_set(chip, chip->maxinput_dc_ma);
		if (rc) {
			pr_err("Error setting idcmax property %d\n", rc);
			goto unregister_batt;
		}
	}

	qpnp_chg_charge_en(chip, !chip->charging_disabled);
	qpnp_chg_force_run_on_batt(chip, chip->charging_disabled);

#ifdef CONFIG_LGE_PM
	qpnp_chg = chip;
	pr_info("success chg_dis = %d, usb = %d, b_health = %d batt_present = %d\n",
			chip->charging_disabled,
			qpnp_chg_is_usb_chg_plugged_in(chip),
			get_prop_batt_present(chip),
			get_prop_batt_health(chip));
#else
	pr_info("success chg_dis = %d, usb = %d, dc = %d b_health = %d batt_present = %d\n",
			chip->charging_disabled,
			qpnp_chg_is_usb_chg_plugged_in(chip),
			qpnp_chg_is_dc_chg_plugged_in(chip),
			get_prop_batt_present(chip),
			get_prop_batt_health(chip));
#endif
	return 0;

unregister_batt:
	if (chip->bat_if_base)
		power_supply_unregister(&chip->batt_psy);
fail_chg_enable:
	kfree(chip->thermal_mitigation);
	kfree(chip);
	dev_set_drvdata(&spmi->dev, NULL);
	return rc;
}

static int __devexit
qpnp_charger_remove(struct spmi_device *spmi)
{
	struct qpnp_chg_chip *chip = dev_get_drvdata(&spmi->dev);
	dev_set_drvdata(&spmi->dev, NULL);
	kfree(chip);

	return 0;
}

static struct spmi_driver qpnp_charger_driver = {
	.probe		= qpnp_charger_probe,
	.remove		= __devexit_p(qpnp_charger_remove),
	.driver		= {
		.name	= QPNP_CHARGER_DEV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = qpnp_charger_match_table,
	},
};

/**
 * qpnp_chg_init() - register spmi driver for qpnp-chg
 */
int __init
qpnp_chg_init(void)
{
	return spmi_driver_register(&qpnp_charger_driver);
}
module_init(qpnp_chg_init);

static void __exit
qpnp_chg_exit(void)
{
	spmi_driver_unregister(&qpnp_charger_driver);
}
module_exit(qpnp_chg_exit);


MODULE_DESCRIPTION("QPNP charger driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" QPNP_CHARGER_DEV_NAME);
