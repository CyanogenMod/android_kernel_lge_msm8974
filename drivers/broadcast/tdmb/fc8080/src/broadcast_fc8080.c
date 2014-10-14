#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
//#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/interrupt.h>

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h> 		/* wake_lock, unlock */

#include "../../broadcast_tdmb_drv_ifdef.h"
#include "../inc/broadcast_fc8080.h"
#include "../inc/fci_types.h"
#include "../inc/bbm.h"

#include <linux/pm_qos.h> // FEATURE_DMB_USE_PM_QOS

#include <linux/err.h>
#include <linux/of_gpio.h>

#include <linux/clk.h>
#include <mach/msm_bus.h> // FEATURE_DMB_USE_BUS_SCALE

/* external function */
extern int broadcast_drv_if_isr(void);
extern void tunerbb_drv_fc8080_isr_control(fci_u8 onoff);

/* proto type declare */
static int broadcast_tdmb_fc8080_probe(struct spi_device *spi);
static int broadcast_tdmb_fc8080_remove(struct spi_device *spi);
static int broadcast_tdmb_fc8080_suspend(struct spi_device *spi, pm_message_t mesg);
static int broadcast_tdmb_fc8080_resume(struct spi_device *spi);

/* G2 APQ8064 GPIOs */
#define DMB_EN			76
#define DMB_INT_N		77

/* SPI Data read using workqueue */
//#define FEATURE_DMB_USE_WORKQUEUE
#define FEATURE_DMB_USE_XO
#define FEATURE_DMB_USE_BUS_SCALE
#define FEATURE_DMB_USE_PM_QOS

/************************************************************************/
/* LINUX Driver Setting                                                 */
/************************************************************************/
static uint32 user_stop_flg = 0;
struct tdmb_fc8080_ctrl_blk
{
	boolean									TdmbPowerOnState;
	struct spi_device*						spi_ptr;
#ifdef FEATURE_DMB_USE_WORKQUEUE
	struct work_struct						spi_work;
	struct workqueue_struct*				spi_wq;
#endif
	struct mutex							mutex;
	struct wake_lock						wake_lock;	/* wake_lock,wake_unlock */
	boolean									spi_irq_status;
	spinlock_t								spin_lock;
#ifdef FEATURE_DMB_USE_XO
	struct clk								*clk;
#endif
#ifdef FEATURE_DMB_USE_BUS_SCALE
	struct platform_device					*pdev;
	struct msm_bus_scale_pdata				*bus_scale_pdata;
	fci_u32										bus_scale_client_id;
#endif
#ifdef FEATURE_DMB_USE_PM_QOS
	struct pm_qos_request    pm_req_list;
#endif
};

static struct tdmb_fc8080_ctrl_blk fc8080_ctrl_info;

#if 0
void tdmb_fc8080_spi_write_read_test(void)
{
	uint16 i;
	uint32 wdata = 0;
	uint32 ldata = 0;
	uint32 data = 0;
	uint32 temp = 0;

#define TEST_CNT    5
	tdmb_fc8080_power_on();

	for(i=0;i<TEST_CNT;i++)
	{
		bbm_com_write(NULL, 0xa4, i & 0xff);
		bbm_com_read(NULL, 0xa4, (fci_u8*)&data);
		printk("FC8080 byte test (0x%x,0x%x)\n", i & 0xff, data);
		if((i & 0xff) != data)
			printk("FC8080 byte test (0x%x,0x%x)\n", i & 0xff, data);
	}

	for(i=0;i<TEST_CNT;i++)
	{
		bbm_com_word_write(NULL, 0xa4, i & 0xffff);
		bbm_com_word_read(NULL, 0xa4, (fci_u16*)&wdata);
		printk("FC8080 word test (0x%x,0x%x)\n", i & 0xffff, wdata);
		if((i & 0xffff) != wdata)
			printk("FC8080 word test (0x%x,0x%x)\n", i & 0xffff, wdata);
	}

	for(i=0;i<TEST_CNT;i++)
	{
		bbm_com_long_write(NULL, 0xa4, i & 0xffffffff);
		bbm_com_long_read(NULL, 0xa4, (fci_u32*)&ldata);
		printk("FC8080 long test (0x%x,0x%x)\n", i & 0xffffffff, ldata);
		if((i & 0xffffffff) != ldata)
			printk("FC8080 long test (0x%x,0x%x)\n", i & 0xffffffff, ldata);
	}

	data = 0;

	for(i=0;i<TEST_CNT;i++)
	{
		temp = i&0xff;
		bbm_com_tuner_write(NULL, 0x58, 0x01, (fci_u8*)&temp, 0x01);
		bbm_com_tuner_read(NULL, 0x58, 0x01, (fci_u8*)&data, 0x01);
		printk("FC8080 tuner test (0x%x,0x%x)\n", i & 0xff, data);
		if((i & 0xff) != data)
			printk("FC8080 tuner test (0x%x,0x%x)\n", i & 0xff, data);
	}
	tdmb_fc8080_power_off();
}
#endif

