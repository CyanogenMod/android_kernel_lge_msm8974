/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
#include "msm_ois.h"
#include "msm_cci.h"
#include "msm_ois_i2c.h"

#define OIS_MAKER_ID_ADDR		(0x700)

extern void fuji_ois_init(struct msm_ois_ctrl_t *msm_ois_t);
extern void lgit_ois_init(struct msm_ois_ctrl_t *msm_ois_t);
extern void lgit2_ois_init(struct msm_ois_ctrl_t *msm_ois_t);

static int ois_lock = 1;

DEFINE_MSM_MUTEX(msm_ois_mutex);

static struct msm_ois_ctrl_t msm_ois_t;

static struct msm_camera_i2c_fn_t msm_sensor_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_seq = msm_camera_cci_i2c_write_seq,	//sungmin.woo added
	.i2c_write_table = msm_camera_cci_i2c_write_table,
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
};

static struct msm_camera_i2c_fn_t msm_sensor_qup_func_tbl = {
	.i2c_read = msm_camera_qup_i2c_read,
	.i2c_read_seq = msm_camera_qup_i2c_read_seq,
	.i2c_write = msm_camera_qup_i2c_write,
	.i2c_write_seq = msm_camera_qup_i2c_write_seq,	//sungmin.woo added
	.i2c_write_table = msm_camera_qup_i2c_write_table,
	.i2c_write_seq_table = msm_camera_qup_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_qup_i2c_write_table_w_microdelay,
};

static const struct v4l2_subdev_internal_ops msm_ois_internal_ops = {
//	.open = msm_ois_open,
//	.close = msm_ois_close,
};


int32_t ois_i2c_write_table(struct msm_camera_i2c_reg_setting *write_setting)
{
	int32_t ret = 0;
	ret = msm_ois_t.i2c_client.i2c_func_tbl->i2c_write_table(&msm_ois_t.i2c_client, write_setting);
	return ret;	
}
int32_t ois_i2c_write_seq_table(struct msm_camera_i2c_seq_reg_setting *write_setting)
{
	int32_t ret = 0;
	ret= msm_ois_t.i2c_client.i2c_func_tbl->i2c_write_seq_table(&msm_ois_t.i2c_client, write_setting);
	return ret;	
}

int32_t ois_i2c_write(uint16_t addr, uint16_t data, enum msm_camera_i2c_data_type data_type)
{
	int32_t ret = 0;
	ret= msm_ois_t.i2c_client.i2c_func_tbl->i2c_write(&msm_ois_t.i2c_client, addr, data, data_type);
	return ret;
}

int32_t ois_i2c_read(uint16_t addr, uint16_t *data, enum msm_camera_i2c_data_type data_type)
{
	int32_t ret = 0;
	ret= msm_ois_t.i2c_client.i2c_func_tbl->i2c_read(&msm_ois_t.i2c_client, addr, &data[0], data_type);
	return ret;
}

int32_t ois_i2c_write_seq(uint16_t addr, uint8_t *data, uint16_t num_byte)
{
	int32_t ret = 0;
	ret= msm_ois_t.i2c_client.i2c_func_tbl->i2c_write_seq(&msm_ois_t.i2c_client, addr, data, num_byte);
	return ret;
}

int32_t ois_i2c_read_seq(uint16_t addr, uint8_t *data, uint16_t num_byte)
{
	int32_t ret = 0;
	ret= msm_ois_t.i2c_client.i2c_func_tbl->i2c_read_seq(&msm_ois_t.i2c_client, addr, &data[0], num_byte);
	return ret;
}

int32_t ois_i2c_e2p_write(uint16_t addr, uint16_t data, enum msm_camera_i2c_data_type data_type)
{
	int32_t ret = 0;
	struct msm_camera_i2c_client *ois_i2c_client = NULL;	
	ois_i2c_client = &msm_ois_t.i2c_client;
	
	ois_i2c_client->cci_client->sid = 0xA0 >> 1;
	ret = ois_i2c_client->i2c_func_tbl->i2c_write(ois_i2c_client, addr, data, data_type);
	ois_i2c_client->cci_client->sid = msm_ois_t.sid_ois;
	return ret;
}

