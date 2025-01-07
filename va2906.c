// SPDX-License-Identifier: GPL-2.0
/*
 * A V4L2 driver for Zgmicro VA2906 mipi bridge.
 * Copyright (C) 2024-2026 Zgmicro Semiconductor, Inc. All Rights Reserved.
 *
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irq_sim.h>
#include <linux/irqdomain.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/string_helpers.h>
#include <linux/uaccess.h>
#include <linux/iopoll.h>
#include <linux/io.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include "include/VA2906_COMMON_define.h"
#include "include/VA2906_SYS_CTRL_define.h"
#include "include/VA2906_IPI_FORWARDING_define.h"
#include "include/VA2906_GPIO_define.h"


#define VA2906_SOURCE_0				0
#define VA2906_SOURCE_1				1
#define VA2906_SOURCE_2				2
#define VA2906_SOURCE_3				3
#define VA2906_SINK_0				4
#define VA2906_SINK_1				5
#define VA2906_SINK_2				6
#define VA2906_SINK_3				7
#define VA2906_SINK_4				8
#define VA2906_PAD_NUM				9

#define VA2905_SER0_I2C_ADDR		0x50
#define VA2905_SER1_I2C_ADDR		0x51
#define VA2905_SER2_I2C_ADDR		0x52
#define VA2905_SER3_I2C_ADDR		0x53

#define VA2905_CDPHY0_I2C_ADDR		0x18
#define VA2905_CDPHY1_I2C_ADDR		0x19
#define VA2905_CDPHY2_I2C_ADDR		0x1A
#define VA2905_CDPHY3_I2C_ADDR		0x1B

#define VA2906_CDPHY0_I2C_ADDR		0x28

#define VA2906_CDPHYTX0_BASE		0x9000
#define VA2906_CDPHYTX1_BASE		0xa000
#define VA2906_CDPHYTX2_BASE		0xb000

#define VA2906_CDPHYTX2_ENABLE		0x1
#define VA2906_PORT2_COPY_SEL		0x0

#define VA2906_LANE_1				0
#define VA2906_LANE_2				1
#define VA2906_LANE_4				2

#define VA2905_MAX_NUM				4
#define VA2905_CURR_NUM				4

#define SLAVE_MAX_NUM				4

#define VA2905_NUM_ATTRS			5

#define VA2906_CTRL_STREAM_REG		0x0f

#define VA2906_STREAM_ON			0x01
#define VA2906_STREAM_OFF			0x00


#define VA2905_CTRL_MAINPLLCTRL4			0x900c
#define VA2905_CTRL_PKT_CLK_DIV				0x902f
#define VA2905_CTRL_CLKGAT2					0x902c
#define VA2905_CTRL_IO_DRV					0x902b
#define VA2905_CTRL_Device_Ready			0x0184
#define VA2905_CTRL_Functional_Program		0x0190
#define VA2905_CTRL_Pixel_Per_Clock_Enable 	0x0194
#define VA2905_CTRL_RX_DATA_WIDTH_HS_Configuration	0x0254
#define VA2905_CTRL_DEV_Pixel_Per_Clock_Enable	0x3104
#define VA2905_CTRL_Enable_Configuration	0x3136
#define VA2905_CTRL_PBI_Lane_Configuration	0x3140
#define VA2905_CTRL_WORKMODE				0xe007
#define VA2905_CTRL_PHY_CTRL				0x9030

#define VA2906_CTRL_CHIPID_L				0x6c00
#define VA2906_CTRL_CHIPID_H				0x6c01
#define VA2906_CTRL_MAINPLLCTRL4			0x6c0c
#define VA2906_CTRL_PIX_PKT_CLK_GATEN		0xc824
#define VA2906_CTRL_WORKMODE				0x0007
#define VA2906_CTRL_Device_Ready			0x8184
#define VA2906_CTRL_Functional_Program		0x8190
#define VA2906_CTRL_Pixel_Per_Clock_Enable	0x8194
#define VA2906_CTRL_config0					0xa800
#define VA2906_CTRL_config1					0xa801
#define VA2906_CTRL_Lane_Configuration		0xb03c
#define VA2906_CTRL_DEV_Pixel_Per_Clock_Enable	0xb104
#define VA2906_CTRL_Data_Width				0xb12c
#define VA2906_CTRL_Enable_Configuration	0xb136
#define VA2906_CTRL_CDPHY_TXCTRL1			0x9026

#define V4L2_CID_FSYNC_MODE				(V4L2_CID_USER_BASE | 0x60a1)

#define VA2906_LINK_FREQ_750MHZ         750000000U
#define VA2906_LINK_FREQ_500MHZ			500000000U
#define VA2906_LINK_FREQ_250MHZ			250000000U


#define VA2906_DBG				1

#define VA2906_MPW				1



bool va2906_debug_verbose=1;
module_param_named(verbose_debug, va2906_debug_verbose, bool, 0644);
MODULE_PARM_DESC(verbose_debug, "verbose debugging messages");

#define va2906_dbg_verbose(fmt, arg...)                        	\
	do {                                                  		\
		if (va2906_debug_verbose)                        		\
			pr_info(fmt, ##arg); 								\
	} while (0)


static const u32 va2906_mbus_codes[] = {
	MEDIA_BUS_FMT_SBGGR8_1X8, MEDIA_BUS_FMT_SGBRG8_1X8,
	MEDIA_BUS_FMT_SGRBG8_1X8, MEDIA_BUS_FMT_SRGGB8_1X8,
	MEDIA_BUS_FMT_SBGGR10_1X10, MEDIA_BUS_FMT_SGBRG10_1X10,
	MEDIA_BUS_FMT_SGRBG10_1X10, MEDIA_BUS_FMT_SRGGB10_1X10,
	MEDIA_BUS_FMT_SBGGR12_1X12, MEDIA_BUS_FMT_SGBRG12_1X12,
	MEDIA_BUS_FMT_SGRBG12_1X12, MEDIA_BUS_FMT_SRGGB12_1X12,
	MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_YVYU8_1X16,
	MEDIA_BUS_FMT_UYVY8_1X16, MEDIA_BUS_FMT_VYUY8_1X16,
	MEDIA_BUS_FMT_RGB565_1X16, MEDIA_BUS_FMT_BGR888_1X24,
	MEDIA_BUS_FMT_RGB565_2X8_LE, MEDIA_BUS_FMT_RGB565_2X8_BE,
	MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_YVYU8_2X8,
	MEDIA_BUS_FMT_UYVY8_2X8, MEDIA_BUS_FMT_VYUY8_2X8
};

static const char *const test_pattern_menu[] = {
	"disabled",
	"2906 Color bars",
	"2905 1 Color bars",
	"2905 2 Color bars",
	"2905 3 Color bars",
	"2905 4 Color bars",
	"2906 2 Color bars",
	"other"
};

static const u8 test_pattern_bits[] = {
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
};

static const s64 link_freq_items[] = {
	VA2906_LINK_FREQ_750MHZ,
	VA2906_LINK_FREQ_500MHZ,
	VA2906_LINK_FREQ_250MHZ,
};

struct regval_list {
	u16 addr;
	u8 	data;
};

struct regval16_list {
	u16 addr;
	u16 data;
};

struct va2905_attribute {
	struct device_attribute dev_attr;
	unsigned int index;
};

struct va2906_dev;
struct va2905_dev;

struct cdphytx_dev {
	struct i2c_client *i2c_client;
	int (*init)(struct cdphytx_dev *dev);
	u8	dev_addr;
	struct mutex lock;
};

struct cdphyrx_dev {
	struct i2c_client *i2c_client;
	int (*init)(struct cdphyrx_dev *dev);
	u8	dev_addr;
	struct mutex lock;
};

struct va2905_dev {
	struct i2c_client *i2c_client;
	int (*dev_init)(struct va2905_dev *dev);
	int (*comp_init)(struct va2905_dev *dev);
	int (*tunnel_init)(struct va2905_dev *dev);
	int (*acmp_init)(struct va2905_dev *dev);
	int (*acmp_uninit)(struct va2905_dev *dev);
	int (*pal_gpio_init)(struct va2905_dev *dev);
	int (*vproc_compraw10_init)(struct va2905_dev *dev);
	int (*vproc_compyuv422_init)(struct va2905_dev *dev);
	int (*vpg_init)(struct va2905_dev *dev);
	u8	dev_addr;
	u8  comp_enable;
	u8 	tunnel_mode;
	u8 	lane_num;
	u8  port_id;
	u8  vc_id;
	u8  enable;
	struct v4l2_mbus_framefmt *fmt;
	struct cdphyrx_dev cdphyrx;
	struct mutex lock;
};

struct va2906_dev {
	struct i2c_client *i2c_client;
	struct cdphytx_dev cdphytx;
	struct va2905_dev va2905[VA2905_MAX_NUM];
	struct v4l2_subdev sd;
	struct media_pad pad[VA2906_PAD_NUM];
	struct clk *xclk;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *test_gpio;
	struct iio_dev *indio_dev;
	u32 mbus_code;
	u8 	nlanes;
	u8  bpp;
	u8 	acmp_mode;
	u8	fsync_mode;
	u8 	comp_enable;
	u8  tunnel_mode;
	u8 	acmp_enable;
	u8 	soft_reset;
	u8	lane_num[SLAVE_MAX_NUM];
	u8	slave_i2c_addr[SLAVE_MAX_NUM];
	u8	slave_alias_addr[SLAVE_MAX_NUM];
	char* sensor_name[SLAVE_MAX_NUM];
	char* sensor_id[SLAVE_MAX_NUM];
	u8	sensor_index[SLAVE_MAX_NUM];
	u8 	sensor_num;
	u8 	acmp_mc_2906;
	u8	acmp_mc_2905_1;
	u8 	acmp_mc_2905_2;
	u8 	acmp_mc_2905_3;
	u8 	acmp_mc_2905_4;
	struct v4l2_fwnode_endpoint rx[VA2905_MAX_NUM];
	struct v4l2_fwnode_endpoint tx;
	struct v4l2_async_notifier notifier;
	struct v4l2_subdev *s_subdev[VA2905_MAX_NUM];
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *fsync_ctrl;
	struct v4l2_mbus_framefmt fmt[VA2906_PAD_NUM];
	const struct attribute_group **attr_groups;
	struct kobject *va2906_obj;
	struct kobject *va2905_obj;
	struct mutex lock;
	struct mutex mc_lock;
	bool streaming;
	bool is_ready;
};

struct va2906_i2c_dev {
	struct va2906_dev **va2906[VA2905_MAX_NUM];
	struct i2c_adapter adapter;
	struct device *dev;
	u32	speed;		
	u32	flags;
};

struct va2906_gpio_dev {
	struct va2906_dev **va2906;
	struct irq_domain *irq_domain;
	struct gpio_chip gc;
	struct device *dev;
	struct mutex i2c_lock;
	struct mutex irq_lock;
	raw_spinlock_t lock;
	u32 irq;
	u8 *irq_enable;
};

struct va2906_adc_dev {
	struct clk *adc_clk;
	u32 buffer[8];
	struct mutex lock;
};

static const struct regval_list va2906_init_regs[] = {
	{0x1006, 0x58},		
	{0x1007, 0x50},		
	{0x1018, 0x0d},		
	{0x1026, 0x06},
	{0x1008, 0x28},		
	{0x1010, 0x18},		
	{0x1021, 0x01},
	
	{0x1406, 0x58},		
	{0x1407, 0x51},		
	{0x1418, 0x05},		
	{0x1426, 0x06},
	{0x1408, 0x28},		
	{0x1410, 0x19},		
	{0x1421, 0x01},

	{0x1806, 0x58},		
	{0x1807, 0x52},		
	{0x1818, 0x05},		
	{0x1826, 0x06},
	{0x1808, 0x28},		
	{0x1810, 0x20},		
	{0x1821, 0x01},
	
	{0x1c06, 0x58},		
	{0x1c07, 0x53},		
	{0x1c18, 0x05},		
	{0x1c26, 0x06},
	{0x1c08, 0x28},		
	{0x1c10, 0x21},		
	{0x1c21, 0x01},
	
	{0x2046, 0x05},
	{0x2446, 0x05},
	{0x2846, 0x05},
	{0x2c46, 0x05},
	{0xc004, 0x20},
	{0xc005, 0x20},
};

static const struct regval16_list va2906_cdphy_regs[] = {
	{0x0100, 0x1500},		
	{0x0800, 0x3a00},		
	{0x0600, 0x0400},		
	{0x1800, 0x0000},
	{0x1900, 0x1400},		
	{0x1a00, 0x4000},		
	{0x1b00, 0x0000},
	{0x0700, 0x1100},
	{0x0002, 0x0100},
	{0x5302, 0x1000},
	{0x4202, 0x1000},
	{0x4302, 0x0600},
	{0x4502, 0x1000},
	{0x4402, 0x0600},
	{0x4b02, 0x0400},
	{0x4702, 0x0a00},
	{0x5402, 0x0800},
	{0x4802, 0x1000},
	{0x4902, 0x0500},
	{0x4a02, 0x0800},
	{0x4c02, 0x0400},
	{0x0600, 0x0600},
};

static const struct regval16_list va2905_cdphy_regs[] = {
	{0x0100, 0x1500},		
	{0x0800, 0x3a00},		
	{0x0600, 0x0000},		
	{0x0700, 0x1100},
	{0x0002, 0x0100},		
	//{0x3102, 0x1400},		//400MHz
	{0x3102, 0x1800},	
	{0x0600, 0x0200},
};

static const struct regval_list va2906_mpw_cdphy_pll_1_5G_regs[] = {
	{0x6c2c, 0x05},		
	{0x6c2d, 0x2c},		
	{0x6c2e, 0x01},	
	{0x6c2f, 0x01},			//1.5G
	{0x6c31, 0x52},
	{0x6c32, 0x00},
	{0x6c33, 0xed},
	{0x6c34, 0xfd},
	{0x6c35, 0x0d},
	{0x6c36, 0x00},	
	{0x6c37, 0x00},	
	{0x6c38, 0x00},	
};

static const struct regval_list va2906_mpw_cdphy_pll_1G_regs[] = {
	{0x6c2c, 0x01},		
	{0x6c2d, 0x50},		
	{0x6c2e, 0x00},	
	{0x6c2f, 0x02},			//1G
	{0x6c31, 0x9f},
	{0x6c32, 0x01},
	{0x6c33, 0xbe},
	{0x6c34, 0xbc},
	{0x6c35, 0x00},
	{0x6c36, 0x00},	
	{0x6c37, 0x00},	
	{0x6c38, 0x00},	
};

static const struct regval_list va2906_mpw_cdphy_pll_regs[] = {
	{0x6c2c, 0x01},		
	{0x6c2d, 0x50},		
	{0x6c2e, 0x00},	
	{0x6c2f, 0x03},			//500M
	{0x6c31, 0x9f},
	{0x6c32, 0x01},
	{0x6c33, 0xbe},
	{0x6c34, 0xbc},
	{0x6c35, 0x00},
	{0x6c36, 0x00},	
	{0x6c37, 0x00},	
	{0x6c38, 0x00},	
};

static const struct regval_list va2906_mpw_cdphy_regs[] = {
	{0x9053, 0x10},		
	{0x9042, 0x10},		
	{0x9043, 0x06},		
	{0x9045, 0x10},
	{0x9044, 0x06},
	{0x904b, 0x04},	
	{0x9047, 0x0a},
	{0x9054, 0x08},
	{0x9048, 0x10},
	{0x9049, 0x05},	
	{0x904a, 0x08},
	{0x904c, 0x04},
	
	{0xa053, 0x10},		
	{0xa042, 0x10},		
	{0xa043, 0x06},		
	{0xa045, 0x10},
	{0xa044, 0x06},
	{0xa04b, 0x04},	
	{0xa047, 0x0a},
	{0xa054, 0x08},
	{0xa048, 0x10},
	{0xa049, 0x05},	
	{0xa04a, 0x08},
	{0xa04c, 0x04},
	
	{0xb053, 0x10},		
	{0xb042, 0x10},		
	{0xb043, 0x06},		
	{0xb045, 0x10},
	{0xb044, 0x06},
	{0xb04b, 0x04},	
	{0xb047, 0x0a},
	{0xb054, 0x08},
	{0xb048, 0x10},
	{0xb049, 0x05},	
	{0xb04a, 0x08},
	{0xb04c, 0x04},
};

static struct regval_list va2906_vpg_pattern[] = {
	{0x5016, 0xec},
	{0x5017, 0x80},
	{0x5018, 0x80},
	{0x5019, 0x00},
	{0x501a, 0x4a},
	{0x501b, 0x61},
	{0x501c, 0x80},
	{0x501d, 0x00},
	{0x501e, 0x7e},
	{0x501f, 0x44},
	{0x5020, 0xcc},
	{0x5021, 0x00},
	{0x5022, 0xde},
	{0x5023, 0x10},
	{0x5024, 0x89},
	{0x5025, 0x00},
	{0x5026, 0xa4},
	{0x5027, 0x30},
	{0x5028, 0x19},
	{0x5029, 0x00},
	{0x502a, 0xb1},
	{0x502b, 0x9f},
	{0x502c, 0x10},
	{0x502d, 0x00},
	{0x502e, 0x1d},
	{0x502f, 0xf0},
	{0x5030, 0x77},
	{0x5031, 0x00},
	{0x5032, 0x10},
	{0x5033, 0x80},
	{0x5034, 0x80},
	{0x5035, 0x00},
};

static struct regval_list va2905_vpg_pattern[] = {
	{0x1016, 0xec},
	{0x1017, 0x80},
	{0x1018, 0x80},
	{0x1019, 0x00},
	{0x101a, 0x4a},
	{0x101b, 0x61},
	{0x101c, 0x80},
	{0x101d, 0x00},
	{0x101e, 0x7e},
	{0x101f, 0x44},
	{0x1020, 0xcc},
	{0x1021, 0x00},
	{0x1022, 0xde},
	{0x1023, 0x10},
	{0x1024, 0x89},
	{0x1025, 0x00},
	{0x1026, 0xa4},
	{0x1027, 0x30},
	{0x1028, 0x19},
	{0x1029, 0x00},
	{0x102a, 0xb1},
	{0x102b, 0x9f},
	{0x102c, 0x10},
	{0x102d, 0x00},
	{0x102e, 0x1d},
	{0x102f, 0xf0},
	{0x1030, 0x77},
	{0x1031, 0x00},
	{0x1032, 0x10},
	{0x1033, 0x80},
	{0x1034, 0x80},
	{0x1035, 0x00},
};

#define VA2906_ADC_CHANNEL(idx) {				    	\
		.type = IIO_VOLTAGE,				    		\
		.indexed = 1,					    			\
		.channel = (idx),				    			\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), 	\
		.scan_index = (idx),							\
		.scan_type = {									\
		.sign = 's',									\
		.realbits = 6,									\
		.storagebits = 8,								\
		.shift = 2,										\
		.endianness = IIO_CPU,							\
	},													\
}

static const struct iio_chan_spec va2906_adc_iio_channels[] = {
	VA2906_ADC_CHANNEL(0),
	VA2906_ADC_CHANNEL(1),
	VA2906_ADC_CHANNEL(2),
	VA2906_ADC_CHANNEL(3),
	VA2906_ADC_CHANNEL(4),
	VA2906_ADC_CHANNEL(5),
	VA2906_ADC_CHANNEL(6),
	VA2906_ADC_CHANNEL(7),
};

static const unsigned long va2906_avail_scan_masks[] = {
	GENMASK(7, 0),
	0
};

const int acmp_addr_2905 = 0x58;

struct va2906_dev *va2906_device[VA2905_MAX_NUM];
static int va2906_enable[VA2905_MAX_NUM];


#if VA2906_DBG
static unsigned int i2c_addr = 0;
static unsigned int reg_addr = 0;
#endif

static struct va2905_attribute *to_va2905_attr(struct device_attribute *dev_attr)
{
	return container_of(dev_attr, struct va2905_attribute, dev_attr);
}

static inline struct va2906_dev *to_va2906_dev(struct v4l2_subdev *sd)
{
	return container_of(sd, struct va2906_dev, sd);
}

static uint8_t crc8_calc(const uint8_t *data, uint8_t len)
{ 
	uint8_t crc8 = 0x0, bit = 0;
	
	while (len--)
	{
		crc8 ^= *data++;
		for (bit = 8; bit > 0; --bit) 
		{
			crc8 = ( crc8 & 0x80 ) ? (crc8 << 1) ^ 0x63 : (crc8 << 1);
		}
	}
	return crc8;
}
		
static int va2906_read_reg(struct i2c_client *client, u8 addr, u16 reg, u8 *val)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct i2c_msg msg[2];
	int ret;

	if (va2906->acmp_enable)
	{
		u8 buf[6]={0};
		u8 readbuf[6]={0};
		u8 readlen=1;

		buf[0] = (addr==0?client->addr:acmp_addr_2905)<<1;
		if (addr == 0x0)
			buf[1] = va2906->acmp_mc_2906 << 5;
		else if (addr == VA2905_SER0_I2C_ADDR)
			buf[1] = va2906->acmp_mc_2905_1 << 5;
		else if (addr == VA2905_SER1_I2C_ADDR)
			buf[1] = va2906->acmp_mc_2905_2 << 5;
		buf[2] = reg >> 8;
		buf[3] = reg & 0xff;
		buf[4] = readlen;
		buf[5] = crc8_calc(buf,sizeof(buf)-1);
		msg[0].addr = addr==0?client->addr:addr;
		msg[0].flags = client->flags;
		msg[0].buf = &buf[1];
		msg[0].len = sizeof(buf)-1;

		ret = i2c_transfer(client->adapter, &msg[0], 1);
		if (ret < 0) 
		{
			dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
			return ret;
		}
		mutex_lock(&va2906->mc_lock);
		if (addr == 0x0)
			va2906->acmp_mc_2906 ++;
		else if (addr == VA2905_SER0_I2C_ADDR)
			va2906->acmp_mc_2905_1 ++;
		else if (addr == VA2905_SER1_I2C_ADDR)
			va2906->acmp_mc_2905_2 ++;
		mutex_unlock(&va2906->mc_lock);
		msg[1].addr = addr==0?client->addr:addr;
		msg[1].flags = client->flags | I2C_M_RD;
		msg[1].buf = readbuf;
		msg[1].len = readlen+2;

		ret = i2c_transfer(client->adapter, &msg[1], 1);
		if (ret < 0) 
		{
			dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
			return ret;
		}
		*val = readbuf[1];
		mutex_lock(&va2906->mc_lock);
		if (addr == 0x0)
			va2906->acmp_mc_2906 ++;
		else if (addr == VA2905_SER0_I2C_ADDR)
			va2906->acmp_mc_2905_1 ++;
		else if (addr == VA2905_SER1_I2C_ADDR)
			va2906->acmp_mc_2905_2 ++;
		mutex_unlock(&va2906->mc_lock);
	}
	else
	{
		u8 buf[2]={0};
		
		buf[0] = reg >> 8;
		buf[1] = reg & 0xff;
		msg[0].addr = addr==0?client->addr:addr;
		msg[0].flags = client->flags;
		msg[0].buf = buf;
		msg[0].len = sizeof(buf);
		msg[1].addr = addr==0?client->addr:addr;
		msg[1].flags = client->flags | I2C_M_RD;
		msg[1].buf = val;
		msg[1].len = 1;

		ret = i2c_transfer(client->adapter, msg, 2);
		if (ret < 0) 
		{
			dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
			return ret;
		}
	}
	return 0;
}

static int va2906_write_reg(struct i2c_client *client, u8 addr, u16 reg, u8 val)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct i2c_msg msg;
	int ret;

	if (va2906->acmp_enable)
	{
		u8 buf[8]={0};

		buf[0] = (addr==0?client->addr:acmp_addr_2905)<<1;
		if (addr == 0x0)
			buf[1] = va2906->acmp_mc_2906 << 5;
		else if (addr == VA2905_SER0_I2C_ADDR)
			buf[1] = va2906->acmp_mc_2905_1 << 5;
		else if (addr == VA2905_SER1_I2C_ADDR)
			buf[1] = va2906->acmp_mc_2905_2 << 5;
		buf[2] = reg >> 8;
		buf[3] = reg & 0xff;
		buf[4] = 1;
		buf[5] = crc8_calc(buf,5);
		buf[6] = val;
		buf[7] = crc8_calc(&buf[6],1);
		msg.addr = addr==0?client->addr:addr;
		msg.flags = client->flags;
		msg.buf = &buf[1];
		msg.len = sizeof(buf)-1;

		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0) 
		{
			dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
			return ret;
		}
		mutex_lock(&va2906->mc_lock);
		if (addr == 0x0)
			va2906->acmp_mc_2906 ++;
		else if (addr == VA2905_SER0_I2C_ADDR)
			va2906->acmp_mc_2905_1 ++;
		else if (addr == VA2905_SER1_I2C_ADDR)
			va2906->acmp_mc_2905_2 ++;
		mutex_unlock(&va2906->mc_lock);
	}
	else
	{
		u8 buf[3]={0};
		
		buf[0] = reg >> 8;
		buf[1] = reg & 0xff;
		buf[2] = val;
		msg.addr = addr==0?client->addr:addr;
		msg.flags = client->flags;
		msg.buf = buf;
		msg.len = sizeof(buf);

		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0) 
		{
			dev_dbg(&client->dev, "%s: va2906 i2c_transfer, reg: %x => %d\n", __func__, reg, ret);
			return ret;
		}
		
	#if 1
		{
			u8 buf[2]={0};
			struct i2c_msg msg[2];
		
			val=0;
			buf[0] = reg >> 8;
			buf[1] = reg & 0xff;
			msg[0].addr = addr==0?client->addr:addr;
			msg[0].flags = client->flags;
			msg[0].buf = buf;
			msg[0].len = sizeof(buf);
			msg[1].addr = addr==0?client->addr:addr;
			msg[1].flags = client->flags | I2C_M_RD;
			msg[1].buf = &val;
			msg[1].len = 1;

			ret = i2c_transfer(client->adapter, msg, 2);
			if (ret < 0) 
			{
				dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
				return ret;
			}
			pr_info("----reg=0x%x--val=0x%x---------\r\n",reg,val);
		}
	#endif
	
	}
	return 0;
}

static int va2906_read_reg16(struct i2c_client *client, u8 addr, u16 reg, u8 *val)
{
	struct i2c_msg msg[2];
	u8 buf[2]={0};
	int ret;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	msg[0].addr = addr==0?client->addr:addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);
	msg[1].addr = addr==0?client->addr:addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = 2;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) 
	{
		dev_dbg(&client->dev, "%s:va2906 %x i2c_transfer, reg: %x => %d\n", __func__, client->addr, reg, ret);
		return ret;
	}
	return 0;
}

static int va2906_write_reg16(struct i2c_client *client, u8 addr, u16 reg, u16 val)
{
	struct i2c_msg msg;
	u8 buf[4]={0};
	int ret;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val >> 8;
	buf[3] = val & 0xff;
	msg.addr = addr==0?client->addr:addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) 
	{
		dev_dbg(&client->dev, "%s: va2906 i2c_transfer, reg: %x => %d\n", __func__, reg, ret);
		return ret;
	}
	return 0;
}

static int va2906_mod_reg(struct i2c_client *client, u8 addr, u16 reg, u8 mask, u8 val)
{
	u8 readval;
	int ret;

	ret = va2906_read_reg(client, addr, reg, &readval);
	if (ret)
		return ret;

	readval &= ~mask;
	val &= mask;
	val |= readval;

	return va2906_write_reg(client, addr, reg, val);
}

static int va2906_write_array(struct va2906_dev *dev, u8 addr, const struct regval_list *regs, int array_size)
{
	struct i2c_client *client = dev->i2c_client;
	int i, ret;

	for (i = 0; i < array_size; i++) 
	{
		ret = va2906_write_reg(client, addr, regs[i].addr, regs[i].data);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static void va2906_reset(struct va2906_dev *dev)
{
	gpiod_set_value_cansleep(dev->reset_gpio, 1);
	usleep_range(5000, 10000);
	gpiod_set_value_cansleep(dev->reset_gpio, 0);
	usleep_range(5000, 10000);
	gpiod_set_value_cansleep(dev->reset_gpio, 1);
	usleep_range(5000, 10000);
}

static int va2906_set_power_on(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret;

	ret = clk_prepare_enable(dev->xclk);
	if (ret) 
	{
		dev_err(&client->dev, "%s: va2906 failed to enable clock\n", __func__);
		return ret;
	}
	if (dev->reset_gpio) 
	{
		dev_dbg(&client->dev, "va2906 apply reset");
		va2906_reset(dev);
	}
	else 
	{
		dev_dbg(&client->dev, "va2906 don't apply reset");
		usleep_range(5000, 10000);
	}
	return 0;
}

static void va2906_set_power_off(struct va2906_dev *dev)
{
	clk_disable_unprepare(dev->xclk);
}

static int va2906_detect(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	u8 reg_l=0,reg_h=0;

	va2906_read_reg(client, 0, REG_VA2906_SYS_CTRL_CHIPID_0, &reg_l);
	va2906_read_reg(client, 0, REG_VA2906_SYS_CTRL_CHIPID_1, &reg_h);
	if (reg_l==0x06&&reg_h==0x29)
	{
		dev_info(&client->dev, "va2906 chip detect ok \n");
		return 0;
	}
	else
	{
		dev_err(&client->dev, "va2906 chip detect failed! \n");
		return 1;
	}
}

static int va2906_cdphy_init(struct cdphytx_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1,i;
	u8 addr = dev->dev_addr;
	u8 buf[4]={0};
	u16 reg_addr,reg_val;
	
	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	for (i = 0; i < ARRAY_SIZE(va2906_cdphy_regs); i++) 
	{
		ret = va2906_write_reg16(client, addr, va2906_cdphy_regs[i].addr, va2906_cdphy_regs[i].data);
		if (ret < 0)
			goto error;
	}
	usleep_range(20000, 25000);
	
	reg_addr=0x0600;
	reg_val=0x0700;
	ret = va2906_write_reg16(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	va2906_read_reg16(client, addr, reg_addr, buf);
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906  cdphy init regs failed %d",ret);
	return 0;
}

static int va2906_cdphy_mpw_init(struct cdphytx_dev *dev)
{
	struct va2906_dev *va2906 = container_of(dev, struct va2906_dev, cdphytx);
	struct i2c_client *client = dev->i2c_client;
	int ret=-1,i;
	u8 addr = dev->dev_addr;
	u32 link_freq=0;
	u16 reg_addr;
	u8 reg_val;
	
	va2906_dbg_verbose("%s\n", __func__);
	reg_addr=0x6c30;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	link_freq = v4l2_ctrl_g_ctrl(va2906->link_freq);
	if (link_freq==0)
	{
		for (i = 0; i < ARRAY_SIZE(va2906_mpw_cdphy_pll_1_5G_regs); i++) 
		{
			ret = va2906_write_reg(client, addr, va2906_mpw_cdphy_pll_1_5G_regs[i].addr, va2906_mpw_cdphy_pll_1_5G_regs[i].data);
			if (ret < 0)
				goto error;
		}
	}
	else if (link_freq==1)
	{
		for (i = 0; i < ARRAY_SIZE(va2906_mpw_cdphy_pll_1G_regs); i++) 
		{
			ret = va2906_write_reg(client, addr, va2906_mpw_cdphy_pll_1G_regs[i].addr, va2906_mpw_cdphy_pll_1G_regs[i].data);
			if (ret < 0)
				goto error;
		}
	}
	else
	{
		for (i = 0; i < ARRAY_SIZE(va2906_mpw_cdphy_pll_regs); i++) 
		{
			ret = va2906_write_reg(client, addr, va2906_mpw_cdphy_pll_regs[i].addr, va2906_mpw_cdphy_pll_regs[i].data);
			if (ret < 0)
				goto error;
		}
	}	
	reg_addr=0x6c30;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	
	reg_addr=0x5d10;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x0<<0);
	if (ret)
		goto error;
	reg_addr=0x5d31;
	reg_val=0x00;	
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x9000;
	reg_val=0x01;	
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x9013;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x1<<0);
	if (ret)
		goto error;
	reg_addr=0x9015;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x4<<0);
	if (ret)
		goto error;
	reg_addr=0x9074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x913d;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<4, 0x3<<4);
	if (ret)
		goto error;
	
	reg_addr=0x6110;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x0<<0);
	if (ret)
		goto error;
	reg_addr=0x6131;
	reg_val=0x00;	
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0xa000;
	reg_val=0x01;	
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xa013;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x1<<0);
	if (ret)
		goto error;
	reg_addr=0xa015;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x4<<0);
	if (ret)
		goto error;
	reg_addr=0xa074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0xa13d;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<4, 0x3<<4);
	if (ret)
		goto error;
	
#if VA2906_CDPHYTX2_ENABLE
	reg_addr=0xb000;
	reg_val=0x01;	
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xb013;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x1<<0);
	if (ret)
		goto error;
	reg_addr=0xb015;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x4<<0);
	if (ret)
		goto error;
	reg_addr=0xb074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0xb13d;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<4, 0x3<<4);
	if (ret)
		goto error;
#endif

	for (i = 0; i < ARRAY_SIZE(va2906_mpw_cdphy_regs); i++) 
	{
		ret = va2906_write_reg(client, addr, va2906_mpw_cdphy_regs[i].addr, va2906_mpw_cdphy_regs[i].data);
		if (ret < 0)
			goto error;
	}
	
	//CDPHYTX0  接口上，需要clk0 ,d0 ,d1做P,N swap. 
	//应该是D2 D3 swap 不是D0 D1 swap
	reg_addr=0x901e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x1<<5);
	if (ret)
		goto error;
	reg_addr=0x9402;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x3<<0);
	if (ret)
		goto error;
	reg_addr=0x901e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x0<<5);
	if (ret)
		goto error;
	
	//CDPHYTX1  接口上，需要clk0 ,d0 ,d1做P,N swap.
	reg_addr=0xa01e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x1<<5);
	if (ret)
		goto error;
	reg_addr=0xa402;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x3<<0);
	if (ret)
		goto error;
	reg_addr=0xa01e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x0<<5);
	if (ret)
		goto error;
	
#if VA2906_CDPHYTX2_ENABLE
	reg_addr=0xb01e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x1<<5);
	if (ret)
		goto error;
	reg_addr=0xb402;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x3<<0);
	if (ret)
		goto error;
	reg_addr=0xb01e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<5, 0x0<<5);
	if (ret)
		goto error;
#endif

	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xff<<0, 0x88<<0);
	if (ret)
		goto error;
	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xff<<0, 0xcc<<0);
	if (ret)
		goto error;

#if VA2906_CDPHYTX2_ENABLE
	reg_addr=0x6c27;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x8<<0);
	if (ret)
		goto error;
	reg_addr=0x6c27;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0xc<<0);
	if (ret)
		goto error;
#endif

	reg_addr=0x9074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<1, 0x1<<1);
	if (ret)
		goto error;
	
	reg_addr=0xa074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<1, 0x1<<1);
	if (ret)
		goto error;
	
#if VA2906_CDPHYTX2_ENABLE
	reg_addr=0xb074;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<1, 0x1<<1);
	if (ret)
		goto error;
#endif

error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 cdphy init regs failed %d",ret);
	return 0;
}

static int va2906_vpg_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct v4l2_mbus_framefmt *fmt=&dev->fmt[0];
	unsigned int width=fmt->width; 
	unsigned int height=fmt->height;
	int ret=-1,i=0,fps=5;
	int hbp,vtotal,vbp,vfp,hactive,vactive,vblank,htotal,hblank,wc,wc_odd;
	u8 addr = 0;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: width=0x%x-height=0x%x\n", __func__, width,height);
	reg_addr=0x5400;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<4, 0x0<<4);
	if (ret)
		goto error;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x1<<0);
	if (ret)
		goto error;
	
	//VPG输出yuv
	for (i = 0; i < ARRAY_SIZE(va2906_vpg_pattern); i++) 
	{
		ret = va2906_write_reg(client, addr, va2906_vpg_pattern[i].addr, va2906_vpg_pattern[i].data);
		if (ret < 0)
			goto error;
	}
	
	//视频格式
	if (fmt->code==MEDIA_BUS_FMT_UYVY8_1X16|| fmt->code==MEDIA_BUS_FMT_UYVY8_2X8||
		fmt->code==MEDIA_BUS_FMT_VYUY8_1X16|| fmt->code==MEDIA_BUS_FMT_VYUY8_2X8||
		fmt->code==MEDIA_BUS_FMT_YUYV8_1X16|| fmt->code==MEDIA_BUS_FMT_YUYV8_2X8||
		fmt->code==MEDIA_BUS_FMT_YVYU8_1X16|| fmt->code==MEDIA_BUS_FMT_YVYU8_2X8)
	{	
		reg_addr=0x5001;
		reg_val=0x1e;	//YUV422 8-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_UYVY10_1X20|| fmt->code==MEDIA_BUS_FMT_UYVY10_2X10||
		fmt->code==MEDIA_BUS_FMT_VYUY10_1X20|| fmt->code==MEDIA_BUS_FMT_VYUY10_2X10||
		fmt->code==MEDIA_BUS_FMT_YUYV10_1X20|| fmt->code==MEDIA_BUS_FMT_YUYV10_2X10||
		fmt->code==MEDIA_BUS_FMT_YVYU10_1X20|| fmt->code==MEDIA_BUS_FMT_YVYU10_2X10)
	{	
		reg_addr=0x5001;
		reg_val=0x1f;	//YUV422 10-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_UYVY8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_VYUY8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_YUYV8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_YVYU8_1_5X8)
	{	
		reg_addr=0x5001;
		reg_val=0x18;	//YUV420 8bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB8_1X8)
	{	
		reg_addr=0x5001;
		reg_val=0x2a;	//RAW8
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
	{	
		reg_addr=0x5001;
		reg_val=0x2b;	//RAW10
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB12_1X12)
	{	
		reg_addr=0x5001;
		reg_val=0x2c;	//RAW12
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB14_1X14)
	{	
		reg_addr=0x5001;
		reg_val=0x2d;	//RAW14
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB16_1X16)
	{	
		reg_addr=0x5001;
		reg_val=0x2e;	//RAW16
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB444_1X12|| 
		fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x20;	//RGB444
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x21;	//RGB555
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB565_1X16|| 
		fmt->code==MEDIA_BUS_FMT_RGB565_1X24_CPADHI|| 
		fmt->code==MEDIA_BUS_FMT_BGR565_2X8_BE||
		fmt->code==MEDIA_BUS_FMT_BGR565_2X8_LE||
		fmt->code==MEDIA_BUS_FMT_RGB565_2X8_BE||
		fmt->code==MEDIA_BUS_FMT_RGB565_2X8_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x22;	//RGB565
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_BGR666_1X18|| 
		fmt->code==MEDIA_BUS_FMT_RGB666_1X18)
	{	
		reg_addr=0x5001;
		reg_val=0x23;	//RGB666
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB888_1X24|| 
		fmt->code==MEDIA_BUS_FMT_RGB888_2X12_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB888_2X12_LE||
		fmt->code==MEDIA_BUS_FMT_RGB888_3X8)
	{	
		reg_addr=0x5001;
		reg_val=0x24;	//RGB888
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else
	{
		dev_err(&client->dev, "va2906 vpg unsupported format 0x%x",dev->fmt->code);
	}
	
	reg_addr=0x5004;
	reg_val=width & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5005;
	reg_val=width >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5006;
	reg_val=height & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5007;
	reg_val=height >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	hactive = width;
	vactive = height;
	vblank = 30;
	vtotal = vactive + vblank;
	vbp = vblank / 2;
	vfp = vblank - vbp;
	htotal = 50000000/(fps*vtotal) - hactive/4 + hactive;
	hblank = 50000000/(fps*vtotal) - hactive/4 - 1;
	hbp = hblank / 2;
	wc = hactive * 2;
	wc_odd = hactive * 2;
	//hbp
	reg_addr=0x5008;
	reg_val=hbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5009;
	reg_val=hbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//htotal 
	reg_addr=0x5010;
	reg_val=htotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5011;
	reg_val=htotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vbp 
	reg_addr=0x5014;
	reg_val=vbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5015;
	reg_val=vbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vtotal 
	reg_addr=0x5012;
	reg_val=vtotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5013;
	reg_val=vtotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc 
	reg_addr=0x5041;
	reg_val=wc & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5042;
	reg_val=wc >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc_odd 
	reg_addr=0x5063;
	reg_val=wc_odd & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5064;
	reg_val=wc_odd >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x5000;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 0x%x vpg set regs failed %d",addr,ret);

	return ret;
}

static int va2906_2_vpg_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct v4l2_mbus_framefmt *fmt=&dev->fmt[0];
	unsigned int width=fmt->width; 
	unsigned int height=fmt->height;
	int ret=-1,i=0,fps=5;
	int hbp,vtotal,vbp,vfp,hactive,vactive,vblank,htotal,hblank,wc,wc_odd;
	u8 addr = 0;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: width=0x%x-height=0x%x\n", __func__, width,height);
	reg_addr=0x5400;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<4, 0x0<<4);
	if (ret)
		goto error;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x4<<0);
	if (ret)
		goto error;
	
	//VPG输出yuv
	for (i = 0; i < ARRAY_SIZE(va2906_vpg_pattern); i++) 
	{
		ret = va2906_write_reg(client, addr, va2906_vpg_pattern[i].addr, va2906_vpg_pattern[i].data);
		if (ret < 0)
			goto error;
	}
	
	//视频格式
	if (fmt->code==MEDIA_BUS_FMT_UYVY8_1X16|| fmt->code==MEDIA_BUS_FMT_UYVY8_2X8||
		fmt->code==MEDIA_BUS_FMT_VYUY8_1X16|| fmt->code==MEDIA_BUS_FMT_VYUY8_2X8||
		fmt->code==MEDIA_BUS_FMT_YUYV8_1X16|| fmt->code==MEDIA_BUS_FMT_YUYV8_2X8||
		fmt->code==MEDIA_BUS_FMT_YVYU8_1X16|| fmt->code==MEDIA_BUS_FMT_YVYU8_2X8)
	{	
		reg_addr=0x5001;
		reg_val=0x1e;	//YUV422 8-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_UYVY10_1X20|| fmt->code==MEDIA_BUS_FMT_UYVY10_2X10||
		fmt->code==MEDIA_BUS_FMT_VYUY10_1X20|| fmt->code==MEDIA_BUS_FMT_VYUY10_2X10||
		fmt->code==MEDIA_BUS_FMT_YUYV10_1X20|| fmt->code==MEDIA_BUS_FMT_YUYV10_2X10||
		fmt->code==MEDIA_BUS_FMT_YVYU10_1X20|| fmt->code==MEDIA_BUS_FMT_YVYU10_2X10)
	{	
		reg_addr=0x5001;
		reg_val=0x1f;	//YUV422 10-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_UYVY8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_VYUY8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_YUYV8_1_5X8|| 
		fmt->code==MEDIA_BUS_FMT_YVYU8_1_5X8)
	{	
		reg_addr=0x5001;
		reg_val=0x18;	//YUV420 8bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG8_1X8|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB8_1X8)
	{	
		reg_addr=0x5001;
		reg_val=0x2a;	//RAW8
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
	{	
		reg_addr=0x5001;
		reg_val=0x2b;	//RAW10
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG12_1X12|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB12_1X12)
	{	
		reg_addr=0x5001;
		reg_val=0x2c;	//RAW12
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG14_1X14|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB14_1X14)
	{	
		reg_addr=0x5001;
		reg_val=0x2d;	//RAW14
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_SBGGR16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SGBRG16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SGRBG16_1X16|| 
		fmt->code==MEDIA_BUS_FMT_SRGGB16_1X16)
	{	
		reg_addr=0x5001;
		reg_val=0x2e;	//RAW16
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB444_1X12|| 
		fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x20;	//RGB444
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x21;	//RGB555
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB565_1X16|| 
		fmt->code==MEDIA_BUS_FMT_RGB565_1X24_CPADHI|| 
		fmt->code==MEDIA_BUS_FMT_BGR565_2X8_BE||
		fmt->code==MEDIA_BUS_FMT_BGR565_2X8_LE||
		fmt->code==MEDIA_BUS_FMT_RGB565_2X8_BE||
		fmt->code==MEDIA_BUS_FMT_RGB565_2X8_LE)
	{	
		reg_addr=0x5001;
		reg_val=0x22;	//RGB565
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_BGR666_1X18|| 
		fmt->code==MEDIA_BUS_FMT_RGB666_1X18)
	{	
		reg_addr=0x5001;
		reg_val=0x23;	//RGB666
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (fmt->code==MEDIA_BUS_FMT_RGB888_1X24|| 
		fmt->code==MEDIA_BUS_FMT_RGB888_2X12_BE|| 
		fmt->code==MEDIA_BUS_FMT_RGB888_2X12_LE||
		fmt->code==MEDIA_BUS_FMT_RGB888_3X8)
	{	
		reg_addr=0x5001;
		reg_val=0x24;	//RGB888
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else
	{
		dev_err(&client->dev, "va2906 vpg unsupported format 0x%x",dev->fmt->code);
	}
	
	reg_addr=0x5004;
	reg_val=width & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5005;
	reg_val=width >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5006;
	reg_val=height & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5007;
	reg_val=height >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	hactive = width;
	vactive = height;
	vblank = 30;
	vtotal = vactive + vblank;
	vbp = vblank / 2;
	vfp = vblank - vbp;
	htotal = 50000000/(fps*vtotal) - hactive/4 + hactive;
	hblank = 50000000/(fps*vtotal) - hactive/4 - 1;
	hbp = hblank / 2;
	wc = hactive * 2;
	wc_odd = hactive * 2;
	//hbp
	reg_addr=0x5008;
	reg_val=hbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5009;
	reg_val=hbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//htotal 
	reg_addr=0x5010;
	reg_val=htotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5011;
	reg_val=htotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vbp 
	reg_addr=0x5014;
	reg_val=vbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5015;
	reg_val=vbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vtotal 
	reg_addr=0x5012;
	reg_val=vtotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5013;
	reg_val=vtotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc 
	reg_addr=0x5041;
	reg_val=wc & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5042;
	reg_val=wc >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc_odd 
	reg_addr=0x5063;
	reg_val=wc_odd & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5064;
	reg_val=wc_odd >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x5000;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 0x%x vpg set regs failed %d",addr,ret);

	return ret;
}

static int va2905_cdphy_init(struct cdphyrx_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1,i;
	u8 addr = dev->dev_addr;
	u16 reg_addr,reg_val;
	
	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	for (i = 0; i < ARRAY_SIZE(va2905_cdphy_regs); i++) 
	{
		ret = va2906_write_reg16(client, addr, va2905_cdphy_regs[i].addr, va2905_cdphy_regs[i].data);
		if (ret < 0)
			goto error;
	}
	usleep_range(20000, 25000);
	
	reg_addr=0x0600;
	reg_val=0x0300;
	ret = va2906_write_reg16(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 %d cdphy init regs failed %d",addr,ret);
	return 0;
}

static int va2905_cdphy_mpw_init(struct cdphyrx_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u16 reg_addr;
	u8 reg_val;
	
	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0xb000;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0xb013;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x1<<0);
	if (ret)
		goto error;
	reg_addr=0xb015;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x4<<0);
	if (ret)
		goto error;
	reg_addr=0xb131;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x0<<0);
	if (ret)
		goto error;
	reg_addr=0xb13d;
	reg_val=0x25;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xb136;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<6, 0x0<<6);
	if (ret)
		goto error;
	reg_addr=0xb13e;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0xb007;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<6, 0x2<<6);
	if (ret)
		goto error;
	reg_addr=0xb031;
	reg_val=0x12;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xb001;	//clk  PN swap
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x1<<0);
	if (ret)
		goto error;
	
	usleep_range(20000, 25000);
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 %d cdphy init regs failed %d",addr,ret);
	return 0;
}

static int va2905_pal_gpio_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	u8 addr = dev->dev_addr;
	int ret=-1;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0xc049;
	reg_val=0x1e;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04a;
	reg_val=0x22;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04b;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04c;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04d;
	reg_val=0x3f;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04e;
	reg_val=0x00;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc04f;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc050;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc051;
	reg_val=0x3f;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc052;
	reg_val=0x00;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc053;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc054;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc055;
	reg_val=0x3f;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0xc056;
	reg_val=0x00;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
		
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x pal gpio regs init failed %d",addr,ret);

	return ret;
}

static int va2905_dev_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val,lane_num=1;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	
#if VA2906_MPW
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<4, 0x0<<4);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x0<<3);
	if (ret)
		goto error;
#else
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<4, 0x1<<4);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
#endif
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	usleep_range(50000, 100000);
	
	//配置SYS_CTRL视频通路选择寄存器
	if (dev->comp_enable)
	{
		reg_addr=0x902b;
		ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<2, 0x1<<2);
		if (ret)
			goto error;
	}
	else
	{
		reg_addr=0x902b;
		ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<2, 0x2<<2);
		if (ret)
			goto error;
	}
	
	//配置CSI2_HOST寄存器
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<6, 0x1<<6);
	if (ret)
		goto error;
	
	usleep_range(50000, 100000);
	ret = va2906_read_reg(client, addr, reg_addr, &reg_val);
	if (ret)
		goto error;
	//if (reg_val & 1)
	//	pr_info("2905 %d csi_host is ready\r\n",addr);
	//else
	//	pr_info("2905 %d csi_host is not ready\r\n",addr);
	
	reg_addr=0x018c;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018d;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018e;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018f;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	if (dev->lane_num)
		lane_num = dev->lane_num-1;
	
	reg_addr=0x0190;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, (0x8|lane_num)<<0);
	if (ret)
		goto error;
	reg_addr=0x0194;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x7<<0);
	if (ret)
		goto error;
	reg_addr=0x0254;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x2<<0);
	if (ret)
		goto error;
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x1<<0);
	if (ret)
		goto error;
	
	if (dev->comp_enable)
	{
		if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10||
			dev->fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10||
			dev->fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10||
			dev->fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
				dev->vproc_compraw10_init(dev);
		else if (dev->fmt->code==MEDIA_BUS_FMT_UYVY8_1X16||
			dev->fmt->code==MEDIA_BUS_FMT_VYUY8_1X16||
			dev->fmt->code==MEDIA_BUS_FMT_YUYV8_1X16||
			dev->fmt->code==MEDIA_BUS_FMT_YVYU8_1X16)
				dev->vproc_compyuv422_init(dev);
		else
			dev_dbg(&client->dev, "va2905 unsupported compress mode 0x%x",dev->fmt->code);
	}
	
	//配置CSI2_DEVICE寄存器
	reg_addr=0x3104;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3136;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3140;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	
	//配置PAL_CSI2寄存器
	if (dev->comp_enable)
	{
		reg_addr=0xe007;
		reg_val=0x00;
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else
	{
		reg_addr=0xe007;
		reg_val=0x01;
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	
	//配置CDPHYRX寄存器
	dev->cdphyrx.init(&dev->cdphyrx);
	
	//配置SYS_CTRL寄存器打开CDPHYRX
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x38<<0);
	if (ret)
		goto error;
	usleep_range(50000, 100000);
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x39<<0);
	if (ret)
		goto error;
	
	usleep_range(20000, 25000);
	reg_addr=0xd004;
	reg_val=0x20;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3138;
	reg_val=0x03;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x init regs failed %d",addr,ret);

	return ret;
}

static int va2905_vpg_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	unsigned int width=dev->fmt->width; 
	unsigned int height=dev->fmt->height;
	int ret=-1,i=0,fps=10;
	int hbp,vtotal,vbp,vfp,hactive,vactive,vblank,htotal,hblank,wc,wc_odd;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x902b;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<2, 0x2<<2);
	if (ret)
		goto error;
	reg_addr=0x4000;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x1<<0);
	if (ret)
		goto error;
	
	//VPG输出yuv
	for (i = 0; i < ARRAY_SIZE(va2905_vpg_pattern); i++) 
	{
		ret = va2906_write_reg(client, addr, va2905_vpg_pattern[i].addr, va2905_vpg_pattern[i].data);
		if (ret < 0)
			goto error;
	}
	
	//视频格式
	if (dev->fmt->code==MEDIA_BUS_FMT_UYVY8_1X16|| dev->fmt->code==MEDIA_BUS_FMT_UYVY8_2X8||
		dev->fmt->code==MEDIA_BUS_FMT_VYUY8_1X16|| dev->fmt->code==MEDIA_BUS_FMT_VYUY8_2X8||
		dev->fmt->code==MEDIA_BUS_FMT_YUYV8_1X16|| dev->fmt->code==MEDIA_BUS_FMT_YUYV8_2X8||
		dev->fmt->code==MEDIA_BUS_FMT_YVYU8_1X16|| dev->fmt->code==MEDIA_BUS_FMT_YVYU8_2X8)
	{	
		reg_addr=0x1001;
		reg_val=0x1e;	//YUV422 8-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_UYVY10_1X20|| dev->fmt->code==MEDIA_BUS_FMT_UYVY10_2X10||
		dev->fmt->code==MEDIA_BUS_FMT_VYUY10_1X20|| dev->fmt->code==MEDIA_BUS_FMT_VYUY10_2X10||
		dev->fmt->code==MEDIA_BUS_FMT_YUYV10_1X20|| dev->fmt->code==MEDIA_BUS_FMT_YUYV10_2X10||
		dev->fmt->code==MEDIA_BUS_FMT_YVYU10_1X20|| dev->fmt->code==MEDIA_BUS_FMT_YVYU10_2X10)
	{	
		reg_addr=0x1001;
		reg_val=0x1f;	//YUV422 10-bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_UYVY8_1_5X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_VYUY8_1_5X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_YUYV8_1_5X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_YVYU8_1_5X8)
	{	
		reg_addr=0x1001;
		reg_val=0x18;	//YUV420 8bit
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR8_1X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG8_1X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG8_1X8|| 
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB8_1X8)
	{	
		reg_addr=0x1001;
		reg_val=0x2a;	//RAW8
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10|| 
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
	{	
		reg_addr=0x1001;
		reg_val=0x2b;	//RAW10
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR12_1X12|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG12_1X12|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG12_1X12|| 
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB12_1X12)
	{	
		reg_addr=0x1001;
		reg_val=0x2c;	//RAW12
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR14_1X14|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG14_1X14|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG14_1X14|| 
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB14_1X14)
	{	
		reg_addr=0x1001;
		reg_val=0x2d;	//RAW14
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR16_1X16|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG16_1X16|| 
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG16_1X16|| 
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB16_1X16)
	{	
		reg_addr=0x1001;
		reg_val=0x2e;	//RAW16
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_RGB444_1X12|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_BE|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE)
	{	
		reg_addr=0x1001;
		reg_val=0x20;	//RGB444
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_BE|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB555_2X8_PADHI_LE)
	{	
		reg_addr=0x1001;
		reg_val=0x21;	//RGB555
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_RGB565_1X16|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB565_1X24_CPADHI|| 
		dev->fmt->code==MEDIA_BUS_FMT_BGR565_2X8_BE||
		dev->fmt->code==MEDIA_BUS_FMT_BGR565_2X8_LE||
		dev->fmt->code==MEDIA_BUS_FMT_RGB565_2X8_BE||
		dev->fmt->code==MEDIA_BUS_FMT_RGB565_2X8_LE)
	{	
		reg_addr=0x1001;
		reg_val=0x22;	//RGB565
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_BGR666_1X18|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB666_1X18)
	{	
		reg_addr=0x1001;
		reg_val=0x23;	//RGB666
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else if (dev->fmt->code==MEDIA_BUS_FMT_RGB888_1X24|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB888_2X12_BE|| 
		dev->fmt->code==MEDIA_BUS_FMT_RGB888_2X12_LE||
		dev->fmt->code==MEDIA_BUS_FMT_RGB888_3X8)
	{	
		reg_addr=0x1001;
		reg_val=0x24;	//RGB888
		ret = va2906_write_reg(client, addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	else
	{
		dev_err(&client->dev, "va2905 vpg unsupported format 0x%x",dev->fmt->code);
	}
	
	reg_addr=0x1004;
	reg_val=width & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1005;
	reg_val=width >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1006;
	reg_val=height & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1007;
	reg_val=height >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	hactive = width;
	vactive = height;
	vblank = 30;
	vtotal = vactive + vblank;
	vbp = vblank / 2;
	vfp = vblank - vbp;
	htotal = 50000000/(fps*vtotal) - hactive/4 + hactive;
	hblank = 50000000/(fps*vtotal) - hactive/4 - 1;
	hbp = hblank / 2;
	wc = hactive * 2;
	wc_odd = hactive * 2;
	//hbp
	reg_addr=0x1008;
	reg_val=hbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1009;
	reg_val=hbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//htotal 
	reg_addr=0x1010;
	reg_val=htotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1011;
	reg_val=htotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vbp 
	reg_addr=0x1014;
	reg_val=vbp & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1015;
	reg_val=vbp >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//vtotal 
	reg_addr=0x1012;
	reg_val=vtotal & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1013;
	reg_val=vtotal >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc 
	reg_addr=0x1041;
	reg_val=wc & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1042;
	reg_val=wc >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	//wc_odd 
	reg_addr=0x1063;
	reg_val=wc_odd & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x1064;
	reg_val=wc_odd >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x1000;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x vpg set regs failed %d",addr,ret);

	return ret;
}

static int va2905_vproc_compyuv422_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	unsigned int width=dev->fmt->width; 
	unsigned int height=dev->fmt->height;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;
	u32 video_size=0;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x,width=0x%x-height=0x%x\n", __func__, addr, width, height);
	reg_addr=0x4004;
	reg_val=0x03;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x4005;
	reg_val=0xa8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x4008;
	reg_val=width & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x4009;
	reg_val=width >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400a;
	reg_val=height & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400b;
	reg_val=height >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	video_size = width*height*16/8/4;
	reg_addr=0x400f;
	//reg_val=0x00;
	reg_val=(video_size>>24)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400e;
	//reg_val=0x07;
	reg_val=(video_size>>16)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400d;
	//reg_val=0x08;
	reg_val=(video_size>>8)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400c;
	//reg_val=0x00;
	reg_val=(video_size>>0)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x vproc comp regs failed %d",addr,ret);

	return ret;
}

static int va2905_vproc_compraw10_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	unsigned int width=dev->fmt->width; 
	unsigned int height=dev->fmt->height;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;
	u32 video_size=0;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x,width=0x%x-height=0x%x\n", __func__, addr, width, height);
	reg_addr=0x4004;
	reg_val=0x05;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x4005;
	reg_val=0xaa;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x4008;
	reg_val=width & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x4009;
	reg_val=width >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400a;
	reg_val=height & 0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400b;
	reg_val=height >> 8;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	video_size = width*height*10/8/4;
	pr_info("----video_size=0x%x-----\r\n",video_size);
	reg_addr=0x400f;
	//reg_val=0x00;
	reg_val=(video_size>>24)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400e;
	//reg_val=0x06;
	reg_val=(video_size>>16)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400d;
	//reg_val=0x01;
	reg_val=(video_size>>8)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x400c;
	//reg_val=0xbc;
	reg_val=(video_size>>0)&0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x vproc comp regs failed %d",addr,ret);

	return ret;
}

static int va2905_dev_tunnel_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<4, 0x1<<4);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;	
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	
	//配置SYS_CTRL视频通路选择寄存器
	reg_addr=0x902b;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<2, 0x0<<2);
	if (ret)
		goto error;
	
	//配置CSI2_HOST寄存器
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<6, 0x1<<6);
	if (ret)
		goto error;
	
	reg_addr=0x018c;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018d;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018e;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018f;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x0190;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, 0x8<<0);
	if (ret)
		goto error;
	reg_addr=0x0194;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x7<<0);
	if (ret)
		goto error;
	reg_addr=0x0254;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x2<<0);
	if (ret)
		goto error;
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x1<<0);
	if (ret)
		goto error;
	
	//配置PAL_CSI2寄存器
	reg_addr=0xe007;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置CDPHYRX寄存器
	dev->cdphyrx.init(&dev->cdphyrx);
	
	//配置SYS_CTRL寄存器打开CDPHYRX
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x38<<0);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x39<<0);
	if (ret)
		goto error;
	
	reg_addr=0xd004;	//sensor reset
	reg_val=0x20;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x tunnel init regs failed %d",addr,ret);

	return ret;
}

static int va2905_dev_comp_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val,lane_num;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	
#if VA2906_MPW
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<4, 0x0<<4);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x0<<3);
	if (ret)
		goto error;
#else
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<4, 0x1<<4);
	if (ret)
		goto error;
	reg_addr=0x902c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
#endif
	reg_addr=0x900c;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x9006;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	
	//配置SYS_CTRL视频通路选择寄存器
	reg_addr=0x902b;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<2, 0x1<<2);
	if (ret)
		goto error;
	
	//配置CSI2_HOST寄存器
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<6, 0x1<<6);
	if (ret)
		goto error;
	
	usleep_range(50000, 100000);
	ret = va2906_read_reg(client, addr, reg_addr, &reg_val);
	if (ret)
		goto error;
	if (reg_val & 1)
		pr_info("2905 %d csi_host is ready\r\n",addr);
	else
		pr_info("2905 %d csi_host is not ready\r\n",addr);
	
	reg_addr=0x018c;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018d;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018e;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x018f;
	reg_val=0xff;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	if (dev->lane_num)
		lane_num = dev->lane_num-1;
	
	reg_addr=0x0190;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1f<<0, (0x8|lane_num)<<0);
	if (ret)
		goto error;
	reg_addr=0x0194;
	ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x7<<0);
	if (ret)
		goto error;
	reg_addr=0x0254;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3<<0, 0x2<<0);
	if (ret)
		goto error;
	reg_addr=0x0184;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x1<<0);
	if (ret)
		goto error;
	
	if (dev->fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10||
		dev->fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10||
		dev->fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10||
		dev->fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
			dev->vproc_compraw10_init(dev);
	else if (dev->fmt->code==MEDIA_BUS_FMT_UYVY8_1X16||
		dev->fmt->code==MEDIA_BUS_FMT_VYUY8_1X16||
		dev->fmt->code==MEDIA_BUS_FMT_YUYV8_1X16||
		dev->fmt->code==MEDIA_BUS_FMT_YVYU8_1X16)
			dev->vproc_compyuv422_init(dev);
	else
		dev_err(&client->dev, "va2905 unsupported compress mode 0x%x",dev->fmt->code);
	//配置CSI2_DEVICE寄存器
	reg_addr=0x3104;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3136;
	reg_val=0x01;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3140;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	
	//配置PAL_CSI2寄存器
	reg_addr=0xe007;
	reg_val=0x00;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置CDPHYRX寄存器
	dev->cdphyrx.init(&dev->cdphyrx);
	
	//配置SYS_CTRL寄存器打开CDPHYRX
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x38<<0);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	reg_addr=0x9030;
	ret = va2906_mod_reg(client, addr, reg_addr, 0x3f<<0, 0x39<<0);
	if (ret)
		goto error;
	
	reg_addr=0xd004;
	reg_val=0x20;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x3138;
	reg_val=0x03;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x init regs failed %d",addr,ret);

	return ret;
}

static int va2905_dev_acmp_init(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x9036;
	reg_val=0x04;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x9041;
	reg_val=0x00;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x9042;
	reg_val=0x80;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x9040;
	reg_val=0xb3;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x acmp init regs failed %d",addr,ret);

	return ret;
}

static int va2905_dev_acmp_uninit(struct va2905_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 addr = dev->dev_addr;
	u8 reg_val;
	u16 reg_addr;

	va2906_dbg_verbose("%s: va2905->dev_addr:0x%x\n", __func__, addr);
	reg_addr=0x9040;
	reg_val=0xb2;
	ret = va2906_write_reg(client, addr, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2905 0x%x acmp uninit regs failed %d",addr,ret);

	return ret;
}

static int va2906_sensor_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int i;
	u8 reg_val;
	u16 reg_addr;
	
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		if (va2905->enable)
		{
			reg_addr=0x1009 + 0x400*i;
			reg_val=dev->slave_i2c_addr[i];
			if (reg_val)
				va2906_write_reg(client, 0, reg_addr, reg_val);
			
			reg_addr=0x1011 + 0x400*i;
			reg_val=dev->slave_alias_addr[i];
			if (reg_val)
				va2906_write_reg(client, 0, reg_addr, reg_val);
		}
	}
	return 0;
}

static int va2906_pal_gpio_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1,offset=0x400,i;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	//配置2906的L0_PAL_GPIO和L1_PAL_GPIO
	for (i=0; i<2; i++)
	{
		reg_addr=0x3007 + i*offset;
		reg_val=0x1e;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3008 + i*offset;
		reg_val=0x22;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3009 + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300a + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300b + i*offset;
		reg_val=0x3f;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300c + i*offset;
		reg_val=0x00;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300d + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300e + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x300f + i*offset;
		reg_val=0x3f;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3010 + i*offset;
		reg_val=0x00;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3011 + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3012 + i*offset;
		reg_val=0xff;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3013 + i*offset;
		reg_val=0x3f;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
		reg_addr=0x3014 + i*offset;
		reg_val=0x00;
		ret = va2906_write_reg(client, 0, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 pal gpio regs init failed %d",ret);

	return ret;
}

static int va2906_dev_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int ret=-1,i;
	u8 reg_val,lane_num;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	//2906配置SYS_CTRL时钟设置相关寄存器
	reg_addr=0x6c06;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x6c0c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	
#if VA2906_MPW
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<1, 0x0<<1);
	if (ret)
		goto error;
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<3, 0x0<<3);
	if (ret)
		goto error;
#else
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<1, 0x1<<1);
	if (ret)
		goto error;
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
#endif
	reg_addr=0x6c0c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x6c06;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	
	//配置L*_PAL_CSI寄存器
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		if (va2905->enable)
		{
			if (va2905->comp_enable)
			{
				//配置SYS_CTRL视频通路选择寄存器
				reg_addr=0x6c20;
				ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<i, 0x1<<i);
				if (ret)
					goto error;
				reg_addr=0x0007+i*0x400;
				reg_val=0x0;
				ret = va2906_write_reg(client, 0, reg_addr, reg_val);
				if (ret)
					goto error;
			}
			else
			{
				reg_addr=0x0007+i*0x400;
				reg_val=0x1;
				ret = va2906_write_reg(client, 0, reg_addr, reg_val);
				if (ret)
					goto error;
			}
		}
	}
	//配置L*_CSIHOST寄存器
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		lane_num = va2905->lane_num-1;
		if (va2905->enable && lane_num <= 3 && lane_num >=0)
		{
			reg_addr=0x4184 + 0x400*i;
			ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<6, 0x0<<6);
			if (ret)
				goto error;
			reg_addr=0x4190 + 0x400*i;
			ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<4, 0x0<<4);
			if (ret)
				goto error;
			ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0x1<<3|lane_num);
			if (ret)
				goto error;
			reg_addr=0x4194 + 0x400*i;
			ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0x7<<0);
			if (ret)
				goto error;
			reg_addr=0x4184 + 0x400*i;
			ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<0, 0x1<<0);
			if (ret)
				goto error;
		}
	}
	
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		if (va2905->enable && va2905->comp_enable)
		{
			//配置vproc寄存器
			reg_addr=0x5408;			//使能第n路link解压功能
			ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<i, 0x1<<i);
			if (ret)
				goto error;
		}
	}
	//配置LINEMEM寄存器
	reg_addr=0x5800;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0xf<<0);
	if (ret)
		goto error;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		if (va2905->enable)
		{
			ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<(4+i), va2905->port_id<<(4+i));
			if (ret)
				goto error;
		}			
	}
	reg_addr=0x5801;
	reg_val=0x03;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//4 pixel mode
	reg_addr=0x5808;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	if (dev->lane_num[0])
		lane_num = dev->lane_num[0]-1;
	
	//配置L*_CSIDEV寄存器
	reg_addr=0x5c3c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, (0x0|lane_num)<<0);
	if (ret)
		goto error;
	reg_addr=0x5d04;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d2c;
	reg_val=0x02;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d30;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d36;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x603c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, (0x0|lane_num)<<0);
	if (ret)
		goto error;
	reg_addr=0x6104;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x612c;
	reg_val=0x02;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6130;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6136;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置CDPHYTX0寄存器
	dev->cdphytx.init(&dev->cdphytx);
	
	//配置SYS_CTRL寄存器打开CDPHYTX0
	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xff<<0, 0x88<<0);
	if (ret)
		goto error;
	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xff<<0, 0xcc<<0);
	if (ret)
		goto error;
	
#if VA2906_PORT2_COPY_SEL
	reg_addr=0x6c21;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<4, 0x1<<4);
	if (ret)
		goto error;
#endif
#if VA2906_CDPHYTX2_ENABLE
	reg_addr=0x6c27;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0x8<<0);
	if (ret)
		goto error;
	reg_addr=0x6c27;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0xc<<0);
	if (ret)
		goto error;
#endif

	reg_addr=0x002c;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x042c;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x082c;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x0c2c;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 init regs failed %d",ret);

	return ret;
}

static int va2906_dev_tunnel_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	//2906配置SYS_CTRL时钟设置相关寄存器
	reg_addr=0x6c06;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<2, 0x0<<2);
	if (ret)
		goto error;
	reg_addr=0x6c0c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x7<<3, 0x6<<3);
	if (ret)
		goto error;
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<1, 0x1<<1);
	if (ret)
		goto error;
	reg_addr=0x6c24;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x6c0c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x7<<3, 0x1<<3);
	if (ret)
		goto error;
	reg_addr=0x6c06;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	usleep_range(20000, 25000);
	
	//配置SYS_CTRL视频通路选择寄存器
	reg_addr=0x6c1f;
	ret = va2906_mod_reg(client, 0, reg_addr, 0x1<<2, 0x1<<2);
	if (ret)
		goto error;
	reg_addr=0x6c20;
	//reg_val=0xc0;//0xe0;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<4, 0xc<<4);
	if (ret)
		goto error;
	
	//配置L*_PAL_CSI寄存器
	reg_addr=0x0007;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x4007;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置L*_CSIDEV寄存器
	reg_addr=0x5c3c;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0x0<<0);
	if (ret)
		goto error;
	
	//for rgb
	reg_addr=0x5808;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x5d04;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d2c;
	reg_val=0x02;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x5d30;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d36;
	reg_val=0x02;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x5d40;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置CDPHYTX0寄存器
	dev->cdphytx.init(&dev->cdphytx);
	
	//配置SYS_CTRL寄存器打开CDPHYTX0
	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0x8<<0);
	if (ret)
		goto error;
	reg_addr=0x6c26;
	ret = va2906_mod_reg(client, 0, reg_addr, 0xf<<0, 0xc<<0);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 tunnel init regs failed %d",ret);

	return ret;
}

static int va2906_fsync_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int ret=-1,i;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	//拼接模式
	reg_addr=0x5801;
	if (dev->fsync_mode==1)
		reg_val=0x31;
	if (dev->fsync_mode==2)
		reg_val=0x21;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	//fpga环境下需要将2906和2905的gpio pal的采样时钟wclk配到50M
	reg_addr=0x6c24;
	reg_val=0x75;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x902c;
	reg_val=0x67;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	
	//在2906_SYS_REG里配置2906的mfp6 pin模式
	reg_addr=0x6c1c;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置2906的gpio controller
	//关闭mfp6的输出使能，并将mfp6的输入数据传进gpio pal
	reg_addr=0xc006;
	reg_val=0x28;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	//选择gpp0_out5的输出来源为MFP6
	reg_addr=0xc019;
	reg_val=0x06;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	//选择gpp1_out5的输出来源为MFP6
	reg_addr=0xc024;
	reg_val=0x06;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置u0_2905和u1_2905的gpio controller
	//打开mfp5输出使能
	reg_addr=0xd005;
	reg_val=0x00;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	usleep_range(20000, 25000);
	
	reg_addr=0xc09b;
	reg_val=0x02;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	
	//enable pal gpio tx rx
	reg_addr=0x309b;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x349b;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x6c04;
	reg_val=0x45;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6c3b;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6c3b;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x9004;
	reg_val=0x48;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, reg_val);
		if (ret)
			goto error;
	}
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 vsync regs failed %d",ret);

	return ret;
}

static int va2906_acmp_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int ret=-1,i;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	reg_addr=0x6c48;
	reg_val=0x60;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x6c5f;
	reg_val=0xff;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	reg_addr=0x6c60;
	reg_val=0xff;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2905->acmp_init(va2905);
		if (ret)
			goto error;
	}
	reg_addr=0x6c5e;
	reg_val=0x58;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 acmp regs failed %d",ret);

	return ret;
}

static int va2906_acmp_uninit(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int ret=-1,i;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		ret = va2905->acmp_uninit(va2905);
		if (ret)
			goto error;
	}
	reg_addr=0x6c5e;
	reg_val=0x18;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 acmp regs failed %d",ret);

	return ret;
}

static int va2906_adc_init(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 reg_val;
	u16 reg_addr;
	
	va2906_dbg_verbose("%s\n", __func__);
	//ADC时钟频率为12.5M , from root_clk
	reg_addr=0x6c18;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//配置启动时间，外部参考电压为5us，若ADC时钟频率为12.5M
	reg_addr=0x6803;
	reg_val=0x3f;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6804;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6805;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6806;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//通道0触发中断的值自行配置 ，不同的通道地址不同
	reg_addr=0x6810;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6811;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6820;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6821;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6801;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6850;
	reg_val=0x04;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	reg_addr=0x6851;
	reg_val=0x00;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//启动ADC和循环采样模式
	reg_addr=0x6800;
	reg_val=0x03;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
	//启动ADC转换模式
	reg_addr=0x6802;
	reg_val=0x01;
	ret = va2906_write_reg(client, 0, reg_addr, reg_val);
	if (ret)
		goto error;
	
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 adc regs failed %d",ret);

	return ret;
}
/*
static int va2906_adc_readdata(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	int ret=-1;
	u8 reg_val;
	u16 reg_addr;
	
	pr_info("-------va2906_adc_readdata-----\r\n");
	reg_addr=0x6870;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----0---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6871;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----1---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6872;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----2---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6873;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----3---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6874;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----4---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6875;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----5---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6876;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----6---reg_val=0x%x------\r\n",reg_val);
	
	
	reg_addr=0x6877;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----7---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6878;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----8---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x6879;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----9---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x687a;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----a---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x687b;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----b---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x687c;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----c---reg_val=0x%x------\r\n",reg_val);
	reg_addr=0x687d;
	ret = va2906_read_reg(client, 0, reg_addr, &reg_val);
	if (ret)
		goto error;
	pr_info("----d---reg_val=0x%x------\r\n",reg_val);
error:
	if (ret!=0)
		dev_err(&client->dev, "va2906 adc read regs failed %d",ret);

	return ret;
}
*/
static int va2906_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret=0;
	
	dev_dbg(&client->dev, "%s va2906 power %d \n", __func__,on);
	pr_info("---------va2906_s_power-------\r\n");

	return ret;
}