struct spi_device *tdmb_fc8080_get_spi_device(void)
{
	return fc8080_ctrl_info.spi_ptr;
}


void tdmb_fc8080_set_userstop(int mode)
{
	user_stop_flg = mode;
	printk("tdmb_fc8080_set_userstop, user_stop_flg = %d \n", user_stop_flg);
}

int tdmb_fc8080_mdelay(int32 ms)
{
	int32	wait_loop =0;
	int32	wait_ms = ms;
	int		rc = 1;  /* 0 : false, 1 : true */

	if(ms > 100)
	{
		wait_loop = (ms /100);   /* 100, 200, 300 more only , Otherwise this must be modified e.g (ms + 40)/50 */
		wait_ms = 100;
	}

	do
	{
		mdelay(wait_ms);

		if(user_stop_flg == 1)
		{
			printk("~~~~~~~~ Ustop flag is set so return false ms =(%d)~~~~~~~\n", ms);
			rc = 0;
			break;
		}
	}while((--wait_loop) > 0);

	return rc;
}

void tdmb_fc8080_Must_mdelay(int32 ms)
{
	mdelay(ms);
}

int tdmb_fc8080_tdmb_is_on(void)
{
	return (int)fc8080_ctrl_info.TdmbPowerOnState;
}

/* EXPORT_SYMBOL() : when we use external symbol
which is not included in current module - over kernel 2.6 */
//EXPORT_SYMBOL(tdmb_fc8080_tdmb_is_on);


int tdmb_fc8080_power_on(void)
{
	int rc = FALSE;

	printk("tdmb_fc8080_power_on \n");
	if ( fc8080_ctrl_info.TdmbPowerOnState == FALSE )
	{
#ifdef FEATURE_DMB_USE_BUS_SCALE
		msm_bus_scale_client_update_request(fc8080_ctrl_info.bus_scale_client_id, 1); /* expensive call, index:1 is the <84 512 3000 152000> entry */
#endif
#ifdef FEATURE_DMB_USE_XO
		if(fc8080_ctrl_info.clk != NULL) {
			rc = clk_prepare_enable(fc8080_ctrl_info.clk);
			if (rc) {
				dev_err(&fc8080_ctrl_info.spi_ptr->dev, "could not enable clock\n");
				return rc;
			}
		}
#endif
#ifdef FEATURE_DMB_USE_PM_QOS
		if(pm_qos_request_active(&fc8080_ctrl_info.pm_req_list)) {
			pm_qos_update_request(&fc8080_ctrl_info.pm_req_list, 20);
		}
#endif
//		gpio_set_value(PM8058_GPIO_PM_TO_SYS(DMB_ANT_SEL_P-1), 0);
//		gpio_set_value(PM8058_GPIO_PM_TO_SYS(DMB_ANT_SEL_N-1), 1);

		wake_lock(&fc8080_ctrl_info.wake_lock);

		gpio_set_value(DMB_EN, 1);
		mdelay(20); /* Due to X-tal stablization Time */

		tdmb_fc8080_interrupt_free();
		fc8080_ctrl_info.TdmbPowerOnState = TRUE;

		printk("tdmb_fc8080_power_on OK\n");
	}
	else
	{
		printk("tdmb_fc8080_power_on the power already turn on \n");
	}

	printk("tdmb_fc8080_power_on completed \n");
	rc = TRUE;

	return rc;
}