int32_t ois_i2c_e2p_read(uint16_t addr, uint16_t *data, enum msm_camera_i2c_data_type data_type)
{
	int32_t ret = 0;
	struct msm_camera_i2c_client *ois_i2c_client = NULL;	
	ois_i2c_client = &msm_ois_t.i2c_client;
	
	ois_i2c_client->cci_client->sid = 0xA0 >> 1;
	ret = ois_i2c_client->i2c_func_tbl->i2c_read(ois_i2c_client, addr, data, data_type);
	ois_i2c_client->cci_client->sid = msm_ois_t.sid_ois;
	return ret;
}
int32_t ois_i2c_act_write(uint8_t data1, uint8_t data2)
{
	int32_t ret = 0;
	struct msm_camera_i2c_client *ois_i2c_client = NULL;
	ois_i2c_client = &msm_ois_t.i2c_client;
	
	ois_i2c_client->cci_client->sid = 0x18 >> 1;
	ois_i2c_client->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;

	ret = ois_i2c_client->i2c_func_tbl->i2c_write(ois_i2c_client, data1, data2, 1);

	ois_i2c_client->cci_client->sid = msm_ois_t.sid_ois;
	ois_i2c_client->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
	return ret;
}

static int32_t msm_ois_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_ois_ctrl_t *act_ctrl_t = NULL;
	pr_err("Enter\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	act_ctrl_t = (struct msm_ois_ctrl_t *)(id->driver_data);
	CDBG("client = %x\n", (unsigned int) client);
	act_ctrl_t->i2c_client.client = client;
	/* Set device type as I2C */
	act_ctrl_t->act_device_type = MSM_CAMERA_I2C_DEVICE;
	act_ctrl_t->i2c_client.i2c_func_tbl = &msm_sensor_qup_func_tbl;

	/* Assign name for sub device */
	snprintf(act_ctrl_t->msm_sd.sd.name, sizeof(act_ctrl_t->msm_sd.sd.name),
		"%s", act_ctrl_t->i2c_driver->driver.name);

	/* Initialize sub device */
	v4l2_i2c_subdev_init(&act_ctrl_t->msm_sd.sd,
		act_ctrl_t->i2c_client.client,
		act_ctrl_t->act_v4l2_subdev_ops);
	v4l2_set_subdevdata(&act_ctrl_t->msm_sd.sd, act_ctrl_t);
	act_ctrl_t->msm_sd.sd.internal_ops = &msm_ois_internal_ops;
	act_ctrl_t->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&act_ctrl_t->msm_sd.sd.entity, 0, NULL, 0);
	act_ctrl_t->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	act_ctrl_t->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_OIS;
	msm_sd_register(&act_ctrl_t->msm_sd);
	CDBG("succeeded\n");
	pr_err("Exit\n");