static int va2906_set_vc_id(struct va2906_dev *dev, int index, int channel)
{
	struct i2c_client *client = dev->i2c_client;
	u16 reg_addr=0x5880;

	reg_addr+=index*0x20;
	return va2906_write_reg(client, 0, reg_addr, channel);
}

static int va2906_soft_reset(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	u8 val;
	int i;
	u16 reg_addr,reg_val;
	
	va2906_dbg_verbose("%s\n", __func__);
	reg_addr=0x0700;
	reg_val=0x0000;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		va2906_write_reg16(va2905->i2c_client, va2905->dev_addr, reg_addr, reg_val);
	}
	
	reg_addr=0x9007;
	val=0xde;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, val);
	}
	
	val=0xfd;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, val);
	}
	
	val=0xff;
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&dev->va2905[i];
		va2906_write_reg(va2905->i2c_client, va2905->dev_addr, reg_addr, val);
	}
	
	reg_addr=0x6c07;
	val=0xfd;
	va2906_write_reg(client, 0, reg_addr, val);
	val=0xff;
	va2906_write_reg(client, 0, reg_addr, val);

	return 0;
}

static int va2906_stream_disable(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	int ret;
	int i=0,count=0;

	dev_dbg(&client->dev, "%s va2906 stop sensor streaming \n", __func__);
	pr_info("------va2906_stream_disable-----\r\n");

	pm_runtime_put(&client->dev);
	for (i = 0; i < dev->sensor_num; i++)
	{
		pr_info("------va2906_stream_disable--111---\r\n");
		dev_dbg(&client->dev, "%s va2906 stop sensor streaming \n", __func__);
		ret = v4l2_subdev_call(dev->s_subdev[i], video, s_stream, 0);
		if (ret < 0) 
		{
			dev_err(&client->dev, "va2906 stream off failed in subdev %d", ret);
			goto stream_error;
		}
	}
	
stream_error:
	if (ret)
		dev_err(&client->dev, "va2906 failed to stream off %d", ret);
	
reset_try:
	if (count++ < 5 && dev->soft_reset==1)
	{
		if (dev->acmp_mode)
		{
			va2906_acmp_uninit(dev);
			mutex_lock(&dev->mc_lock);
			dev->acmp_enable = 0;
			mutex_unlock(&dev->mc_lock);
		}
		va2906_soft_reset(dev);
		va2906_sensor_init(dev);
		ret = va2906_write_array(dev, 0, va2906_init_regs, ARRAY_SIZE(va2906_init_regs));
		if (ret < 0) 
		{
			dev_err(&client->dev, "va2906 write va2906_init_regs error\n");
			goto reset_try;
		}
		
		for (i=0; i<VA2905_CURR_NUM; i++)
		{
			va2905=&dev->va2905[i];
			if (va2905->tunnel_mode)
				ret = va2905->tunnel_init(va2905);
			else
				ret = va2905->dev_init(va2905);
			if (ret != 0)
				goto reset_try;
			ret = va2905->pal_gpio_init(va2905);
			if (ret != 0)
				goto reset_try;
		}
	
		if (dev->va2905[0].tunnel_mode || dev->va2905[1].tunnel_mode)
			ret = va2906_dev_tunnel_init(dev);
		else
			ret = va2906_dev_init(dev);
		if (ret != 0)
			goto reset_try;
		ret = va2906_pal_gpio_init(dev);
		if (ret != 0)
			goto reset_try;
		if (dev->fsync_mode)
		{
			ret = va2906_fsync_init(dev);
			if (ret != 0)
				goto reset_try;
		}
		if (dev->acmp_mode && !dev->acmp_enable)
		{
			va2906_acmp_init(dev);
			mutex_lock(&dev->mc_lock);
			dev->acmp_mc_2906 = 0;
			dev->acmp_mc_2905_1 = 0;
			dev->acmp_mc_2905_2 = 0;
			dev->acmp_enable = 1;
			mutex_unlock(&dev->mc_lock);
		}
	}
	return ret;
}