int tdmb_fc8080_power_off(void)
{
	if ( fc8080_ctrl_info.TdmbPowerOnState == TRUE )
	{
		tdmb_fc8080_interrupt_lock();

#ifdef FEATURE_DMB_USE_XO
		if(fc8080_ctrl_info.clk != NULL) {
			clk_disable_unprepare(fc8080_ctrl_info.clk);
		}
#endif

		fc8080_ctrl_info.TdmbPowerOnState = FALSE;

		gpio_set_value(DMB_EN, 0);

		wake_unlock(&fc8080_ctrl_info.wake_lock);
		mdelay(20);

//		gpio_set_value(PM8058_GPIO_PM_TO_SYS(DMB_ANT_SEL_P-1), 1);	// for ESD TEST
//		gpio_set_value(PM8058_GPIO_PM_TO_SYS(DMB_ANT_SEL_N-1), 0);

#ifdef FEATURE_DMB_USE_BUS_SCALE
		msm_bus_scale_client_update_request(fc8080_ctrl_info.bus_scale_client_id, 0); /* expensive call, index:0 is the <84 512 0 0> entry */
#endif
#ifdef FEATURE_DMB_USE_PM_QOS
		if(pm_qos_request_active(&fc8080_ctrl_info.pm_req_list)) {
			pm_qos_update_request(&fc8080_ctrl_info.pm_req_list, PM_QOS_DEFAULT_VALUE);
		}
#endif
	}
	else
	{
		printk("tdmb_fc8080_power_on the power already turn off \n");
	}

	printk("tdmb_fc8080_power_off completed \n");
	return TRUE;
}

int tdmb_fc8080_select_antenna(unsigned int sel)
{
	return FALSE;
}

static struct of_device_id tdmb_spi_table[] = {
	{
		.compatible = "fci,tdmb-fc8080",
	},
	{}
};

static struct spi_driver broadcast_tdmb_driver = {
	.probe = broadcast_tdmb_fc8080_probe,
	.remove	= __devexit_p(broadcast_tdmb_fc8080_remove),
	.suspend = broadcast_tdmb_fc8080_suspend,
	.resume  = broadcast_tdmb_fc8080_resume,
	.driver = {
		.name = "tdmb-fc8080",
		.of_match_table = tdmb_spi_table,
		.bus	= &spi_bus_type,
		.owner = THIS_MODULE,
	},
};

void tdmb_fc8080_interrupt_lock(void)
{
	if (fc8080_ctrl_info.spi_ptr == NULL)
	{
		printk("tdmb_fc8080_interrupt_lock fail\n");
	}
	else
	{
		disable_irq(fc8080_ctrl_info.spi_ptr->irq);
	}
}

void tdmb_fc8080_interrupt_free(void)
{
	if (fc8080_ctrl_info.spi_ptr == NULL)
	{
		printk("tdmb_fc8080_interrupt_free fail\n");
	}
	else
	{
		enable_irq(fc8080_ctrl_info.spi_ptr->irq);
	}
}

int tdmb_fc8080_spi_write_read(uint8* tx_data, int tx_length, uint8 *rx_data, int rx_length)
{
	int rc;

	struct spi_transfer	t = {
			.tx_buf		= tx_data,
			.rx_buf		= rx_data,
			.len		= tx_length+rx_length,
		};

	struct spi_message	m;

	if (fc8080_ctrl_info.spi_ptr == NULL)
	{
		printk("tdmb_fc8080_spi_write_read error txdata=0x%x, length=%d\n", (unsigned int)tx_data, tx_length+rx_length);
		return FALSE;
	}

	mutex_lock(&fc8080_ctrl_info.mutex);

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	rc = spi_sync(fc8080_ctrl_info.spi_ptr, &m);

	if ( rc < 0 )
	{
		printk("tdmb_fc8080_spi_read_burst result(%d), actual_len=%d\n",rc, m.actual_length);
	}

	mutex_unlock(&fc8080_ctrl_info.mutex);

	return TRUE;
}

#ifdef FEATURE_DMB_USE_WORKQUEUE
static irqreturn_t broadcast_tdmb_spi_isr(int irq, void *handle)
{
	struct tdmb_fc8080_ctrl_blk* fc8080_info_p;

	fc8080_info_p = (struct tdmb_fc8080_ctrl_blk *)handle;
	if ( fc8080_info_p && fc8080_info_p->TdmbPowerOnState )
	{
		unsigned long flag;
		if (fc8080_info_p->spi_irq_status)
		{
			printk("######### spi read function is so late skip #########\n");
			return IRQ_HANDLED;
		}
//		printk("***** broadcast_tdmb_spi_isr coming *******\n");
		spin_lock_irqsave(&fc8080_info_p->spin_lock, flag);
		queue_work(fc8080_info_p->spi_wq, &fc8080_info_p->spi_work);
		spin_unlock_irqrestore(&fc8080_info_p->spin_lock, flag);
	}
	else
	{
		printk("broadcast_tdmb_spi_isr is called, but device is off state\n");
	}

	return IRQ_HANDLED;
}