probe_failure:
	return rc;
}
static int32_t msm_ois_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	struct msm_camera_cci_client *cci_client = NULL;
	pr_err("Enter\n");

	if (!pdev->dev.of_node) {
		pr_err("of_node NULL\n");
		return -EINVAL;
	}

	rc = of_property_read_u32((&pdev->dev)->of_node, "cell-index",
		&pdev->id);
	CDBG("cell-index %d, rc %d\n", pdev->id, rc);
	if (rc < 0) {
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	rc = of_property_read_u32((&pdev->dev)->of_node, "qcom,cci-master",
		&msm_ois_t.cci_master);
	CDBG("qcom,cci-master %d, rc %d\n", msm_ois_t.cci_master, rc);
	if (rc < 0) {
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	/* Set platform device handle */
	msm_ois_t.pdev = pdev;
	/* Set device type as platform device */
	msm_ois_t.act_device_type = MSM_CAMERA_PLATFORM_DEVICE;
	msm_ois_t.i2c_client.i2c_func_tbl = &msm_sensor_cci_func_tbl;
	msm_ois_t.i2c_client.cci_client = kzalloc(sizeof(
		struct msm_camera_cci_client), GFP_KERNEL);
	if (!msm_ois_t.i2c_client.cci_client) {
		pr_err("failed no memory\n");
		return -ENOMEM;
	}

	/* ois initial settings */
	msm_ois_t.i2c_client.cci_client->sid = 0x7C >> 1; //0x48 >> 1;
	msm_ois_t.i2c_client.cci_client->retries = 3;
	msm_ois_t.i2c_client.cci_client->id_map = 0;
	msm_ois_t.i2c_client.cci_client->cci_i2c_master = MASTER_0;
	/* Update sensor address type */
	msm_ois_t.i2c_client.addr_type = MSM_CAMERA_I2C_WORD_ADDR;

	cci_client = msm_ois_t.i2c_client.cci_client;
	cci_client->cci_subdev = msm_cci_get_subdev();
	v4l2_subdev_init(&msm_ois_t.msm_sd.sd,
		msm_ois_t.act_v4l2_subdev_ops);
	v4l2_set_subdevdata(&msm_ois_t.msm_sd.sd, &msm_ois_t);
	msm_ois_t.msm_sd.sd.internal_ops = &msm_ois_internal_ops;
	msm_ois_t.msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(msm_ois_t.msm_sd.sd.name,
		ARRAY_SIZE(msm_ois_t.msm_sd.sd.name), "msm_ois");
	media_entity_init(&msm_ois_t.msm_sd.sd.entity, 0, NULL, 0);
	msm_ois_t.msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	msm_ois_t.msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_OIS;
	msm_sd_register(&msm_ois_t.msm_sd);

	msm_ois_t.sid_ois = msm_ois_t.i2c_client.cci_client->sid;
	msm_ois_t.ois_func_tbl = NULL;
	
	CDBG("Exit\n");
	return rc;
}


static const struct i2c_device_id msm_ois_i2c_id[] = {
	{"msm_ois", (kernel_ulong_t)&msm_ois_t},
	{ }
};

static struct i2c_driver msm_ois_i2c_driver = {
	.id_table = msm_ois_i2c_id,
	.probe  = msm_ois_i2c_probe,
	.remove = __exit_p(msm_ois_i2c_remove),
	.driver = {
		.name = "msm_ois",
	},
};

static const struct of_device_id msm_ois_dt_match[] = {
	{.compatible = "qcom,ois"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_ois_dt_match);

static struct platform_driver msm_ois_platform_driver = {
	.driver = {
		.name = "qcom,ois",
		.owner = THIS_MODULE,
		.of_match_table = msm_ois_dt_match,
	},
};

static int __init msm_ois_init_module(void)
{
	int32_t rc = 0;
	CDBG("Enter\n");
	rc = platform_driver_probe(msm_ois_t.pdriver,
		msm_ois_platform_probe);
	if (!rc)
		return rc;
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(msm_ois_t.i2c_driver);
}

int msm_init_ois(enum ois_ver_t ver)
{
	int rc = 0;
	uint16_t chipid = 0;
	
	ois_i2c_e2p_read(OIS_MAKER_ID_ADDR, &chipid, 1);

	CDBG("Enter %d\n", chipid);


	switch(chipid)
	{
	case 0x01:
		lgit_ois_init(&msm_ois_t);
		msm_ois_t.i2c_client.cci_client->sid = msm_ois_t.sid_ois;
		rc = ois_i2c_read(0x027F, &chipid, 1);
		if (rc < 0) {
				msm_ois_t.ois_func_tbl = NULL;
				printk("%s: kernel ois not supported, rc = %d\n", __func__, rc);
				return OIS_INIT_NOT_SUPPORTED;
		}	
		printk("%s : LGIT OIS module type #1!\n", __func__);
		break;
	case 0x02:
	case 0x05:
		lgit2_ois_init(&msm_ois_t); 
		printk("%s : LGIT OIS module type #2!\n", __func__); 
		break;
	case 0x03:
		fuji_ois_init(&msm_ois_t);
		printk("%s : FujiFilm OIS module!\n", __func__);
		break;
	default:
		printk("%s : unknown module! maker id = %d\n", __func__, chipid);
		msm_ois_t.ois_func_tbl = NULL;
		return OIS_INIT_NOT_SUPPORTED;
	}
	msm_ois_t.i2c_client.cci_client->sid = msm_ois_t.sid_ois;

	if (ois_lock)
	{
		mutex_lock(msm_ois_t.ois_mutex);
		if (msm_ois_t.ois_func_tbl)
		{
			rc = msm_ois_t.ois_func_tbl->ois_on(ver);
			if (rc < 0) {
				msm_ois_t.ois_func_tbl = NULL;
				CDBG("%s: ois open fail\n", __func__);
			}
		}
		else
		{
			CDBG("%s: No OIS support!\n",__func__);
		}
		mutex_unlock(msm_ois_t.ois_mutex);
	}
	return rc;
}

int msm_ois_off(void)
{
	int rc = 0;

	CDBG("Enter\n");

	if (ois_lock)
	{
		mutex_lock(msm_ois_t.ois_mutex);
		if (msm_ois_t.ois_func_tbl)
		{
			msm_ois_t.ois_func_tbl->ois_off();
		}
		else
		{
			printk("%s: No OIS support!\n",__func__);
		}
		mutex_unlock(msm_ois_t.ois_mutex);	
	}
	else
	{
		ois_lock = 1;
	}
	return rc;
}

int msm_ois_info(struct msm_sensor_ois_info_t *ois_info)
{
	int32_t rc = 0;
	memset(ois_info, 0, sizeof(struct msm_sensor_ois_info_t));
	
	if (msm_ois_t.ois_func_tbl)
	{
		rc = msm_ois_t.ois_func_tbl->ois_stat(ois_info);
	}
	return rc;
}

int msm_ois_mode(enum ois_mode_t data)
{
	int32_t rc = 0;
	CDBG("%s: mode = %d\n",__func__, data);
	if (msm_ois_t.ois_func_tbl)
	{
		rc = msm_ois_t.ois_func_tbl->ois_mode(data);
	}
	return rc;
}

int msm_ois_move_lens (int16_t target_x, int16_t target_y)
{
	int32_t rc = 0;
	CDBG("%s: target = %d, %d\n",__func__, target_x, target_y);
	if (msm_ois_t.ois_func_tbl)
	{
		rc = msm_ois_t.ois_func_tbl->ois_move_lens(target_x, target_y);
	}
	return rc;
}


static long msm_ois_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	struct msm_ois_ctrl_t *a_ctrl = v4l2_get_subdevdata(sd);
	void __user *argp = (void __user *)arg;
	CDBG("Enter\n");
	CDBG("%s:%d a_ctrl %p argp %p\n", __func__, __LINE__, a_ctrl, argp);   
	return -ENOIOCTLCMD;
}

static int32_t msm_ois_power(struct v4l2_subdev *sd, int on)
{
	int rc = 0;
	CDBG("Enter\n");
	CDBG("Exit\n");
	return rc;
}

static struct v4l2_subdev_core_ops msm_ois_subdev_core_ops = {
	.ioctl = msm_ois_subdev_ioctl,
	.s_power = msm_ois_power,
};

static struct v4l2_subdev_ops msm_ois_subdev_ops = {
	.core = &msm_ois_subdev_core_ops,
};

static struct msm_ois_ctrl_t msm_ois_t = {
	.i2c_driver = &msm_ois_i2c_driver,
	.pdriver = &msm_ois_platform_driver,
	.act_v4l2_subdev_ops = &msm_ois_subdev_ops,
	.ois_mutex = &msm_ois_mutex,
};

module_init(msm_ois_init_module);
MODULE_DESCRIPTION("MSM OIS");
MODULE_LICENSE("GPL v2");