static int va2906_stream_enable(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;
	struct media_link *link;
	const struct media_pad *remote_pad;
	int ret = -EINVAL;
	int i=0;

	pr_info("------va2906_stream_enable---sensor_num=%d--\r\n",dev->sensor_num);
	dev_dbg(&client->dev, "%s va2906 starting sensor streaming \n", __func__);
	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		goto error;
	
	for (i=VA2906_SINK_0; i<VA2906_PAD_NUM; i++)
	{
		for_each_media_entity_data_link(dev->pad[i].entity, link) 
		{
			if (!(link->flags & MEDIA_LNK_FL_ENABLED))
				continue;

			if (link->sink == &dev->pad[i])
			{
				struct v4l2_subdev *subdev;
				
				remote_pad = link->source;
				//pr_info("------index=%d--\r\n",remote_pad->index);
				//if (remote_pad->entity)
				//	pr_info("------name=%s--\r\n",remote_pad->entity->name);
				
				subdev = media_entity_to_v4l2_subdev(remote_pad->entity);
				if (subdev)
				{
					ret = v4l2_subdev_call(subdev, video, s_stream, 1);
					if (ret < 0) 
					{
						dev_err(&client->dev, "va2906 stream on failed in subdev %d", ret);
						goto stream_error;
					}
				}
			}
		}
	}
	return 0;

stream_error:
	
error:
	dev_err(&client->dev, "va2906 failed to stream on %d", ret);
	return ret;
}