static void broacast_tdmb_spi_work(struct work_struct *tdmb_work)
{
	struct tdmb_fc8080_ctrl_blk *pTdmbWorkData;

	pTdmbWorkData = container_of(tdmb_work, struct tdmb_fc8080_ctrl_blk, spi_work);
	if ( pTdmbWorkData )
	{
		tunerbb_drv_fc8080_isr_control(0);
		pTdmbWorkData->spi_irq_status = TRUE;
		broadcast_drv_if_isr();
		pTdmbWorkData->spi_irq_status = FALSE;
		tunerbb_drv_fc8080_isr_control(1);
	}
	else
	{
		printk("~~~~~~~broadcast_tdmb_spi_work call but pTdmbworkData is NULL ~~~~~~~\n");
	}
}
#else
static irqreturn_t broadcast_tdmb_spi_event_handler(int irq, void *handle)
{
	struct tdmb_fc8080_ctrl_blk* fc8080_info_p;

	fc8080_info_p = (struct tdmb_fc8080_ctrl_blk *)handle;
	if ( fc8080_info_p && fc8080_info_p->TdmbPowerOnState )
	{
		if (fc8080_info_p->spi_irq_status)
		{
			printk("######### spi read function is so late skip ignore #########\n");
			return IRQ_HANDLED;
		}

		tunerbb_drv_fc8080_isr_control(0);
		fc8080_info_p->spi_irq_status = TRUE;
		broadcast_drv_if_isr();
		fc8080_info_p->spi_irq_status = FALSE;
		tunerbb_drv_fc8080_isr_control(1);
	}
	else
	{
		printk("broadcast_tdmb_spi_isr is called, but device is off state\n");
	}

	return IRQ_HANDLED;
}
#endif

static int tdmb_configure_gpios(void)
{
	int rc = OK;
	int err_count = 0;

	rc = gpio_request(DMB_EN, "DMB_EN");
	if (rc < 0) {
		err_count++;
		printk("%s:Failed GPIO DMB_EN request!!!\n",__func__);
	}

	rc = gpio_request(DMB_INT_N, "DMB_INT_N");
	if (rc < 0) {
		err_count++;
		printk("%s:Failed GPIO DMB_INT_N request!!!\n",__func__);
	}

	gpio_direction_output(DMB_EN, 0);
	gpio_direction_input(DMB_INT_N);

	if(err_count > 0) rc = -EINVAL;

	return rc;
}