static int va2906_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;
	int ret = 0;

	dev_dbg(&client->dev, "%s : va2906 requested %d / current = %d", __func__, enable, dev->streaming);
	pr_info("------va2906_s_stream---enable=%d----\r\n",enable);

	mutex_lock(&dev->lock);
	if (dev->streaming == enable)
		goto out;
	ret = enable ? va2906_stream_enable(dev) : va2906_stream_disable(dev);
	if (!ret)
		dev->streaming = enable;
out:
	dev_dbg(&client->dev, "%s va2906 current now = %d / %d", __func__, dev->streaming, ret);
	mutex_unlock(&dev->lock);
	return ret;
}

static void va2906_init_format(struct va2906_dev *dev, struct v4l2_mbus_framefmt *fmt)
{
	int i;
	
	mutex_lock(&dev->lock);
	for (i=0; i<VA2906_PAD_NUM; i++)
	{
		fmt[i].code = MEDIA_BUS_FMT_SRGGB10_1X10;
		fmt[i].field = V4L2_FIELD_NONE;
		fmt[i].colorspace = V4L2_COLORSPACE_SRGB;
		fmt[i].ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(V4L2_COLORSPACE_SRGB);
		fmt[i].quantization = V4L2_QUANTIZATION_FULL_RANGE;
		fmt[i].xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(V4L2_COLORSPACE_SRGB);
		fmt[i].width = 1280;
		fmt[i].height = 720;
	}
	mutex_unlock(&dev->lock);
}

static int va2906_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *sd_state,
									struct v4l2_subdev_mbus_code_enum *code)
{
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;
	int ret = 0;

	dev_dbg(&client->dev, "%s  %d", __func__, code->index);
	if (code->index >= ARRAY_SIZE(va2906_mbus_codes))
		ret = -EINVAL;
	else
		code->code = va2906_mbus_codes[code->index];
	
	return ret;
}

static int va2906_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *sd_state,
							struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mbus_fmt = &format->format;
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;
	struct v4l2_mbus_framefmt *fmt;

	dev_dbg(&client->dev, "%s  %d", __func__, format->pad);
	if (format->pad >= VA2906_PAD_NUM)
		return -EINVAL;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt = v4l2_subdev_get_try_format(&dev->sd, sd_state, format->pad);
	else
		fmt = &dev->fmt[format->pad];
	
	mutex_lock(&dev->lock);
	*mbus_fmt = *fmt;
	mutex_unlock(&dev->lock);
	return 0;
}

static void __va2906_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *sd_state,
								struct v4l2_subdev_format *format)
{
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;
	struct va2905_dev *va2905;
	struct v4l2_mbus_framefmt *mbus_format = &format->format;
	struct v4l2_mbus_framefmt *fmt;
	u32 mbus_code = 0;
	unsigned int i,pad;
	
	pr_info("------__va2906_set_fmt------\r\n");
	for (i = 0; i < ARRAY_SIZE(va2906_mbus_codes); i++) 
	{
		if (va2906_mbus_codes[i] == mbus_format->code) 
		{
			mbus_code = mbus_format->code;
			break;
		}
	}
	if (!mbus_code)
		mbus_code = va2906_mbus_codes[0];
	format->format.code = mbus_code;
	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt = v4l2_subdev_get_try_format(sd, sd_state, format->pad);
	else
		fmt = &dev->fmt[format->pad];

	pr_info("------__va2906_set_fmt---format->pad=%d---\r\n",format->pad);
	*fmt = format->format;
	dev->mbus_code = mbus_code;
	if (format->pad >= VA2906_SINK_0)
	{
		pad = format->pad - VA2906_SINK_0;
		pr_info("------__va2906_set_fmt---pad=%d---\r\n",pad);
		va2905=&dev->va2905[pad];
		*va2905->fmt = *fmt;
		if (va2905->comp_enable)
		{
			if (va2905->fmt->code==MEDIA_BUS_FMT_SBGGR10_1X10||
				va2905->fmt->code==MEDIA_BUS_FMT_SGBRG10_1X10||
				va2905->fmt->code==MEDIA_BUS_FMT_SGRBG10_1X10||
				va2905->fmt->code==MEDIA_BUS_FMT_SRGGB10_1X10)
					va2905->vproc_compraw10_init(va2905);
			else if (va2905->fmt->code==MEDIA_BUS_FMT_UYVY8_1X16||
				va2905->fmt->code==MEDIA_BUS_FMT_VYUY8_1X16||
				va2905->fmt->code==MEDIA_BUS_FMT_YUYV8_1X16||
				va2905->fmt->code==MEDIA_BUS_FMT_YVYU8_1X16)
					va2905->vproc_compyuv422_init(va2905);
			else
				dev_err(&client->dev, "va2905 unsupported compress mode 0x%x",va2905->fmt->code);
		}
	}
}

static int va2906_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *sd_state,
							struct v4l2_subdev_format *format)
{
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;
	int ret = 0;

	dev_dbg(&client->dev, "%s  %d", __func__, format->pad);
	if (format->pad >= VA2906_PAD_NUM)
		return -EINVAL;

	mutex_lock(&dev->lock);
	if (dev->streaming) 
	{
		ret = -EBUSY;
		goto error;
	}
	__va2906_set_fmt(sd, sd_state, format);
	
error:
	mutex_unlock(&dev->lock);
	return ret;
}

static int va2906_get_mbus_config(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_mbus_config *cfg)
{
	struct va2906_dev *dev = to_va2906_dev(sd);
	struct i2c_client *client = dev->i2c_client;

	dev_dbg(&client->dev, "%s  %d", __func__, pad);
	cfg->type = V4L2_MBUS_CSI2_DPHY;

	return 0;
}

static unsigned long va2906_mode_mipi_clk_rate(void)
{
	return VA2906_LINK_FREQ_500MHZ;
}

static u64 va2906_calc_pixel_rate(struct va2906_dev *dev)
{
	s64 link_freq = VA2906_LINK_FREQ_500MHZ;
	unsigned int bits_per_sample;
	unsigned long mipi_clk_rate;
	u8 nlanes = dev->nlanes;
	u32 mbus_code,i;
	u64 pixel_rate;
	
	mipi_clk_rate = va2906_mode_mipi_clk_rate();
	if (!mipi_clk_rate)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(link_freq_items); i++) 
	{
		link_freq = link_freq_items[i];
		if (link_freq == mipi_clk_rate)
			break;
	}
	switch (mbus_code) 
	{
		case MEDIA_BUS_FMT_SBGGR8_1X8:
			bits_per_sample = 8;
			break;
		case MEDIA_BUS_FMT_SBGGR10_1X10:
			bits_per_sample = 10;
			break;
		default:
			bits_per_sample = 8;
	}

	pixel_rate = link_freq * 2 * nlanes / bits_per_sample;
	do_div(pixel_rate, dev->bpp);
	return pixel_rate;
}

static int va2906_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct va2906_dev *dev = container_of(ctrl->handler, struct va2906_dev, ctrls);
	struct va2905_dev *va2905;
	struct i2c_client *client = dev->i2c_client;
	int ret = 0,i;
	u8 reg_val;
	u16 reg_addr;
	unsigned int index,value;
	
	pr_info("-------va2906_s_ctrl------\r\n");
	if(!dev->is_ready)
		return 0;
	switch (ctrl->id) 
	{
		case V4L2_CID_FSYNC_MODE:
			value = (unsigned int)ctrl->val;
			pr_info("-----fsync_mode=%d-------\r\n",value);
			reg_addr=0x5801;
			if (value==0)
			{
				reg_val=0x01;
				va2906_write_reg(client, 0, reg_addr, reg_val);
			}
			if (value==1)
			{
				reg_val=0x31;
				va2906_write_reg(client, 0, reg_addr, reg_val);
			}
			if (value==2)
			{
				reg_val=0x21;
				va2906_write_reg(client, 0, reg_addr, reg_val);
			}
			return 0;
		default:
			break;
	}
	if (pm_runtime_get_if_in_use(&client->dev) == 0)
		return 0;
	pr_info("-------va2906_s_ctrl---id=0x%x---\r\n",ctrl->id);
	dev_dbg(&client->dev, "%s  ctrl->id %d", __func__, ctrl->id);
	switch (ctrl->id) 
	{
		case V4L2_CID_LINK_FREQ:
			break;
		case V4L2_CID_PIXEL_RATE:
			break;
		case V4L2_CID_TEST_PATTERN:
			index = (unsigned int)ctrl->val;
			
			if (index >= ARRAY_SIZE(test_pattern_bits))
				ret= -EINVAL;
			if (index == 0)
			{
				struct i2c_client *client = dev->i2c_client;
				u8 addr = 0;
				u16 reg_addr;
				
				reg_addr=0x5400;
				ret = va2906_mod_reg(client, addr, reg_addr, 0xf<<0, 0x0<<0);
				reg_addr=0x5000;
				ret = va2906_write_reg(client, addr, reg_addr, 0x00);
				reg_addr=0x4000;
				for (i=0; i<VA2905_CURR_NUM; i++)
				{
					va2905=&dev->va2905[i];
					if (va2905->enable)
					{
						addr = va2905->dev_addr;
						ret = va2906_mod_reg(client, addr, reg_addr, 0x1<<0, 0x0<<0);
						ret = va2906_write_reg(client, addr, 0x1000, 0x00);
					}
				}
			}
			else if (index == 1)
			{
				va2906_vpg_init(dev);
			}
			else if (index == 2)
			{
				va2905=&dev->va2905[0];
				va2905->vpg_init(va2905);
			}
			else if (index == 3)
			{
				va2905=&dev->va2905[1];
				va2905->vpg_init(va2905);
			}
			else if (index == 4)
			{
				va2905=&dev->va2905[2];
				va2905->vpg_init(va2905);
			}
			else if (index == 5)
			{
				va2905=&dev->va2905[3];
				va2905->vpg_init(va2905);
			}
			else if (index == 6)
			{
				va2906_2_vpg_init(dev);
			}
			else
			{
				
			}
			break;
		default:
			dev_info(&client->dev,"Control (id:0x%x, val:0x%x) not supported\n", ctrl->id, ctrl->val);
			return -EINVAL;
	}
	pm_runtime_put(&client->dev);
	return ret;
}

static const struct v4l2_ctrl_ops va2906_ctrl_ops = {
	.s_ctrl = va2906_s_ctrl,
};

static const struct v4l2_subdev_core_ops va2906_core_ops = {
	.s_power = va2906_s_power,
};

static const struct v4l2_subdev_video_ops va2906_video_ops = {
	.s_stream = va2906_s_stream,
};