static int broadcast_tdmb_fc8080_probe(struct spi_device *spi)
{
	int rc;

	if(spi == NULL)
	{
		printk("broadcast_fc8080_probe spi is NULL, so spi can not be set\n");
		return -1;
	}

	fc8080_ctrl_info.TdmbPowerOnState = FALSE;
	fc8080_ctrl_info.spi_ptr 				= spi;
	fc8080_ctrl_info.spi_ptr->mode 			= SPI_MODE_0;
	fc8080_ctrl_info.spi_ptr->bits_per_word 	= 8;
	fc8080_ctrl_info.spi_ptr->max_speed_hz 	= (15000*1000);

#ifdef FEATURE_DMB_USE_BUS_SCALE
	fc8080_ctrl_info.pdev = to_platform_device(&spi->dev);
	fc8080_ctrl_info.bus_scale_pdata = msm_bus_cl_get_pdata(fc8080_ctrl_info.pdev);
	fc8080_ctrl_info.bus_scale_client_id = msm_bus_scale_register_client(fc8080_ctrl_info.bus_scale_pdata);
#endif

	// Once I have a spi_device structure I can do a transfer anytime

	rc = spi_setup(spi);
	printk("broadcast_tdmb_fc8080_probe spi_setup=%d\n", rc);
	bbm_com_hostif_select(NULL, 1);

#ifdef FEATURE_DMB_USE_XO
	fc8080_ctrl_info.clk = clk_get(&fc8080_ctrl_info.spi_ptr->dev, "xo");
	if (IS_ERR(fc8080_ctrl_info.clk)) {
		rc = PTR_ERR(fc8080_ctrl_info.clk);
		dev_err(&fc8080_ctrl_info.spi_ptr->dev, "could not get clock\n");
		return rc;
	}

	/* We enable/disable the clock only to assure it works */
	rc = clk_prepare_enable(fc8080_ctrl_info.clk);
	if (rc) {
		dev_err(&fc8080_ctrl_info.spi_ptr->dev, "could not enable clock\n");
		return rc;
	}
	clk_disable_unprepare(fc8080_ctrl_info.clk);
#endif

#ifdef FEATURE_DMB_USE_WORKQUEUE
	INIT_WORK(&fc8080_ctrl_info.spi_work, broacast_tdmb_spi_work);
	fc8080_ctrl_info.spi_wq = create_singlethread_workqueue("tdmb_spi_wq");
	if(fc8080_ctrl_info.spi_wq == NULL){
		printk("Failed to setup tdmb spi workqueue \n");
		return -ENOMEM;
	}
#endif

	tdmb_configure_gpios();

#ifdef FEATURE_DMB_USE_WORKQUEUE
	rc = request_irq(spi->irq, broadcast_tdmb_spi_isr, IRQF_DISABLED | IRQF_TRIGGER_FALLING,
	                   spi->dev.driver->name, &fc8080_ctrl_info);
#else
	rc = request_threaded_irq(spi->irq, NULL, broadcast_tdmb_spi_event_handler, IRQF_ONESHOT | IRQF_DISABLED | IRQF_TRIGGER_FALLING,
	                   spi->dev.driver->name, &fc8080_ctrl_info);
#endif
	printk("broadcast_tdmb_fc8080_probe request_irq=%d\n", rc);

	tdmb_fc8080_interrupt_lock();

	mutex_init(&fc8080_ctrl_info.mutex);

	wake_lock_init(&fc8080_ctrl_info.wake_lock,  WAKE_LOCK_SUSPEND, dev_name(&spi->dev));

	spin_lock_init(&fc8080_ctrl_info.spin_lock);

#ifdef FEATURE_DMB_USE_PM_QOS
	pm_qos_add_request(&fc8080_ctrl_info.pm_req_list, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
#endif
	printk("broadcast_fc8080_probe End\n");

	return rc;
}

static int broadcast_tdmb_fc8080_remove(struct spi_device *spi)
{
	printk("broadcast_tdmb_fc8080_remove \n");

#ifdef FEATURE_DMB_USE_WORKQUEUE
	if (fc8080_ctrl_info.spi_wq)
	{
		flush_workqueue(fc8080_ctrl_info.spi_wq);
		destroy_workqueue(fc8080_ctrl_info.spi_wq);
	}
#endif

#ifdef FEATURE_DMB_USE_BUS_SCALE
	msm_bus_scale_unregister_client(fc8080_ctrl_info.bus_scale_client_id);
#endif

	free_irq(spi->irq, &fc8080_ctrl_info);

	mutex_destroy(&fc8080_ctrl_info.mutex);

	wake_lock_destroy(&fc8080_ctrl_info.wake_lock);

#ifdef FEATURE_DMB_USE_PM_QOS
	pm_qos_remove_request(&fc8080_ctrl_info.pm_req_list);
#endif
	memset((unsigned char*)&fc8080_ctrl_info, 0x0, sizeof(struct tdmb_fc8080_ctrl_blk));
	return 0;
}

static int broadcast_tdmb_fc8080_suspend(struct spi_device *spi, pm_message_t mesg)
{
	printk("broadcast_tdmb_fc8080_suspend \n");
	return 0;
}

static int broadcast_tdmb_fc8080_resume(struct spi_device *spi)
{
	printk("broadcast_tdmb_fc8080_resume \n");
	return 0;
}

int __devinit broadcast_tdmb_drv_init(void)
{
	int rc;

	rc = broadcast_tdmb_drv_start();
	printk("FC8080 DRV_VER : 20130808, BBM_VER : 1.5.1 \n");
	printk("broadcast_tdmb_fc8080_probe start %d \n", rc);

	return spi_register_driver(&broadcast_tdmb_driver);
}

static void __exit broadcast_tdmb_drv_exit(void)
{
	spi_unregister_driver(&broadcast_tdmb_driver);
}



module_init(broadcast_tdmb_drv_init);
module_exit(broadcast_tdmb_drv_exit);

/* optional part when we include driver code to build-on
it's just used when we make device driver to module(.ko)
so it doesn't work in build-on */
MODULE_DESCRIPTION("FC8080 tdmb device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FCI");