static const struct v4l2_subdev_pad_ops va2906_pad_ops = {
	.enum_mbus_code = va2906_enum_mbus_code,
	.get_fmt = va2906_get_fmt,
	.set_fmt = va2906_set_fmt,
	.get_mbus_config = va2906_get_mbus_config,
};

static const struct v4l2_subdev_ops va2906_subdev_ops = {
	.core 	= &va2906_core_ops,
	.video 	= &va2906_video_ops,
	.pad 	= &va2906_pad_ops,
};

static const struct media_entity_operations va2906_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_ctrl_config fsync_mode_ctrl = {
	.ops = &va2906_ctrl_ops,
	.id = V4L2_CID_FSYNC_MODE,
	.name = "fsync mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min  = 0,
	.max  = 2,
	.step = 1,
	.def  = 0,
};

static int va2906_async_bound(struct v4l2_async_notifier *notifier, struct v4l2_subdev *s_subdev,
								struct v4l2_async_subdev *asd)
{
	struct va2906_dev *dev = to_va2906_dev(notifier->sd);
	struct i2c_client *client = dev->i2c_client;
	int source_pad,ret,i;
	int sink_pad = VA2906_SINK_0;
	char *result;
	char *result_id;

	dev_dbg(&client->dev, "va2906 sensor_async_bound call %p", s_subdev);
	if (!dev->is_ready)
		return -1;
	source_pad = media_entity_get_fwnode_pad(&s_subdev->entity, s_subdev->fwnode, MEDIA_PAD_FL_SOURCE);
	if (source_pad < 0) 
	{
		dev_err(&client->dev, "va2906 couldn't find output pad for subdev %s\n", s_subdev->name);
		return source_pad;
	}
	for (i=0; i<SLAVE_MAX_NUM; i++)
    {
		if (dev->sensor_name[i]==NULL || dev->sensor_id[i]==NULL)
			continue;
		result = strstr(s_subdev->entity.name, dev->sensor_name[i]);
		result_id = strstr(s_subdev->entity.name, dev->sensor_id[i]);
		if (result && result_id)
			break;
	}
	sink_pad = dev->sensor_index[i];
	ret = media_create_pad_link(&s_subdev->entity, source_pad, &dev->sd.entity, sink_pad, MEDIA_LNK_FL_ENABLED );
	if (ret) 
	{
		dev_err(&client->dev, "va2906 couldn't create media link %d", ret);
		return ret;
	}
	sink_pad = VA2906_SINK_4;
	ret = media_create_pad_link(&s_subdev->entity, source_pad, &dev->sd.entity, sink_pad, 0);
	if (ret) 
	{
		dev_err(&client->dev, "va2906 couldn't create media link %d", ret);
		return ret;
	}
	if (dev->sensor_num < SLAVE_MAX_NUM)
		dev->s_subdev[dev->sensor_num++] = s_subdev;
	return 0;
}

static void va2906_async_unbind(struct v4l2_async_notifier *notifier, struct v4l2_subdev *s_subdev,
									struct v4l2_async_subdev *asd)
{
	int i = 0;
	struct va2906_dev *dev = to_va2906_dev(notifier->sd);

	if (!dev->is_ready)
		return;
	for (i = 0; i < VA2905_MAX_NUM; i++)
		dev->s_subdev[i] = NULL;
	dev->sensor_num --;
}

static const struct v4l2_async_notifier_operations va2906_notifier_ops = {
	.bound	= va2906_async_bound,
	.unbind	= va2906_async_unbind,
};

static int va2906_parse_rx_ep(struct va2906_dev *dev)
{
	struct v4l2_mbus_config_mipi_csi2 *bus_mipi_csi2;
	struct v4l2_fwnode_endpoint ep = { .bus_type = V4L2_MBUS_CSI2_DPHY };
	struct i2c_client *client = dev->i2c_client;
	struct device *pdev = &client->dev;
	struct v4l2_async_subdev *asd=NULL;
	struct device_node *ep_node[SLAVE_MAX_NUM];
	struct device_node *ep_renode[SLAVE_MAX_NUM];
	unsigned int lanes_count_rx=VA2906_LANE_2;
	int ret,i,j,index=VA2906_SINK_0;
	u32 alias_addr=0,i2c_addr=0,vc_id=0,compress_enable=0,tunnel_mode=0,port_id=0;
	struct of_endpoint of_ep;
	struct device_node *node;
	const char *compatible;
	int asd_init=0;

	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		ep_node[i] = of_graph_get_endpoint_by_regs(dev->i2c_client->dev.of_node, index, 0);
		if (!ep_node[i])
		{
			index++;
			continue;
		}
		else
		{
			break;
		}
	}
	
	for (; i<VA2905_CURR_NUM; i++)
	{
		ep_node[i] = of_graph_get_endpoint_by_regs(dev->i2c_client->dev.of_node, index, 0);
		if (!ep_node[i])
		{
			break;
		}
		ep_renode[i] = of_graph_get_remote_endpoint(ep_node[i]);
		if (ep_renode[i])
		{
			node = of_get_parent(ep_renode[i]);
			if (node)
			{
				node = of_get_parent(node);
				if (node)
				{
					of_graph_parse_endpoint(node, &of_ep);
					dev->sensor_name[i] = devm_kasprintf(pdev, GFP_KERNEL, "%s", node->name ? node->name : "");
					dev->sensor_id[i] = devm_kasprintf(pdev, GFP_KERNEL, "%x", of_ep.id);
					dev->sensor_index[i]=index;
					//pr_info("------name=%s---id=%s------",dev->sensor_name[i],dev->sensor_id[i]);
					of_property_read_string(node,"compatible",&compatible);
					//pr_info("--------compatible=%s----\r\n",compatible);
				}
			}
		}
		ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(ep_node[i]), &ep);
		if (ret) 
		{
			dev_err(&client->dev, "va2906 could not parse v4l2 port1 endpoint %d\n", ret);
			goto error_of_node_put;
		}
		dev->rx[i] = ep;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "alias-addr", &alias_addr);
		if (ret==0)
			dev->slave_alias_addr[i]=alias_addr;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "i2c-addr", &i2c_addr);
		if (ret==0)
			dev->slave_i2c_addr[i]=i2c_addr;
		dev->va2905[i].vc_id=0;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "vc-id", &vc_id);
		if (ret==0)
			dev->va2905[i].vc_id=vc_id;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "compress-enable", &compress_enable);
		if (ret==0)
			dev->va2905[i].comp_enable=compress_enable;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "tunnel-mode", &tunnel_mode);
		if (ret==0)
			dev->va2905[i].tunnel_mode=tunnel_mode;
		dev->va2905[i].port_id=0;
		ret=fwnode_property_read_u32(of_fwnode_handle(ep_node[i]), "port-map", &port_id);
		if (ret==0)
			dev->va2905[i].port_id=port_id;
		bus_mipi_csi2 =&ep.bus.mipi_csi2;
		lanes_count_rx = bus_mipi_csi2->num_data_lanes;
		dev->va2905[i].lane_num = lanes_count_rx;
		dev->va2905[i].enable = 1;
		pr_info("--i=%d-lane_num=%d---port_id=%d--vc_id=%d--\r\n",i,dev->va2905[i].lane_num,dev->va2905[i].port_id,dev->va2905[i].vc_id);
		if (asd_init==0)
		{
			v4l2_async_nf_init(&dev->notifier);
			asd_init = 1;
		}
		asd = v4l2_async_nf_add_fwnode_remote(&dev->notifier, of_fwnode_handle(ep_node[i]), struct v4l2_async_subdev);
		of_node_put(ep_node[i]);
		index += 1;
	}
	if (IS_ERR(asd)) 
	{
		dev_err(&client->dev, "va2906 fail to register asd to notifier %ld", PTR_ERR(asd));
		return PTR_ERR(asd);
	}
	dev->notifier.ops = &va2906_notifier_ops;
	ret = v4l2_async_subdev_nf_register(&dev->sd, &dev->notifier);
	if (ret)
		v4l2_async_nf_cleanup(&dev->notifier);
	return ret;

error_of_node_put:
	for (j=0; j<i; j++)
		of_node_put(ep_node[j]);
	return ret;
}

static int va2906_parse_tx_ep(struct va2906_dev *dev)
{
	struct v4l2_mbus_config_mipi_csi2 *bus_mipi_csi2;
	struct v4l2_fwnode_endpoint ep = { .bus_type = V4L2_MBUS_CSI2_DPHY };
	struct i2c_client *client = dev->i2c_client;
	struct device_node *ep_node;
	unsigned int lanes_count_tx=VA2906_LANE_2;
	int ret;

	ep_node = of_graph_get_endpoint_by_regs(dev->i2c_client->dev.of_node, 0, 0);
	if (!ep_node) 
	{
		dev_err(&client->dev, "va2906 unable to find port0 ep");
		ret = -EINVAL;
		goto error;
	}
	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(ep_node), &ep);
	if (ret) 
	{
		dev_err(&client->dev, "va2906 could not parse v4l2 port0 endpoint\n");
		goto error_of_node_put;
	}
	of_node_put(ep_node);
	dev->tx = ep;
	bus_mipi_csi2 =&ep.bus.mipi_csi2;
	lanes_count_tx = bus_mipi_csi2->num_data_lanes;
	dev->lane_num[0] = lanes_count_tx;
	dev->nlanes = lanes_count_tx;
	return 0;

error_of_node_put:
	of_node_put(ep_node);
error:

	return -EINVAL;
}

static int va2906_init_controls(struct va2906_dev *dev)
{
	struct i2c_client *client = dev->i2c_client;

	v4l2_ctrl_handler_init(&dev->ctrls, 4);
	dev->link_freq = v4l2_ctrl_new_int_menu(&dev->ctrls, &va2906_ctrl_ops,
				V4L2_CID_LINK_FREQ, ARRAY_SIZE(link_freq_items) - 1, 2, link_freq_items);			
	v4l2_ctrl_s_ctrl(dev->link_freq,0);
	dev->pixel_rate = v4l2_ctrl_new_std(&dev->ctrls, &va2906_ctrl_ops,
				V4L2_CID_PIXEL_RATE, 1, INT_MAX, 1, va2906_calc_pixel_rate(dev));
	v4l2_ctrl_s_ctrl_int64(dev->pixel_rate, va2906_calc_pixel_rate(dev));
	dev->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	dev->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	dev->fsync_ctrl = v4l2_ctrl_new_custom(&dev->ctrls, &fsync_mode_ctrl, NULL);
	v4l2_ctrl_new_std_menu_items(&dev->ctrls, &va2906_ctrl_ops, V4L2_CID_TEST_PATTERN,
				ARRAY_SIZE(test_pattern_menu) - 1, 0, ARRAY_SIZE(test_pattern_menu) - 1, test_pattern_menu);
	if (dev->ctrls.error)
		goto handler_free;
	dev->sd.ctrl_handler = &dev->ctrls;
	return 0;

handler_free:
	dev_err(&client->dev, "%s Controls initialization failed (%d)\n", __func__, dev->ctrls.error);
	v4l2_ctrl_handler_free(&dev->ctrls);
	return dev->ctrls.error;
}

static ssize_t lane_num_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2905->lane_num;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t port_map_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2905->port_id;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t vc_id_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2905->vc_id;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t vc_id_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	va2905->vc_id = value;
	va2906_set_vc_id(va2906,va2905_attr->index,va2905->vc_id);
    return count;
}

static ssize_t tunnel_mode_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2905->tunnel_mode;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t tunnel_mode_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	va2905->tunnel_mode = value;
    return count;
}

static ssize_t comp_enable_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2905->comp_enable;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t comp_enable_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct va2905_attribute *va2905_attr = to_va2905_attr(attr);
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	struct va2905_dev *va2905=&va2906->va2905[va2905_attr->index];
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	va2905->comp_enable = value;
    return count;
}

static ssize_t soft_reset_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2906->soft_reset;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t soft_reset_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	va2906->soft_reset = value;
    return count;
}

static ssize_t fsync_mode_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2906->fsync_mode;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t fsync_mode_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	va2906->fsync_mode = value;
    return count;
}

static ssize_t acmp_mode_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2906->acmp_mode;
	return sprintf(buf, "0x%x\n", value);
}

static ssize_t acmp_mode_store(struct device *device, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value,ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	if (value)
	{
		va2906_acmp_init(va2906);
		mutex_lock(&va2906->mc_lock);
		va2906->acmp_mc_2906 = 0;
		va2906->acmp_mc_2905_1 = 0;
		va2906->acmp_mc_2905_2 = 0;
	}
	else 
	{
		va2906_acmp_uninit(va2906);
		mutex_lock(&va2906->mc_lock);
	}
	va2906->acmp_mode = value;
	va2906->acmp_enable = value;
	mutex_unlock(&va2906->mc_lock);
    return count;
}

static ssize_t tx_lane_num_show(struct device *device, struct device_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(device->kobj.parent);
    struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value;
	
	if (!va2906->is_ready)
		return -1;
	value = va2906->lane_num[0];
	return sprintf(buf, "0x%x\n", value);
}

static DEVICE_ATTR_RW(soft_reset);
static DEVICE_ATTR_RW(fsync_mode);
static DEVICE_ATTR_RW(acmp_mode);
static DEVICE_ATTR_RO(tx_lane_num);

static struct attribute *va2906_attrs[] = {
	&dev_attr_soft_reset.attr,
	&dev_attr_fsync_mode.attr,
	&dev_attr_acmp_mode.attr,
	&dev_attr_tx_lane_num.attr,
	NULL,
};

static const struct attribute_group va2906_attr_group = {
    .attrs = va2906_attrs
};

#if VA2906_DBG
static ssize_t reg_val_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	u8 read;
	int ret;
	
	if (!va2906->is_ready)
		return -1;
	ret = va2906_read_reg(client, i2c_addr, reg_addr, &read);
	if (ret < 0)
		return ret;
	//va2906_adc_readdata(va2906);
	return sprintf(buf, "0x%x\n", read);
}

static ssize_t reg_val_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);
	int value;
	int ret;

	if (!va2906->is_ready)
		return -1;
	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;
	ret = va2906_write_reg(client, i2c_addr, reg_addr, value);
	if (ret < 0)
		return ret;
    return count;
}

static ssize_t reg_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%x\n", reg_addr);
}

static ssize_t reg_addr_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	int value;
	int ret;

    ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
        return ret;
 	reg_addr = value;
	return count;
}

static ssize_t i2c_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%x\n", i2c_addr);
}

static ssize_t i2c_addr_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	int value;
	int ret;

    ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
        return ret;
 	i2c_addr = value;
	return count;
}

static DEVICE_ATTR_RW(reg_val);
static DEVICE_ATTR_RW(reg_addr);
static DEVICE_ATTR_RW(i2c_addr);
#endif

static int __va2906_gpio_get(struct va2906_gpio_dev *gpio_dev, unsigned int offset)
{
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	int val=0;
	u8 reg_val;
	u16 reg_addr;
	
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		//reg_addr=0xc000 + offset;
		reg_addr = REG_VA2906_GPIO_BASE + offset;
		va2906_read_reg(client, 0, reg_addr, &reg_val);
		val = (reg_val&0x01)>>0;
		pr_info("-----__va2906_gpio_get----reg_addr=0x%x--reg_val=0x%x--val=%d-\r\n",reg_addr,reg_val,val);
	}
	return val;
}

static void __va2906_gpio_set(struct va2906_gpio_dev *gpio_dev, unsigned int offset, int value)
{
	u8 reg_val;
	u16 reg_addr;
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		reg_addr = REG_VA2906_GPIO_BASE + offset;
		reg_val = 0x0|(value<<1);
		pr_info("-----__va2906_gpio_set----reg_addr=0x%x--reg_val=0x%x---\r\n",reg_addr,reg_val);
		va2906_write_reg(client, 0, reg_addr, reg_val);
	}
}

static int va2906_gpio_dirout(struct gpio_chip *gc, unsigned int offset, int value)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);

	mutex_lock(&gpio_dev->i2c_lock);
	__va2906_gpio_set(gpio_dev, offset, value);
	mutex_unlock(&gpio_dev->i2c_lock);
	return 0;
}

static int va2906_gpio_dirin(struct gpio_chip *gc, unsigned int offset)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	u8 reg_val;
	u16 reg_addr;
	
	pr_info("-----va2906_gpio_dirin----offset=0x%x-----\r\n",offset);
	mutex_lock(&gpio_dev->i2c_lock);
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		reg_addr = REG_VA2906_GPIO_BASE + offset;
		reg_val = 0x20;
		va2906_write_reg(client, 0, reg_addr, reg_val);
	}
	mutex_unlock(&gpio_dev->i2c_lock);
	return 0;
}

static int va2906_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	int direction;
	u8 reg_val;
	u16 reg_addr;
	
	mutex_lock(&gpio_dev->i2c_lock);
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		reg_addr = REG_VA2906_GPIO_BASE + offset;
		va2906_read_reg(client, 0, reg_addr, &reg_val);
		direction = (reg_val&0x20)>>5;
		pr_info("-----va2906_gpio_get_direction----reg_addr=0x%x--reg_val=0x%x--direction=%d-\r\n",reg_addr,reg_val,direction);
	}
	mutex_unlock(&gpio_dev->i2c_lock);
	return direction;
}

static int va2906_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	int val=0;
	
	mutex_lock(&gpio_dev->i2c_lock);
	val = __va2906_gpio_get(gpio_dev, offset);
	mutex_unlock(&gpio_dev->i2c_lock);
	return val;
}

static void va2906_gpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);

	mutex_lock(&gpio_dev->i2c_lock);
	__va2906_gpio_set(gpio_dev, offset, value);
	mutex_unlock(&gpio_dev->i2c_lock);
}

static int va2906_gpio_set_config(struct gpio_chip *gc, unsigned int offset, unsigned long config)
{
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);

	dev_info(gpio_dev->dev, "--------va2906_gpio_set_config------\r\n");
	va2906_dbg_verbose("%s\n", __func__);
	switch (pinconf_to_config_param(config)) 
	{
		case PIN_CONFIG_BIAS_PULL_UP:
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			break;
		default:
			break;
	}
	return -ENOTSUPP;
}

static irqreturn_t va2906_gpio_isr(int irq, void *dev)
{
	pr_info("--------va2906_gpio_isr------\r\n");
	return IRQ_HANDLED;
}

static void va2906_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	unsigned int gpio = d->hwirq;
	unsigned long flags;
	
	dev_err(gpio_dev->dev, "--------va2906_irq_mask----gpio=0x%x--\r\n",gpio);
	va2906_dbg_verbose("%s: gpio:0x%x\n", __func__, gpio);
	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	if (gpio < gpio_dev->gc.ngpio)
		gpio_dev->irq_enable[gpio] = 0;
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static void va2906_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	unsigned int gpio = d->hwirq;
	unsigned long flags;
			
	dev_err(gpio_dev->dev, "--------va2906_irq_unmask----gpio=0x%x--\r\n",gpio);
	va2906_dbg_verbose("%s: gpio:0x%x\n", __func__, gpio);
	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	if (gpio < gpio_dev->gc.ngpio)
		gpio_dev->irq_enable[gpio] = 1;
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static int va2906_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	unsigned int gpio = d->hwirq;

	pr_info("--------va2906_irq_set_type----gpio=0x%x--\r\n",gpio);
	va2906_dbg_verbose("%s: gpio:0x%x\n", __func__, gpio);
	switch (type & IRQ_TYPE_SENSE_MASK) 
	{
		case IRQ_TYPE_EDGE_RISING:
			break;
		case IRQ_TYPE_EDGE_FALLING:
			break;
		case IRQ_TYPE_EDGE_BOTH:
			break;
		case IRQ_TYPE_LEVEL_HIGH:
		case IRQ_TYPE_LEVEL_LOW:
			break;
		default:
			dev_err(gpio_dev->dev, "Invalid GPIO irq type 0x%x\n", type);
			return -EINVAL;
	}
	
	return 0;
}

static void va2906_irq_bus_lock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);

	dev_info(gpio_dev->dev, "--------va2906_irq_bus_lock------\r\n");
	mutex_lock(&gpio_dev->irq_lock);
}

static void va2906_irq_bus_unlock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct va2906_gpio_dev *gpio_dev = gpiochip_get_data(gc);
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	u8 reg_val;
	u16 reg_addr;
	int value=0,i;

	dev_info(gpio_dev->dev, "--------va2906_irq_bus_unlock------\r\n");
	mutex_lock(&gpio_dev->i2c_lock);
	for (i = 0; i < gpio_dev->gc.ngpio; i++)
	{
		value += gpio_dev->irq_enable[i]<<i; 
	}
	
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		reg_addr = REG_VA2906_GPIO_INTR_MASK_0;
		reg_val = value;
		va2906_write_reg(client, 0, reg_addr, reg_val);
	}
	mutex_unlock(&gpio_dev->i2c_lock);
	mutex_unlock(&gpio_dev->irq_lock);
	dev_info(gpio_dev->dev, "--------va2906_irq_bus_unlock----end--\r\n");
}

static struct irq_chip va2906_irq_chip = {
	.name = "va2906_irq_chip",
	.irq_mask = va2906_irq_mask,
	.irq_unmask = va2906_irq_unmask,
	.irq_set_type = va2906_irq_set_type,
	.irq_bus_lock = va2906_irq_bus_lock,
	.irq_bus_sync_unlock = va2906_irq_bus_unlock,
	.flags = IRQCHIP_IMMUTABLE,
	 GPIOCHIP_IRQ_RESOURCE_HELPERS,
};

static int va2906_adc_read(struct iio_dev *indio_dev, struct iio_chan_spec const *chan)
{
	u32 val;
	//struct va2906_adc_dev *adc_dev = iio_priv(indio_dev);


	val = 0x25;

	return val;

//err_timeout:


	return -ETIMEDOUT;
}

static int va2906_adc_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan,
				  int *val, int *val2, long info)
{
	struct va2906_adc_dev *adc_dev = iio_priv(indio_dev);

	pr_info("--------va2906_adc_read_raw-------\r\n");
	switch (info) 
	{
		case IIO_CHAN_INFO_SCALE:
			switch (chan->type) 
			{
				case IIO_VOLTAGE:
					break;
				case IIO_CURRENT:
					break;
				default:
					return -EINVAL;
			}
			return IIO_VAL_FRACTIONAL;
		case IIO_CHAN_INFO_RAW:
			if (iio_buffer_enabled(indio_dev)) 
                return -EBUSY;
			mutex_lock(&adc_dev->lock);
			*val = va2906_adc_read(indio_dev, chan);
			if (*val < 0) 
			{
				mutex_unlock(&adc_dev->lock);
				dev_err(indio_dev->dev.parent, "failed to sample data on channel[%d]\n", chan->channel);
				return *val;
			}
			mutex_unlock(&adc_dev->lock);
			return IIO_VAL_INT;
		case IIO_CHAN_INFO_PROCESSED:
		mutex_lock(&adc_dev->lock);
			*val = va2906_adc_read(indio_dev, chan);
			if (*val < 0) 
			{
				mutex_unlock(&adc_dev->lock);
				dev_err(indio_dev->dev.parent, "failed to sample data on channel[%d]\n", chan->channel);
				return *val;
			}
			//*val = *val * VOLTAGE_FULL_RANGE / AUXADC_PRECISE;
			mutex_unlock(&adc_dev->lock);
			return IIO_VAL_INT;

		default:
			return -EINVAL;
	}
}

static const struct iio_info va2906_adc_info = {
	.read_raw = &va2906_adc_read_raw,
};

static irqreturn_t va2906_adc_trigger(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct va2906_adc_dev *adc_dev = iio_priv(indio_dev);
	int i, j = 0;

	pr_info("------va2906_adc_trigger------\r\n");
	mutex_lock(&adc_dev->lock);
	memset(adc_dev->buffer, 0, sizeof(adc_dev->buffer));
	for (i = 0; i < indio_dev->masklength; i++) 
	{
		if (!test_bit(i, indio_dev->active_scan_mask))
			continue;

		adc_dev->buffer[j] = va2906_adc_read(indio_dev, &indio_dev->channels[i]);
		j++;
	}
	iio_push_to_buffers_with_timestamp(indio_dev, adc_dev->buffer, pf->timestamp);
	mutex_unlock(&adc_dev->lock);

	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int va2906_probe(struct i2c_client *client)
{
	struct device_node *np = client->dev.of_node;
	struct device *dev = &client->dev;
	struct va2906_dev *va2906;
	struct va2905_dev *va2905;
	struct attribute_group *attr_group;
	struct attribute **attrs;
	struct va2905_attribute *tunnel_mode_attr, *comp_enable_attr, *lane_num_attr, *port_map_attr, *vc_id_attr;
	struct device_attribute *tunnel_mode_dev_attr, *comp_enable_dev_attr, *lane_num_dev_attr, *port_map_dev_attr, *vc_id_dev_attr;
	struct va2906_adc_dev *adc_dev;
	u32 clk_freq,fsync_mode=0,acmp_mode=0;
	int ret,i,j;

	va2906_dbg_verbose("%s\n", __func__);
	va2906 = devm_kzalloc(dev, sizeof(*va2906), GFP_KERNEL);
	if (!va2906)
	{
        dev_err(&client->dev, "va2906 memory allocation failed! \n");
        return -ENOMEM;
    }
	va2906->i2c_client = client;
	for (i=0; i<VA2905_MAX_NUM; i++)
	{
		if (va2906_enable[i]==0)
		{
			va2906_enable[i]=1;
			break;
		}
	}
	if (i < VA2905_MAX_NUM)
		va2906_device[i] = va2906;
	va2906->bpp = 8;
	va2906->nlanes = 1;
	va2906->mbus_code = va2906_mbus_codes[0];
	va2906->fsync_mode = 0;
	va2906->acmp_mode = 0;
	ret = va2906_init_controls(va2906);
	if (ret)
	{
		dev_err(&client->dev, "va2906_init_controls failed! \n");
		return ret;
	}
	v4l2_i2c_subdev_init(&va2906->sd, client, &va2906_subdev_ops);
	ret = va2906_parse_tx_ep(va2906);
	if (ret)
	{
		dev_err(&client->dev, "va2906 failed to parse tx %d\n", ret);
		goto power_off;
	}
	ret = va2906_parse_rx_ep(va2906);
	if (ret) 
	{
		dev_err(&client->dev, "va2906 failed to parse rx %d\n", ret);
		goto power_off;
	}
	/*
	va2906->xclk = devm_clk_get(dev, NULL);
	if (IS_ERR(va2906->xclk)) 
	{
		dev_err(&client->dev, "va2906 failed to get xclk\n");
		return PTR_ERR(va2906->xclk);
	}
	clk_freq = clk_get_rate(va2906->xclk);
	if (clk_freq < 6000000 || clk_freq > 27000000) 
	{
		dev_err(&client->dev, "va2906 xclk freq must be in 6-27 Mhz range. got %d Hz\n", clk_freq);
		return -EINVAL;
	}
	
	va2906->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(va2906->reset_gpio)) 
	{
		dev_err(&client->dev, "va2906 failed to get reset gpio\n");
		return PTR_ERR(va2906->reset_gpio);
	}
	*/
	va2906->test_gpio = devm_gpiod_get_optional(dev, "test", GPIOD_OUT_HIGH);
	if (IS_ERR(va2906->test_gpio))
	{
		dev_err(&client->dev, "va2906 failed to get test gpio\n");
	}
	else
	{
		int irq;
		
		gpiod_set_value_cansleep(va2906->test_gpio, 1);
		
		irq = gpiod_to_irq(va2906->test_gpio);
		if (irq <= 0) 
		{
			dev_err(&client->dev, "No IRQ resource\n");
		}
		else
		{
			pr_info("-----irq=0x%x-----\r\n",irq);
			ret = devm_request_threaded_irq(dev, irq, NULL, va2906_gpio_isr,
					   IRQF_ONESHOT | IRQF_TRIGGER_FALLING, "va2906-gpio", va2906);
			if (ret) 
			{
				dev_err(&client->dev, "Unable to request interrupt ret=%d\n",ret);
			}
		}
	}
	mutex_init(&va2906->lock);
	mutex_init(&va2906->mc_lock);
	va2906->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	va2906->sd.entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	va2906->sd.entity.ops = &va2906_subdev_entity_ops;
	va2906->pad[0].flags = MEDIA_PAD_FL_SOURCE;
	va2906->pad[1].flags = MEDIA_PAD_FL_SOURCE;
	va2906->pad[2].flags = MEDIA_PAD_FL_SOURCE;
	va2906->pad[3].flags = MEDIA_PAD_FL_SOURCE;
	va2906->pad[4].flags = MEDIA_PAD_FL_SINK;
	va2906->pad[5].flags = MEDIA_PAD_FL_SINK;
	va2906->pad[6].flags = MEDIA_PAD_FL_SINK;
	va2906->pad[7].flags = MEDIA_PAD_FL_SINK;
	va2906->pad[8].flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&va2906->sd.entity, VA2906_PAD_NUM, va2906->pad);
	if (ret) 
	{
		dev_err(&client->dev, "va2906 pads init failed %d\n", ret);
		goto entity_cleanup;
	}
	ret = va2906_set_power_on(va2906);
	if (ret)
	{
		dev_err(&client->dev, "va2906 power on failed %d\n", ret);
		goto power_off;
	}
	ret = va2906_detect(va2906);
	if (ret) 
	{
		dev_err(&client->dev, "failed to detect va2906 %d\n", ret);
		goto power_off;
	}
	if (IS_ENABLED(CONFIG_OF) && np) 
	{
		ret = fwnode_property_read_u32(of_fwnode_handle(np), "fsync-mode", &fsync_mode);
		if (ret)
			dev_err(&client->dev, "va2906 parsing fsync-enbale error: %d\n", ret);
		va2906->fsync_mode = fsync_mode;
		v4l2_ctrl_s_ctrl(va2906->fsync_ctrl, va2906->fsync_mode);
		ret = fwnode_property_read_u32(of_fwnode_handle(np), "acmp-mode", &acmp_mode);
		if (ret)
			dev_err(&client->dev, "va2906 parsing acmp-enbale error: %d\n", ret);
		va2906->acmp_mode = acmp_mode;
	}
	va2906_sensor_init(va2906);
	va2906_init_format(va2906,va2906->fmt);
	va2906->cdphytx.i2c_client=client;
	va2906->cdphytx.dev_addr=VA2906_CDPHY0_I2C_ADDR;
	va2906->cdphytx.init=va2906_cdphy_init;
#if VA2906_MPW
	va2906->cdphytx.dev_addr=0x0;
	va2906->cdphytx.init=va2906_cdphy_mpw_init;
#endif
	ret = va2906_write_array(va2906, 0, va2906_init_regs, ARRAY_SIZE(va2906_init_regs));
	if (ret < 0) 
	{
		dev_err(&client->dev, "va2906 write va2906_init_regs error\n");
		goto power_off;
	}
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&va2906->va2905[i];
		va2905->i2c_client=client;
		va2905->dev_addr=VA2905_SER0_I2C_ADDR+i;
		va2905->dev_init=va2905_dev_init;
		va2905->comp_init=va2905_dev_comp_init;
		va2905->tunnel_init=va2905_dev_tunnel_init;
		va2905->acmp_init=va2905_dev_acmp_init;
		va2905->acmp_uninit=va2905_dev_acmp_uninit;
		va2905->pal_gpio_init=va2905_pal_gpio_init;
		va2905->vproc_compraw10_init=va2905_vproc_compraw10_init;
		va2905->vproc_compyuv422_init=va2905_vproc_compyuv422_init;
		va2905->vpg_init=va2905_vpg_init;
		va2905->cdphyrx.i2c_client=client;
		va2905->fmt=&va2906->fmt[i];
		va2905->cdphyrx.dev_addr=VA2905_CDPHY0_I2C_ADDR+i;
		va2905->cdphyrx.init=va2905_cdphy_init;
#if VA2906_MPW
		va2905->cdphyrx.dev_addr=VA2905_SER0_I2C_ADDR+i;
		va2905->cdphyrx.init=va2905_cdphy_mpw_init;	
#endif
	}
	if (va2906->acmp_mode)
	{
		va2906_acmp_init(va2906);
		mutex_lock(&va2906->mc_lock);
		va2906->acmp_enable = 1;
		mutex_unlock(&va2906->mc_lock);
	}
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&va2906->va2905[i];
		if (va2905->enable)
		{
			va2905->comp_enable=1;
			if (va2905->tunnel_mode)
				ret = va2905->tunnel_init(va2905);
			else
				ret = va2905->dev_init(va2905);
			if (ret==0)
				dev_info(&client->dev, "va2905 0x%x init success\n",va2905->dev_addr);
			else
				dev_err(&client->dev, "va2905 0x%x init failed \n",va2905->dev_addr);
			ret = va2905->pal_gpio_init(va2905);
			if (ret==0)
				dev_info(&client->dev, "va2905 pal gpio init success\n");
			else
				dev_err(&client->dev, "va2905 pal gpio init failed \n");
		}
	}
	
	if (va2906->va2905[0].tunnel_mode || va2906->va2905[1].tunnel_mode)
		ret = va2906_dev_tunnel_init(va2906);
	else
		ret = va2906_dev_init(va2906);
	if (ret==0)
		dev_info(&client->dev, "va2906 init success\n");
	else
		dev_err(&client->dev, "va2906 init failed \n");
	ret = va2906_pal_gpio_init(va2906);
	if (ret==0)
		dev_info(&client->dev, "va2906 pal gpio init success\n");
	else
		dev_err(&client->dev, "va2906 pal gpio init failed \n");
	if (va2906->fsync_mode)
	{
		ret = va2906_fsync_init(va2906);
		if (ret==0)
			dev_info(&client->dev, "va2906 fsync init success\n");
		else
			dev_err(&client->dev, "va2906 fsync init failed \n");
	}
	
	va2906_adc_init(va2906);
	for (i=0; i<VA2905_CURR_NUM; i++)
	{
		va2905=&va2906->va2905[i];
		if (va2905->enable)
			va2906_set_vc_id(va2906,i,va2905->vc_id);
	}
	
	ret = v4l2_async_register_subdev(&va2906->sd);
	if (ret < 0) 
	{
		dev_err(&client->dev, "va2906 v4l2_async_register_subdev failed %d\n", ret);
		goto unregister_notifier;
	}
	va2906->is_ready = 1;
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);
#if VA2906_DBG
	device_create_file(dev, &dev_attr_reg_val);
	device_create_file(dev, &dev_attr_reg_addr);
	device_create_file(dev, &dev_attr_i2c_addr);
#endif
	va2906->va2906_obj = kobject_create_and_add("va2906", &dev->kobj);
	if (!va2906->va2906_obj)
	{
		pr_err("kobject_create_and_add va2906_obj failed\n");
		goto unregister_notifier;
	}
	ret = sysfs_create_group(va2906->va2906_obj, &va2906_attr_group);
	if (ret) 
	{
		pr_err("sysfs_create_group va2906_attr_group failed\n");
		goto err_create_object_2906;
	}
	va2906->va2905_obj = kobject_create_and_add("va2905", &dev->kobj);
	if (!va2906->va2905_obj)
	{
		pr_err("kobject_create_and_add va2905_obj failed\n");
		goto err_create_group_2906;
	}
	va2906->attr_groups = devm_kcalloc(dev, sizeof(*va2906->attr_groups), VA2905_CURR_NUM + 1, GFP_KERNEL);
	if (!va2906->attr_groups)
	{
		pr_err("devm_kcalloc va2906 attr_groups failed\n");
		goto err_create_object_2905;	
	}

	for (i = 0,j = 0; i < VA2905_CURR_NUM; i++) 
	{
		va2905=&va2906->va2905[i];
		if (va2905->enable)
		{
			attr_group = devm_kzalloc(dev, sizeof(*attr_group), GFP_KERNEL);
			if (!attr_group)
				return -ENOMEM;
			attr_group->name = devm_kasprintf(dev, GFP_KERNEL, "ch%u", i);
			if (!attr_group->name)
				return -ENOMEM;
			
			attrs = devm_kcalloc(dev, VA2905_NUM_ATTRS, sizeof(*attrs), GFP_KERNEL);
			if (!attrs)
				return -ENOMEM;
			tunnel_mode_attr = devm_kzalloc(dev, sizeof(*tunnel_mode_attr), GFP_KERNEL);
			comp_enable_attr = devm_kzalloc(dev, sizeof(*comp_enable_attr), GFP_KERNEL);
			lane_num_attr = devm_kzalloc(dev, sizeof(*lane_num_attr), GFP_KERNEL);
			port_map_attr = devm_kzalloc(dev, sizeof(*port_map_attr), GFP_KERNEL);
			vc_id_attr = devm_kzalloc(dev, sizeof(*vc_id_attr), GFP_KERNEL);
			if (!tunnel_mode_attr || !comp_enable_attr || !lane_num_attr || !port_map_attr || !vc_id_attr)
				return -ENOMEM;

			tunnel_mode_attr->index = i;
			comp_enable_attr->index = i;
			lane_num_attr->index = i;
			port_map_attr->index = i;
			vc_id_attr->index = i;

			tunnel_mode_dev_attr = &tunnel_mode_attr->dev_attr;
			comp_enable_dev_attr = &comp_enable_attr->dev_attr;
			lane_num_dev_attr = &lane_num_attr->dev_attr;
			port_map_dev_attr = &port_map_attr->dev_attr;
			vc_id_dev_attr = &vc_id_attr->dev_attr;

			sysfs_attr_init(&tunnel_mode_dev_attr->attr);
			sysfs_attr_init(&comp_enable_dev_attr->attr);
			sysfs_attr_init(&lane_num_dev_attr->attr);
			sysfs_attr_init(&port_map_dev_attr->attr);
			sysfs_attr_init(&vc_id_dev_attr->attr);

			tunnel_mode_dev_attr->attr.name = "tunnel_mode";
			comp_enable_dev_attr->attr.name = "comp_enable";
			lane_num_dev_attr->attr.name = "lane_num";
			port_map_dev_attr->attr.name = "port_map";
			vc_id_dev_attr->attr.name = "vc_id";
			tunnel_mode_dev_attr->attr.mode = 0644;
			comp_enable_dev_attr->attr.mode = 0644;
			lane_num_dev_attr->attr.mode = 0644;
			port_map_dev_attr->attr.mode = 0644;
			vc_id_dev_attr->attr.mode = 0644;
			tunnel_mode_dev_attr->show = tunnel_mode_show;
			tunnel_mode_dev_attr->store = tunnel_mode_store;
			comp_enable_dev_attr->show = comp_enable_show;
			comp_enable_dev_attr->store = comp_enable_store;
			lane_num_dev_attr->show = lane_num_show;
			port_map_dev_attr->show = port_map_show;
			vc_id_dev_attr->show = vc_id_show;
			vc_id_dev_attr->store = vc_id_store;

			attrs[0] = &tunnel_mode_dev_attr->attr;
			attrs[1] = &comp_enable_dev_attr->attr;
			attrs[2] = &lane_num_dev_attr->attr;
			attrs[3] = &port_map_dev_attr->attr;
			attrs[4] = &vc_id_dev_attr->attr;

			attr_group->attrs = attrs;
			va2906->attr_groups[j++] = attr_group;
		}
	}
	ret = sysfs_create_groups(va2906->va2905_obj, va2906->attr_groups);
	if (ret)
	{
		pr_err("sysfs_create_groups va2905_attr_groups failed\n");
		goto err_create_object_2905;
	}
	/*
	va2906->indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*adc_dev));
	if (!va2906->indio_dev)
		return -ENOMEM;

	adc_dev = iio_priv(va2906->indio_dev);
	va2906->indio_dev->name = "va2906_adc";
	va2906->indio_dev->info = &va2906_adc_info;
	va2906->indio_dev->modes = INDIO_DIRECT_MODE;
	va2906->indio_dev->channels = va2906_adc_iio_channels;
	va2906->indio_dev->num_channels = ARRAY_SIZE(va2906_adc_iio_channels);
	va2906->indio_dev->available_scan_masks = va2906_avail_scan_masks;
	mutex_init(&adc_dev->lock);
	ret = devm_iio_triggered_buffer_setup(&client->dev, va2906->indio_dev,
					      iio_pollfunc_store_time, va2906_adc_trigger, NULL);
	if (ret < 0) 
	{
		dev_err(&client->dev, "va2906 iio triggered buffer setup failed\n");
		goto err_create_object_2905;
	}
	ret = devm_iio_device_register(&client->dev, va2906->indio_dev);
	if (ret < 0) 
	{
		dev_err(&client->dev, "va2906 failed to register iio device\n");
		goto err_create_object_2905;
	}*/
					 	 
	dev_info(&client->dev, "va2906 device probe successfully \n");
	return 0;

err_create_object_2905:
    kobject_put(va2906->va2905_obj);
err_create_group_2906:
	sysfs_remove_group(va2906->va2906_obj, &va2906_attr_group);
err_create_object_2906:
    kobject_put(va2906->va2906_obj);
	
	v4l2_async_unregister_subdev(&va2906->sd);
power_off:
	va2906_set_power_off(va2906);
unregister_notifier:
	v4l2_async_nf_unregister(&va2906->notifier);
	v4l2_async_nf_cleanup(&va2906->notifier);
entity_cleanup:
	v4l2_ctrl_handler_free(&va2906->ctrls);
	media_entity_cleanup(&va2906->sd.entity);
	mutex_destroy(&va2906->lock);
	return ret;
}

static void va2906_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct va2906_dev *va2906 = to_va2906_dev(sd);

	va2906->is_ready = 0;
	v4l2_ctrl_handler_free(&va2906->ctrls);
	v4l2_async_nf_unregister(&va2906->notifier);
	v4l2_async_nf_cleanup(&va2906->notifier);
	v4l2_async_unregister_subdev(&va2906->sd);
	pm_runtime_disable(&client->dev);
	va2906_set_power_off(va2906);
	media_entity_cleanup(&va2906->sd.entity);
	mutex_destroy(&va2906->lock);
	mutex_destroy(&va2906->mc_lock);
#if VA2906_DBG
	device_remove_file(&client->dev, &dev_attr_reg_val);
	device_remove_file(&client->dev, &dev_attr_reg_addr);
	device_remove_file(&client->dev, &dev_attr_i2c_addr);
#endif
	sysfs_remove_groups(va2906->va2905_obj, va2906->attr_groups);
	sysfs_remove_group(va2906->va2906_obj, &va2906_attr_group);
	kobject_put(va2906->va2905_obj);
	kobject_put(va2906->va2906_obj);
}

static const struct of_device_id va2906_dt_ids[] = {
	{ .compatible = "zgm,va2906" },
	{  }
};
MODULE_DEVICE_TABLE(of, va2906_dt_ids);

static struct i2c_driver va2906_i2c_driver = {
	.driver = {
		.name  = "zgmicro-va2906",
		.of_match_table = va2906_dt_ids,
	},
	.probe_new = va2906_probe,
	.remove = va2906_remove,
};

static int va2906_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
    struct va2906_i2c_dev *i2c_dev = i2c_get_adapdata(adap);
	struct va2906_dev *va2906 = NULL;
    int ret,i;
	
	ret = pm_runtime_resume_and_get(i2c_dev->dev);
	if (ret < 0) 
	{
		dev_err(i2c_dev->dev, "va2906 i2c failed to runtime_get device: %d\n", ret);
		return ret;
	}
	for (i=0; i<VA2905_MAX_NUM; i++)		
	{
		va2906 = *i2c_dev->va2906[i];
		if (va2906_enable[i] && va2906)
		{
			struct i2c_client *client = va2906->i2c_client;

			if (msgs[0].addr==va2906->slave_alias_addr[0] ||
				msgs[0].addr==va2906->slave_alias_addr[1] ||
				msgs[0].addr==va2906->slave_alias_addr[2] ||
				msgs[0].addr==va2906->slave_alias_addr[3])
			{
				ret = i2c_transfer(client->adapter, msgs, num);
				if (ret < 0) 
				{
					dev_dbg(i2c_dev->dev, "%s:va2906 %x i2c_transfer => %d\n", __func__, client->addr, ret);
					goto out;
				}
				
			}
		}
		
	}
		
    if (ret == 0)
        ret = num;
out:
	pm_runtime_mark_last_busy(i2c_dev->dev);
	pm_runtime_put_autosuspend(i2c_dev->dev);
    return ret;
}

static u32 va2906_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_QUICK | I2C_FUNC_I2C | I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |	I2C_FUNC_SMBUS_WORD_DATA |	I2C_FUNC_SMBUS_BLOCK_DATA |	I2C_FUNC_SMBUS_I2C_BLOCK;
}

static const struct i2c_algorithm va2906_i2c_algo = {
    .master_xfer = va2906_i2c_xfer,
    .functionality = va2906_i2c_func,
};

static const struct i2c_adapter_quirks va2906_dw_quirks = {
	.flags = 0,
};

static const struct of_device_id va2906_i2c_bus_of_match[] = {
    {
        .compatible = "zgm,va2906_i2c_bus",
    },
    { },
};
MODULE_DEVICE_TABLE(of, va2906_i2c_bus_of_match);

static int va2906_i2c_bus_probe(struct platform_device *pdev)
{
    struct va2906_i2c_dev *i2c_dev;
    struct i2c_adapter *adap;
    struct device_node *node = pdev->dev.of_node;
    int ret;
	
	va2906_dbg_verbose("%s\n", __func__);
    i2c_dev = devm_kzalloc(&pdev->dev, sizeof(struct va2906_i2c_dev), GFP_KERNEL);
    if (!i2c_dev) 
	{
        dev_err(&pdev->dev, "va2906 i2c bus memory allocation failed! \n");
        return -ENOMEM;
    }
	i2c_dev->dev = &pdev->dev;
	pm_runtime_set_autosuspend_delay(i2c_dev->dev, 500);
	pm_runtime_use_autosuspend(i2c_dev->dev);
	pm_runtime_enable(i2c_dev->dev);
	ret = pm_runtime_resume_and_get(i2c_dev->dev);
	if (ret < 0) 
	{
		dev_err(i2c_dev->dev, "va2906 failed to runtime_get device: %d\n", ret);
		goto err_pm;
	}
	platform_set_drvdata(pdev, i2c_dev);
	i2c_dev->va2906[0] = &va2906_device[0];
	i2c_dev->va2906[1] = &va2906_device[1];
	i2c_dev->va2906[2] = &va2906_device[2];
	i2c_dev->va2906[3] = &va2906_device[3];
	adap = &i2c_dev->adapter; 
	i2c_set_adapdata(adap, i2c_dev);  
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "VA2906 I2C adapter", sizeof(adap->name));
	adap->algo = &va2906_i2c_algo; 
	adap->quirks = &va2906_dw_quirks;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = node;
	adap->nr = 0x10;
	if (pdev->id != PLATFORM_DEVID_NONE || !node || of_property_read_u32(node, "bus_nr", &adap->nr))
	{
		adap->nr = pdev->id;
	}
	ret = i2c_add_numbered_adapter(adap);
	if (ret)
	{
		dev_err(i2c_dev->dev, "va2906 i2c bus registering failed (%d)\n", ret);
		goto err_unuse_clocks;
	}
	dev_info(i2c_dev->dev, "va2906 i2c bus probe successfully \n");
	pm_runtime_mark_last_busy(i2c_dev->dev);
	pm_runtime_put_autosuspend(i2c_dev->dev);
	return 0;

err_unuse_clocks:
	pm_runtime_dont_use_autosuspend(i2c_dev->dev);
	pm_runtime_put_sync(i2c_dev->dev);
err_pm:
	pm_runtime_disable(i2c_dev->dev);
	return ret;
}

static int va2906_i2c_bus_remove(struct platform_device *pdev)
{
	struct va2906_i2c_dev *i2c_dev = platform_get_drvdata(pdev);
	int ret;

	i2c_del_adapter(&i2c_dev->adapter);
	ret = pm_runtime_resume_and_get(&pdev->dev);
	if (ret < 0)
		return ret;

	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static struct platform_driver va2906_i2c_bus_driver = {
    .driver = {
        .name = "va2906-i2c-bus",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(va2906_i2c_bus_of_match),
    },
	.probe = va2906_i2c_bus_probe,
    .remove = va2906_i2c_bus_remove,
};

static irqreturn_t gpio_dev_irq(int irq, void *data)
{
	struct va2906_gpio_dev *gpio_dev = data;
	struct va2906_dev *va2906 = *gpio_dev->va2906;
	u8 reg_val;
	u16 reg_addr;
	int bit=0;
	unsigned long status=0;

	pr_info("--------gpio_dev_irq--------\r\n");
	mutex_lock(&gpio_dev->i2c_lock);
	if (va2906)
	{
		struct i2c_client *client = va2906->i2c_client;
		
		reg_addr = REG_VA2906_GPIO_INTR_STATUS_0;
		va2906_read_reg(client, 0, reg_addr, &reg_val);
		status = reg_val;
		pr_info("-----gpio_dev_irq------status=0x%lx---\r\n",status);
	}
	mutex_unlock(&gpio_dev->i2c_lock);
	
	//status=0x04;
	for_each_set_bit(bit, &status, gpio_dev->gc.ngpio)
	{
		unsigned int child_irq;
		
		child_irq = irq_find_mapping(gpio_dev->gc.irq.domain, bit);
		handle_nested_irq(child_irq);
	}
	return IRQ_HANDLED;
}

static int va2906_gpio_probe(struct platform_device *pdev)
{
	struct va2906_gpio_dev *gpio_dev;
	struct device_node *np = pdev->dev.of_node;
	struct gpio_chip *gc;
	struct gpio_irq_chip *girq;
	struct device *dev;
	const char *name = "va2906-gc";
	int ret, base, i;
	u32 ngpio, irqno;

	va2906_dbg_verbose("%s\n", __func__);
	dev = &pdev->dev;
	ret = device_property_read_u32(dev, "gpio-base", &base);
	if (ret)
		base = -1;
	ret = device_property_read_u32(dev, "nr-gpios", &ngpio);
	if (ret)
	{
		dev_err(dev, "va2906 gpio parse nr-gpios failed\n");
		return ret;
	}
	ret = device_property_read_string(dev, "chip-label", &name);
	if (ret)
		name = dev_name(dev);
	gpio_dev = devm_kzalloc(dev, sizeof(*gpio_dev), GFP_KERNEL);
	if (!gpio_dev)
	{
        dev_err(dev, "va2906 gpio memory allocation failed\n");
        return -ENOMEM;
    }
	gpio_dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, gpio_dev);
	raw_spin_lock_init(&gpio_dev->lock);
	mutex_init(&gpio_dev->i2c_lock);
	mutex_init(&gpio_dev->irq_lock);
	gpio_dev->va2906 = &va2906_device[0];
	gc = &gpio_dev->gc;
	gc->base = base;
	gc->ngpio = ngpio;
	gc->label = name;
	gc->owner = THIS_MODULE;
	gc->parent = dev;
	gc->get = va2906_gpio_get;
	gc->set = va2906_gpio_set;
	gc->direction_output = va2906_gpio_dirout;
	gc->direction_input = va2906_gpio_dirin;
	gc->get_direction = va2906_gpio_get_direction;
	gc->set_config = va2906_gpio_set_config;
	
	girq = &gc->irq;
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->parents = NULL;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_edge_irq;
	girq->threaded = true;
		
	gpio_irq_chip_set_chip(&gc->irq, &va2906_irq_chip);
	ret = devm_gpiochip_add_data(dev, &gpio_dev->gc, gpio_dev);
	if (ret) 
	{
		dev_err(gpio_dev->dev, "va2906 gpio dev registering failed (%d)\n", ret);
		return ret;
	}
	
	gpio_dev->irq_enable = devm_kcalloc(gpio_dev->dev, ngpio, 1, GFP_KERNEL);
	if (!gpio_dev->irq_enable)
	{
		dev_err(gpio_dev->dev, "va2906 gpio dev alloc memory failed \n");
		return -ENOMEM;
	}

	for (i = 0; i < ngpio; i++) 
	{
		gpio_dev->irq_enable[i] = 0x00;
	}

	irqno = irq_of_parse_and_map(np, 0);
	printk("irqno = %d\n", irqno);
	gpio_dev->irq = irqno;
	ret = devm_request_threaded_irq(gpio_dev->dev, irqno, NULL, gpio_dev_irq,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT, "va2906_irq_chip", gpio_dev);
	if (ret != 0) 
	{
		dev_err(gpio_dev->dev, "va2906 gpio device can't request IRQ#%d: %d\n", irqno, ret);
		return ret;
	}

	dev_info(gpio_dev->dev, "va2906 gpio device probe successfully \n");
	return 0;
}

static int va2906_gpio_remove(struct platform_device *pdev)
{
	struct va2906_gpio_dev *gpio_dev = platform_get_drvdata(pdev);
	int ret;

	gpiochip_remove(&gpio_dev->gc);
	mutex_destroy(&gpio_dev->i2c_lock);
	mutex_destroy(&gpio_dev->irq_lock);
	ret = pm_runtime_resume_and_get(&pdev->dev);
	if (ret < 0)
		return ret;

	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static const struct of_device_id va2906_gpio_of_match[] = {
	{ .compatible = "zgm,va2906_gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, va2906_gpio_of_match);

static struct platform_driver va2906_gpio_chip_driver = {
	.driver = {
		.name = "va2906-gpio",
		.of_match_table = va2906_gpio_of_match,
	},
	.probe = va2906_gpio_probe,
	.remove = va2906_gpio_remove,
};

static int __init va2906_init(void)
{
	int ret;
	
	ret = platform_driver_register(&va2906_gpio_chip_driver);
	if (ret) 
	{
		pr_err("error registering va2906 gpio_chip platform driver\n");
		return ret;
	}
	ret = i2c_add_driver(&va2906_i2c_driver);
	if (ret)
	{
		pr_err("error registering va2906 i2c platform driver \n");
		return ret;
	}
	ret = platform_driver_register(&va2906_i2c_bus_driver);
	if (ret) 
	{
		pr_err("error registering va2906 i2c_bus platform driver\n");
		return ret;
	}
	return 0;
}
module_init(va2906_init);

static void __exit va2906_exit(void)
{
	platform_driver_unregister(&va2906_gpio_chip_driver);
	platform_driver_unregister(&va2906_i2c_bus_driver);
    i2c_del_driver(&va2906_i2c_driver);
}
module_exit(va2906_exit);


MODULE_AUTHOR("Ljj");
MODULE_DESCRIPTION("Zgmicro va2906 MIPI CSI2 bridge driver");
MODULE_LICENSE("GPL v2");
