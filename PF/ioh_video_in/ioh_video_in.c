/*
 * Copyright (C) 2010-2011 OKI SEMICONDUCTOR CO., LTD.
 * Copyright (C) 2011-2012 LAPIS SEMICONDUCTOR CO., LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 */

/* DEBUG                   : Debug print on */
/* V2G_BRIDGE_IF           : Video to Graphics Bridge Driver Interface */
/* OPERATION_COUNT         : Print out the counter value */
/* OVERFLOW_OFF            : skip overflow operation for testing */

#if 0
#endif

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>

#include "ioh_video_in_main.h"
#include "ioh_dma_main.h"

#define IOH_VIN_MAJOR_VERSION 1
#define IOH_VIN_MINOR_VERSION 5
#define IOH_VIN_RELEASE 0
#define IOH_VIN_VERSION \
	KERNEL_VERSION(IOH_VIN_MAJOR_VERSION, IOH_VIN_MINOR_VERSION, \
							IOH_VIN_RELEASE)

/* Macros for register offset. */
#define IOH_VIN_VICTRL1		(0x0000U)
#define IOH_VIN_VICTRL2		(0x0004U)

#define IOH_VIN_VOCTRL1		(0x0008U)
  #define OBITSEL1		(10)
  #define OBITSEL0		(9)
  #define OUT2S			(8)
  #define LDSEL			(6)
  #define ORGBSEL		(1)

#define IOH_VIN_VOCTRL2		(0x000CU)
  #define O422SEL		(8)
  #define CBON			(7)
  #define BBON			(6)
  #define SBON			(4)
  #define RGBLEV		(3)

#define IOH_VIN_BLNKTIM		(0x0018U)
#define IOH_VIN_LUMLEV		(0x0020U)
#define IOH_VIN_GGAIN		(0x0024U)
#define IOH_VIN_BGAIN		(0x0028U)
#define IOH_VIN_RGAIN		(0x002CU)
#define IOH_VIN_THMOD1		(0x00F8U)
#define IOH_VIN_THMOD2		(0x00FCU)
  #define BLNDRENB		(4)

#define IOH_VIN_INTENB		(0x0100U)

#define IOH_VIN_INTSTS		(0x0104U)
  #define INTBITS		(0x7)
  #define OFINTSTS		(0x4)
  #define HSINTSTS		(0x2)
  #define VSINTSTS		(0x1)

#define IOH_VIN_VDATA		(0x1000U)

#define IOH_VIN_RESET		(0x1ffcU)
  #define ASSERT_RESET		(0x0001U)
  #define DE_ASSERT_RESET	(0x0000U)

#define VSYNC_SYNC		0
#define VSYNC_NOT_SYNC		1

#define DISABLE			0
#define ENABLE			1

#define INPUT_DEFAULT		0

#define MAX_WIDTH		832
#define MAX_HEIGHT		573
#define MAX_BYTESPERLINE	(ALIGN(MAX_WIDTH * 2, 128))

#define MAX_DESC_NUM		(MAX_HEIGHT + 3)

#define MIN_N_FRAME_BUF		3
#define MAX_N_FRAME_BUF		MAXIMUM_FRAME_BUFFERS

#define IOH_VIN_FLAG_OPENED	0	/* V4L2 device is opened */
#define	IOH_VIN_FLAG_START_CAP	1	/* Capturing is started */

enum ioh_vin_input_data_format {
	/* Input format for NTSC Square Pixel frequency */
	NT_SQPX_ITU_R_BT_656_4_8BIT,
	NT_SQPX_ITU_R_BT_656_4_10BIT,
	NT_SQPX_YCBCR_422_8BIT,
	NT_SQPX_YCBCR_422_10BIT,
	/* Input format for NTSC ITU-R BT.601 */
	NT_BT601_ITU_R_BT_656_4_8BIT,
	NT_BT601_ITU_R_BT_656_4_10BIT,
	NT_BT601_YCBCR_422_8BIT,
	NT_BT601_YCBCR_422_10BIT,
	/* Input format for RAW. */
	NT_RAW_8BIT,
	NT_RAW_10BIT,
	NT_RAW_12BIT,
	/* Input format for PAL ITU-R BT.601 */
	PAL_BT601_ITU_R_BT_656_4_8BIT,
	PAL_BT601_ITU_R_BT_656_4_10BIT,
	PAL_BT601_YCBCR_422_8BIT,
	PAL_BT601_YCBCR_422_10BIT,
	/* Invalid Input Format. */
	INVALID_INPUT_DATA_FORMAT	/* Invalid Input data format */
};

enum ioh_vin_output_data_format {
	YCBCR_422,			/* YCbCr 4:2:2 format.	*/
	YCBCR_444,			/* YCbCr 4:4:4 format.	*/
	RGB888,				/* RGB888 format.	*/
	RGB666,				/* RGB666 format.	*/
	RGB565,				/* RGB565 format.	*/
	RAW_INPUT,			/* Input format is RAW	*/
	INVALID_OUTPUT_DATA_FORMAT	/* Invalid output format. */
};

struct reg_intenb {
	u8 drevsem;
	u8 dmarenb;
	u8 ofintenb;
	u8 hsintenb;
	u8 vsintenb;
};

struct ioh_vin_config {
	struct v4l2_input vin_input;
	enum ioh_vin_input_data_format hw_config_in;
	enum ioh_vin_output_data_format hw_config_out;
	v4l2_std_id std_id;
	struct v4l2_pix_format pix_format;
};


/* bytesperline should be a multiple of 128 */
static const struct ioh_vin_config ioh_vin_config_tables[] = {
	{
		.vin_input = {
			.index = 0,
			.name = "RGB565",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = (V4L2_STD_NTSC | V4L2_STD_PAL),
		},
		.hw_config_in = NT_RAW_8BIT,
		.hw_config_out = RAW_INPUT,
		.std_id = V4L2_STD_UNKNOWN,
		.pix_format = {
			.width		= 800,
			.height		= 480,
			.pixelformat	= V4L2_PIX_FMT_RGB565,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 832 * 2,
			.sizeimage	= 832 * 2 * 480,
		},
	},
	{
		.vin_input = {
			.index = 1,
			.name = "NTSC",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = (V4L2_STD_NTSC | V4L2_STD_PAL),
		},
		.hw_config_in = NT_BT601_ITU_R_BT_656_4_8BIT,
		.hw_config_out = YCBCR_422,
		.std_id = V4L2_STD_NTSC,
		.pix_format = {
			.width		= 720,
			.height		= 480,
			.pixelformat	= V4L2_PIX_FMT_UYVY,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 768 * 2,
			.sizeimage	= 768 * 2 * 480,
		},
	},
	{
		.vin_input = {
			.index = 2,
			.name = "PAL",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = (V4L2_STD_NTSC | V4L2_STD_PAL),
		},
		.hw_config_in = PAL_BT601_ITU_R_BT_656_4_8BIT,
		.hw_config_out = YCBCR_422,
		.std_id = V4L2_STD_PAL,
		.pix_format = {
			.width		= 720,
			.height		= 573,
			.pixelformat	= V4L2_PIX_FMT_UYVY,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 768 * 2,
			.sizeimage	= 768 * 2 * 573,
		},
	},
};

struct ioh_vin_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer		vb;

};

/*
 * base_address		base (remapped) address of the video device
 * physical_address	physical address of the video device
 * p_device		pci_dev structure reference of the device
 * irq			IRQ line reserved for the device
 *
 * bufs			Video Frame Buffers
 *
 * dev_lock		the spinlock for this data structure
 * slock		the spinlock for videobuf
 * s_mutex		mutex lock for this data structure
 *
 * s_flags		status flags
 *
 * dma_status		dma status from dma callback function
 * dma_channel		DMA channel obtained for capturing data
 * dma_desc_virt	base virtual addres of dma descriptor
 * dma_desc_phy		base physical addres of dma descriptor
 * desc_start		start pointer of dma descriptor for restarting dma
 * desc_end		end pointer of dma descriptor for restarting dma
 * dma_fail_count	DMA fail count
 *
 * frame_index		frame index number
 *
 * refresh_count	counter for image refreshing
 *
 * v4l2_dev		v4l2 device
 * vfd			video device node
 * vidq_active		the queue head of buffers
 *
 * intenb		intenb register value
 * cur_hw_config_in	current input setting
 * cur_hw_config_out	current output setting
 * cur_input		current input index
 * pix_format		current pixel format
 * vb_vidq		videobuf queue
 * type			buffer type
 */

struct ioh_vin_device {
	void __iomem		*base_address;
	u32			physical_address;
	struct pci_dev		*p_device;
	int			irq;

	struct ioh_video_in_frame_buffers	bufs;

	spinlock_t		dev_lock;
	spinlock_t		slock;
	struct mutex		s_mutex;

	unsigned long		s_flags;

	s8			dma_status;
	s32			dma_channel;
	void			*dma_desc_virt;
	dma_addr_t		dma_desc_phy;
	struct ioh_dma_desc	*desc_start[MAX_N_FRAME_BUF];
	struct ioh_dma_desc	*desc_end;
	int			dma_fail_count;

	u32			frame_index;

	int			refresh_count;

	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd;
	struct list_head	vidq_active;

	struct reg_intenb	intenb;
	enum ioh_vin_input_data_format	cur_hw_config_in;
	enum ioh_vin_output_data_format	cur_hw_config_out;
	int			cur_input;
	struct v4l2_pix_format	pix_format;
	struct videobuf_queue	vb_vidq;
	enum v4l2_buf_type	type;
};

#define IOH_VIN_DRV_NAME		"ioh_videoin"
#define PCI_VENDOR_ID_IOH		(0x10DB)
#define PCI_DEVICE_ID_IVI_VIDEOIN	(0x802A)
#define PCI_DEVICE_ID_MP_VIDEOIN	(0x8011)

#define SET_BIT(value, bit)		((value) |= ((u32)0x1 << (bit)))
#define RESET_BIT(value, bit)		((value) &= ~((u32)0x1 << (bit)))
#define RESET_BIT_RANGE(value, lbit, hbit)			\
((value) &= ~((((u32)2 << ((hbit) - (lbit) + 1)) - 1) << (lbit)))

/* -- */
#define ioh_err(device, fmt, arg...) \
	dev_err(&(device)->p_device->dev, fmt, ##arg);
#define ioh_warn(device, fmt, arg...) \
	dev_warn(&(device)->p_device->dev, fmt, ##arg);
#define ioh_info(device, fmt, arg...) \
	dev_info(&(device)->p_device->dev, fmt, ##arg);
#define ioh_dbg(device, fmt, arg...) \
	dev_dbg(&(device)->p_device->dev, fmt, ##arg);
/* -- */

#ifdef OPERATION_COUNT
static int count_dma_callback;
static int count_overflow_ipt;
static int count_overflow_frame;
static int count_image_refresh;
static int count_address_idle;
static int count_address_read;
static int count_address_wait;
static int count_address_access;
static int count_address_other;
static int count_address_match;
static int count_address_nomatch;
static int count_v2g_callback;
#define COUNTUP(x) do {x++; } while (0)
#else /* OPERATION_COUNT */
#define COUNTUP(x) do { } while (0)
#endif /* OPERATION_COUNT */

static int video_nr = -1;
module_param(video_nr, int, 0444);
MODULE_PARM_DESC(video_nr, "videoX number, -1 is autodetect.");

static unsigned n_frame_buf = 3;
module_param(n_frame_buf, uint, 0444);
MODULE_PARM_DESC(n_frame_buf, "the number of frame buffer to allocate[3-5].");

static unsigned refresh_period = 60;
module_param(refresh_period, int, 0644);
MODULE_PARM_DESC(refresh_period, "the period of the image refresh. "
				 "default 60 vsync. 0 is every time. "
				 "-1 is :off.");

static unsigned int vid_limit = 16;
static int err_flag;
static int cap_start_flag;
static int cap_stop_flag;

#ifdef V2G_BRIDGE_IF
static void *ioh_vin_v2g_bridge_token;
static int (*ioh_vin_v2g_bridge_callback)
			(void *token, int frame_index);
#endif /* V2G_BRIDGE_IF */

/* prototype */
static void ioh_vin_dma_complete(struct ioh_vin_device *device);
static int ioh_vin_restart_dma(struct ioh_vin_device *device);

/* ------------------------------------------------------------------
 *	Inline Helper Function
 * ------------------------------------------------------------------*/

static inline void ioh_vin_assert_reset(struct ioh_vin_device *device)
{
	ioh_dbg(device, "Assert reset");
	iowrite32(ASSERT_RESET, device->base_address + IOH_VIN_RESET);
}

static inline void ioh_vin_de_assert_reset(struct ioh_vin_device *device)
{
	ioh_dbg(device, "De-Assert reset");
	iowrite32(DE_ASSERT_RESET, device->base_address + IOH_VIN_RESET);
}

static inline void print_v4l2_pix_format(struct ioh_vin_device *device,
						struct v4l2_pix_format *fmt)
{
	ioh_dbg(device, "width=%d, height=%d, format=%c%c%c%c, field=%s, "
			"bytesperline=%d, sizeimage=%d, colorspace=%d",
				fmt->width, fmt->height,
				(fmt->pixelformat >> 0) & 0xff,
				(fmt->pixelformat >> 8) & 0xff,
				(fmt->pixelformat >> 16) & 0xff,
				(fmt->pixelformat >> 24) & 0xff,
				v4l2_field_names[fmt->field],
				fmt->bytesperline,
				fmt->sizeimage,
				fmt->colorspace);
}

static inline u32 ioh_vin_get_additional_line_size
				(struct ioh_vin_device *device)
{
	enum ioh_vin_input_data_format format;
	u32 addition = 0;

	format = device->cur_hw_config_in;

	if ((format == NT_SQPX_ITU_R_BT_656_4_8BIT)
	 || (format == NT_SQPX_ITU_R_BT_656_4_10BIT)
	 || (format == NT_SQPX_YCBCR_422_8BIT)
	 || (format == NT_SQPX_YCBCR_422_10BIT)
	 || (format == NT_BT601_ITU_R_BT_656_4_8BIT)
	 || (format == NT_BT601_ITU_R_BT_656_4_10BIT)
	 || (format == NT_BT601_YCBCR_422_8BIT)
	 || (format == NT_BT601_YCBCR_422_10BIT))
		addition = 3;

	ioh_dbg(device, "%s additional line is %d", __func__, addition);

	return addition;
}

/* ------------------------------------------------------------------
 *	HAL Operations
 * ------------------------------------------------------------------*/
void write_intenb(struct ioh_vin_device *device)
{
	void __iomem *addr;
	u32 data;

	addr = device->base_address + IOH_VIN_INTENB;
	data = device->intenb.drevsem << 5
		| device->intenb.dmarenb << 4
		| device->intenb.ofintenb << 2
		| device->intenb.hsintenb << 1
		| device->intenb.vsintenb << 0;

	iowrite32(data, addr);

	ioh_dbg(device, "%s -> 0x%04x write", __func__, data);
}

struct ioh_vin_input_settings {
	enum ioh_vin_input_data_format format;
	u32 victrl1;
	u32 victrl2;
};

static const struct ioh_vin_input_settings ioh_input_settings[] = {
	/* <Input Format>		<VICTRL1>   <VICTRL2> */
	/* 640 x 480 */
	{NT_SQPX_ITU_R_BT_656_4_8BIT,	0x00000001, 0x00000000},
	{NT_SQPX_ITU_R_BT_656_4_10BIT,	0x00000001, 0x00000200},
	{NT_SQPX_YCBCR_422_8BIT,	0x00000009, 0x00000000},
	{NT_SQPX_YCBCR_422_10BIT,	0x00000009, 0x00000200},
	/* 720 x 480 */
	{NT_BT601_ITU_R_BT_656_4_8BIT,	0x00000000, 0x00000000},
	{NT_BT601_ITU_R_BT_656_4_10BIT,	0x00000000, 0x00000200},
	{NT_BT601_YCBCR_422_8BIT,	0x00000008, 0x00000000},
	{NT_BT601_YCBCR_422_10BIT,	0x00000008, 0x00000200},
	/* RAW */
	{NT_RAW_8BIT,			0x00000000, 0x00000100},
	{NT_RAW_10BIT,			0x00000000, 0x00000300},
	{NT_RAW_12BIT,			0x00000000, 0x00000500},
	/* 720 x 573 */
	{PAL_BT601_ITU_R_BT_656_4_8BIT,	0x00000000, 0x00000008},
	{PAL_BT601_ITU_R_BT_656_4_10BIT, 0x00000000, 0x00000208},
	{PAL_BT601_YCBCR_422_8BIT,	0x00000008, 0x00000008},
	{PAL_BT601_YCBCR_422_10BIT,	0x00000008, 0x00000208},
};

static int ioh_vin_set_input_format(struct ioh_vin_device *device)
{
	u32 victrl1;
	u32 victrl2;
	u32 counter;
	void __iomem *base_address;
	s32 ret;
	enum ioh_vin_input_data_format format;
	unsigned long flags;

	base_address = device->base_address;
	format = device->cur_hw_config_in;

	for (counter = 0; counter < ARRAY_SIZE(ioh_input_settings); counter++) {
		if (format == ioh_input_settings[counter].format)
			break;
	}

	if (counter >= ARRAY_SIZE(ioh_input_settings)) {
		ioh_err(device, "%s -> Invalid input data format", __func__);
		device->cur_hw_config_in = INVALID_INPUT_DATA_FORMAT;
		return -EINVAL;
	}

	victrl1 = ioh_input_settings[counter].victrl1;
	victrl2 = ioh_input_settings[counter].victrl2;

	spin_lock_irqsave(&device->dev_lock, flags);

	iowrite32(victrl1, (base_address + IOH_VIN_VICTRL1));

	if ((format == PAL_BT601_ITU_R_BT_656_4_8BIT)
	 || (format == PAL_BT601_ITU_R_BT_656_4_10BIT)
	 || (format == PAL_BT601_YCBCR_422_8BIT)
	 || (format == PAL_BT601_YCBCR_422_10BIT)) {

		u32 val = ioread32(base_address + IOH_VIN_THMOD2);
		SET_BIT(val, BLNDRENB);
		iowrite32(val, (base_address + IOH_VIN_THMOD2));

		iowrite32(victrl2, (base_address + IOH_VIN_VICTRL2));

		RESET_BIT(val, BLNDRENB);
		iowrite32(val, (base_address + IOH_VIN_THMOD2));
	} else {
		iowrite32(victrl2, (base_address + IOH_VIN_VICTRL2));
	}

	ioh_dbg(device, "%s -> victrl1 = %08x",	__func__, victrl1);
	ioh_dbg(device, "%s -> victrl2 = %08x", __func__, victrl2);

	if ((victrl1 == (ioread32(base_address + IOH_VIN_VICTRL1)))
	 && (victrl2 == (ioread32(base_address + IOH_VIN_VICTRL2)))) {
		ioh_dbg(device, "%s -> Register write successful", __func__);
		ret = 0;
	} else {
		ioh_err(device, "%s -> Register write unsuccessful", __func__);
		device->cur_hw_config_in = INVALID_INPUT_DATA_FORMAT;
		ret = -EINVAL;
	}
	spin_unlock_irqrestore(&device->dev_lock, flags);

	ioh_dbg(device, "%s ended(%d)", __func__, ret);
	return ret;
}

static int ioh_vin_check_raw_input(enum ioh_vin_input_data_format format)
{
	if ((format == NT_RAW_8BIT)
	|| (format == NT_RAW_10BIT)
	|| (format == NT_RAW_12BIT)) {
		return true;
	} else {
		return false;
	}
}

static int ioh_vin_set_output_format(struct ioh_vin_device *device)
{
	u32 voctrl1 = 0;
	u32 voctrl2 = 0;
	void __iomem *base_address;
	enum ioh_vin_output_data_format format_out;
	enum ioh_vin_input_data_format format_in;
	int err = 0;
	unsigned long flags;

	base_address = device->base_address;
	format_out = device->cur_hw_config_out;
	format_in = device->cur_hw_config_in;

	spin_lock_irqsave(&device->dev_lock, flags);

	switch (format_out) {
	case YCBCR_422:
		if (ioh_vin_check_raw_input(format_in)) {
			err = -EINVAL;
		} else {
			RESET_BIT(voctrl1, ORGBSEL);
			SET_BIT(voctrl2, O422SEL);
		}
		break;

	case YCBCR_444:
		if (ioh_vin_check_raw_input(format_in)) {
			err = -EINVAL;
		} else {
			RESET_BIT(voctrl1, ORGBSEL);
			RESET_BIT(voctrl2, O422SEL);
		}
		break;

	case RGB888:
		if (ioh_vin_check_raw_input(format_in)) {
			err = -EINVAL;
		} else {
			RESET_BIT(voctrl1, OBITSEL1);
			RESET_BIT(voctrl1, OBITSEL0);
			SET_BIT(voctrl1, ORGBSEL);
		}
		break;

	case RGB666:
		if (ioh_vin_check_raw_input(format_in)) {
			err = -EINVAL;
		} else {
			RESET_BIT(voctrl1, OBITSEL1);
			SET_BIT(voctrl1, OBITSEL0);
			SET_BIT(voctrl1, ORGBSEL);
		}
		break;

	case RGB565:
		if (ioh_vin_check_raw_input(format_in)) {
			err = -EINVAL;
		} else {
			SET_BIT(voctrl1, OBITSEL1);
			RESET_BIT(voctrl1, OBITSEL0);
			SET_BIT(voctrl1, ORGBSEL);
		}
		break;

	case RAW_INPUT:
		if (ioh_vin_check_raw_input(format_in) == false)
			err = -EINVAL;
		break;

	default:
		err = -EINVAL;
	}

	if (err) {
		ioh_err(device, "%s -> Invalid Output Data Format", __func__);
		device->cur_hw_config_out = INVALID_OUTPUT_DATA_FORMAT;
		goto out;
	}

	RESET_BIT(voctrl1, OUT2S);
	RESET_BIT(voctrl1, LDSEL);
	RESET_BIT(voctrl2, CBON);
	RESET_BIT(voctrl2, BBON);
	RESET_BIT(voctrl2, SBON);
	RESET_BIT(voctrl2, RGBLEV);

	iowrite32(voctrl1, (base_address + IOH_VIN_VOCTRL1));
	iowrite32(voctrl2, (base_address + IOH_VIN_VOCTRL2));

	ioh_dbg(device, "%s -> voctrl1 = %08x",	__func__, voctrl1);
	ioh_dbg(device, "%s -> voctrl2 = %08x", __func__, voctrl2);

	if ((voctrl1 == ioread32(base_address + IOH_VIN_VOCTRL1))
	 && (voctrl2 == ioread32(base_address + IOH_VIN_VOCTRL2))) {
		ioh_dbg(device, "%s -> Register write successful", __func__);
		err = 0;
	} else {
		ioh_err(device, "%s -> Register write unsuccessful", __func__);
		device->cur_hw_config_out = INVALID_OUTPUT_DATA_FORMAT;
		err = -EINVAL;
	}
out:
	spin_unlock_irqrestore(&device->dev_lock, flags);

	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}

static u32 ioh_vin_get_buffer_size(struct ioh_vin_device *device)
{
	u32 retval = device->pix_format.sizeimage;

	ioh_dbg(device, "%s invoked successfully(%u)",
			__func__, retval);

	return retval;
}

static int ioh_vin_alloc_frame_buffer(struct ioh_vin_device *device)
{
	size_t size;
	struct ioh_video_in_frame_buffer *buf;
	int i, j;

	ioh_dbg(device, "%s start", __func__);

	size = MAX_HEIGHT * MAX_BYTESPERLINE;
	ioh_dbg(device, "%s size is %d", __func__, size);

	for (i = 0; i < n_frame_buf; i++) {

		buf = &device->bufs.frame_buffer[i];

		buf->virt_addr =
		(unsigned int)dma_alloc_coherent(&device->p_device->dev,
					size,
					(dma_addr_t *)&buf->phy_addr,
					GFP_ATOMIC);

		if (!buf->virt_addr) {
			for (j = 0; j < (i - 1); j++) {
				buf = &device->bufs.frame_buffer[j];
				dma_free_coherent(&device->p_device->dev,
						size,
						(void *)buf->virt_addr,
						buf->phy_addr);
				buf->virt_addr = 0;
				buf->phy_addr = 0;
			}
			ioh_err(device, "%s -> dma_alloc_coherent "
				"failed for Video Frame Buffer", __func__);
			return -ENOMEM;
		}
		buf->index = i;
		buf->data_size = 0;

		ioh_dbg(device, "%s i:%d v=0x%08x p=0x%08x, s=%d",
			__func__, device->bufs.frame_buffer[i].index,
				device->bufs.frame_buffer[i].virt_addr,
				device->bufs.frame_buffer[i].phy_addr,
				device->bufs.frame_buffer[i].data_size);
	}

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static int ioh_vin_free_frame_buffer(struct ioh_vin_device *device)
{
	size_t size;
	struct ioh_video_in_frame_buffer *buf;
	int i;

	ioh_dbg(device, "%s start", __func__);

	size = MAX_HEIGHT * MAX_BYTESPERLINE;

	for (i = 0; i < n_frame_buf; i++) {

		buf = &device->bufs.frame_buffer[i];

		if (buf->virt_addr) {
			dma_free_coherent(&device->p_device->dev,
						size,
						(void *)buf->virt_addr,
						buf->phy_addr);

			buf->index = 0;
			buf->virt_addr = 0;
			buf->phy_addr = 0;
			buf->data_size = 0;
		}
	}

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static struct ioh_video_in_frame_buffers
ioh_vin_get_frame_buffers(struct ioh_vin_device *device)
{
	ioh_dbg(device, "%s", __func__);

	ioh_dbg(device, "%s i:%d v=0x%08x p=0x%08x, s=%d",
		__func__, device->bufs.frame_buffer[0].index,
				device->bufs.frame_buffer[0].virt_addr,
				device->bufs.frame_buffer[0].phy_addr,
				device->bufs.frame_buffer[0].data_size);
	ioh_dbg(device, "%s i:%d v=0x%08x p=0x%08x, s=%d",
		__func__, device->bufs.frame_buffer[1].index,
				device->bufs.frame_buffer[1].virt_addr,
				device->bufs.frame_buffer[1].phy_addr,
				device->bufs.frame_buffer[1].data_size);
	ioh_dbg(device, "%s i:%d v=0x%08x p=0x%08x, s=%d",
		__func__, device->bufs.frame_buffer[2].index,
				device->bufs.frame_buffer[2].virt_addr,
				device->bufs.frame_buffer[2].phy_addr,
				device->bufs.frame_buffer[2].data_size);

	return device->bufs;
}

static void ioh_vin_init_intenb(struct ioh_vin_device *device)
{
	unsigned long flags;
	spin_lock_irqsave(&(device->dev_lock), flags);
	device->intenb.drevsem = VSYNC_SYNC;
	device->intenb.dmarenb = DISABLE;
	device->intenb.ofintenb = DISABLE;
	device->intenb.hsintenb = DISABLE;
	device->intenb.vsintenb = DISABLE;
	write_intenb(device);
	spin_unlock_irqrestore(&(device->dev_lock), flags);
}


static void ioh_vin_start_dma(struct ioh_vin_device *device)
{
	unsigned long flags;
	spin_lock_irqsave(&(device->dev_lock), flags);
	device->intenb.drevsem = VSYNC_SYNC;
	device->intenb.dmarenb = ENABLE;
	write_intenb(device);
	spin_unlock_irqrestore(&(device->dev_lock), flags);
}

static void ioh_vin_stop_dma(struct ioh_vin_device *device)
{
	unsigned long flags;
	spin_lock_irqsave(&device->dev_lock, flags);
	device->intenb.dmarenb = DISABLE;
	write_intenb(device);
	spin_unlock_irqrestore(&device->dev_lock, flags);
}


static void ioh_vin_start_interrupt(struct ioh_vin_device *device)
{
	unsigned long flags;

	spin_lock_irqsave(&(device->dev_lock), flags);
	device->intenb.ofintenb = ENABLE;
	device->intenb.hsintenb = DISABLE;
	device->intenb.vsintenb = ENABLE;
	write_intenb(device);
	spin_unlock_irqrestore(&(device->dev_lock), flags);
}

static void ioh_vin_stop_interrupt(struct ioh_vin_device *device)
{
	unsigned long flags;

	spin_lock_irqsave(&device->dev_lock, flags);
	device->intenb.ofintenb = DISABLE;
	device->intenb.hsintenb = DISABLE;
	device->intenb.vsintenb = DISABLE;
	write_intenb(device);
	spin_unlock_irqrestore(&device->dev_lock, flags);
}

static int ioh_vin_setup_hw(struct ioh_vin_device *device)
{
	int err;
	ioh_dbg(device, "%s start", __func__);

	ioh_vin_assert_reset(device);
	ioh_vin_de_assert_reset(device);
	ioh_vin_init_intenb(device);

	err = ioh_vin_set_input_format(device);
	if (err)
		return err;

	err = ioh_vin_set_output_format(device);
	if (err)
		return err;

	ioh_dbg(device, "%s ended", __func__);

	return 0;
}

static void ioh_vin_cleanup_hw(struct ioh_vin_device *device)
{
	ioh_dbg(device, "%s start", __func__);

	ioh_vin_assert_reset(device);
	ioh_vin_init_intenb(device);

	ioh_dbg(device, "%s ended", __func__);
}

#define VIN_CHECK_OK	0
#define VIN_CHECK_NG	1
#define VIN_CHECK_NO_DESC	2
static int ioh_vin_check_desc(struct ioh_vin_device *device)
{
	s32 channel = device->dma_channel;
	struct ioh_dma_desc desc_reg, *desc_mem;
	u16 status = 0;
	u32 i;
	u32 offset;
	u32 Y_comp, addition;
	s32 retval;

	if (device->dma_desc_virt == 0) {
		ioh_dbg(device, "%s:ioh_vin_cap_stop can be called\n", __func__);
		return VIN_CHECK_NO_DESC;
	}

	retval = ioh_get_dma_desc(channel, &desc_reg, &status);
	if (retval < 0) {

#ifdef OPERATION_COUNT
		#define DMA_STATUS_IDLE		(0)
		#define DMA_STATUS_DESC_READ	(1)
		#define DMA_STATUS_WAIT		(2)
		#define DMA_STATUS_ACCESS	(3)

		if (status == DMA_STATUS_IDLE)
			COUNTUP(count_address_idle);
		else if (status == DMA_STATUS_DESC_READ)
			COUNTUP(count_address_read);
		else if (status == DMA_STATUS_WAIT)
			COUNTUP(count_address_wait);
		else if (status == DMA_STATUS_ACCESS)
			COUNTUP(count_address_access);
		else
			COUNTUP(count_address_other);
#endif /* OPERATION_COUNT */

		ioh_dbg(device, "%s -> ioh_get_dma_desc "
				"failed(%d).", __func__, retval);

		return VIN_CHECK_NG;
	}

	ioh_dbg(device, "%s -> desc_reg.next= 0x%08x",
				__func__, desc_reg.nextDesc);

	Y_comp = device->pix_format.height;
	addition = ioh_vin_get_additional_line_size(device);

	for (i = 0; i < n_frame_buf; i++) {
		offset = i * (Y_comp + addition);
		desc_mem =
		  &((struct ioh_dma_desc *)device->dma_desc_virt)[offset];

		ioh_dbg(device, "%s -> desc_mem[%d].next= 0x%08x",
					__func__, i, desc_mem->nextDesc);

		if (desc_mem->nextDesc == desc_reg.nextDesc) {
			COUNTUP(count_address_match);
			return VIN_CHECK_OK;
		}
	}

	COUNTUP(count_address_nomatch);

	return VIN_CHECK_NG;
}

/* ------------------------------------------------------------------
 *	Interrupt handler
 * ------------------------------------------------------------------*/
static irqreturn_t ioh_vin_interrupt(int irq, void *dev_id)
{
	u32 insts;
	u32 emask;
	int err;
	void __iomem *base_address;
	irqreturn_t retval = IRQ_NONE;
	struct ioh_vin_device *device = dev_id;
	s32 channel = device->dma_channel;

	/* Obtaining the base address. */
	base_address = device->base_address;

	/* Reading the interrupt register. */
	insts = ((ioread32(base_address + IOH_VIN_INTSTS)) & INTBITS);
	emask = ((ioread32(base_address + IOH_VIN_INTENB)) & INTBITS);

	if ((emask & insts) == 0)
		goto out;

	if (((emask & insts) & VSINTSTS) != 0) {

		if (refresh_period >= 0)
			device->refresh_count++;

		if (err_flag) {
			ioh_vin_restart_dma(device);
			err_flag = 0; 
		}
		if (cap_start_flag) {
			err = ioh_set_dma_desc(device->dma_channel,
						    device->desc_start[0], device->desc_end);

			if (err) {
				ioh_err(device, "%s -> ioh_set_dma_desc "
						    "failed(%d)", __func__, err);
			}

			err = ioh_enable_dma(device->dma_channel);
			if (err) {
				ioh_err(device, "%s -> ioh_enable_dma "
						    "failed(%d)", __func__, err);
			}

			device->dma_fail_count = 0;

			ioh_vin_start_interrupt(device);
			ioh_vin_start_dma(device);
			cap_start_flag = 0;
		}
		retval = IRQ_HANDLED;
	}

	if (((emask & insts) & HSINTSTS) != 0)
		retval = IRQ_HANDLED;

	if (((emask & insts) & OFINTSTS) != 0) {
		COUNTUP(count_overflow_ipt);

#ifdef OVERFLOW_OFF
#else /* OVERFLOW_OFF */
		ioh_dbg(device, "%s -> overflow occured", __func__);
		ioh_disable_dma(channel);
		ioh_vin_setup_hw(device);
		ioh_vin_start_interrupt(device);
		err_flag = 1;
		COUNTUP(count_overflow_frame);
#endif /* OVERFLOW_OFF */

		retval = IRQ_HANDLED;
	}

out:
	iowrite32(insts, (base_address + IOH_VIN_INTSTS));

	return retval;
}

/* ------------------------------------------------------------------
 *	DMA Operations
 * ------------------------------------------------------------------*/
static void ioh_vin_dma_callback(int value, unsigned long dev)
{
	struct ioh_vin_device *device = (struct ioh_vin_device *)dev;

	COUNTUP(count_dma_callback);

	device->dma_status = value;
	if (cap_stop_flag == 0)
		ioh_vin_dma_complete(device);

}

static void ioh_vin_make_dma_descriptors(struct ioh_vin_device *device)
{
	u32 i, j;
	u32 index;
	struct ioh_dma_desc *start;
	struct ioh_dma_desc *end;
	struct ioh_dma_desc *desc = NULL;
	u32 paddr = device->physical_address;
	u32 addition;
	struct ioh_video_in_frame_buffer *buf;
	u32 height = device->pix_format.height;
	u32 bytesperline = device->pix_format.bytesperline;
	u32 offset;
	unsigned long flags;

	ioh_dbg(device, "%s start", __func__);
	print_v4l2_pix_format(device, &device->pix_format);

	spin_lock_irqsave(&(device->dev_lock), flags);

	addition = ioh_vin_get_additional_line_size(device);

	for (i = 0, index = 0; i < n_frame_buf; i++) {

		buf = &device->bufs.frame_buffer[i];

		/* Making one descriptor set */
		for (j = 0; j < height; j++, index++) {
			desc =
			&((struct ioh_dma_desc *)device->dma_desc_virt)[index];

			desc->insideAddress = paddr + IOH_VIN_VDATA;
			desc->outsideAddress = buf->phy_addr
						+ (j * bytesperline);
			desc->size = (bytesperline / 4);
			desc->nextDesc = ((u32)(device->dma_desc_phy +
				(index + 1) * sizeof(struct ioh_dma_desc)) &
				0xFFFFFFFC) | DMA_DESC_FOLLOW_WITHOUT_INTERRUPT;
		}

		/* Additional descriptor of one set are empty */
		for (j = 0; j < addition; j++, index++) {

			desc =
			&((struct ioh_dma_desc *)device->dma_desc_virt)[index];

			desc->insideAddress = paddr + IOH_VIN_VDATA;
			desc->outsideAddress = buf->phy_addr
						+ (j * bytesperline);
			desc->size = 0;
			desc->nextDesc = ((u32)(device->dma_desc_phy +
				(index + 1) * sizeof(struct ioh_dma_desc)) &
				0xFFFFFFFC) | DMA_DESC_FOLLOW_WITHOUT_INTERRUPT;
		}

		/* Last descriptor of one set is with interrupt */
		desc =
		&((struct ioh_dma_desc *)device->dma_desc_virt)[index - 1];

		desc->nextDesc = ((u32)(device->dma_desc_phy +
				index * sizeof(struct ioh_dma_desc)) &
				0xFFFFFFFC) | DMA_DESC_FOLLOW_WITH_INTERRUPT;

	}

	/* The chain address of the last descriptor of all of the set is
	   the first address of the first set */
	desc->nextDesc =
		((((u32)(device->dma_desc_phy)) & 0xFFFFFFFC) |
			DMA_DESC_FOLLOW_WITH_INTERRUPT);

	/* Save the start address of each of descriptor set */
	for (i = 0; i < n_frame_buf; i++) {
		offset = i * (height + addition);
		start = (struct ioh_dma_desc *)
			(((u32)(device->dma_desc_phy +
			offset * sizeof(struct ioh_dma_desc)) &
			0xFFFFFFFC) | DMA_DESC_FOLLOW_WITHOUT_INTERRUPT);
		device->desc_start[i] = start;
		ioh_dbg(device, "%s -> desc_start[%d] = 0x%08x",
				__func__, i, (u32)device->desc_start[i]);
	}

	/* Save the end address of descriptor set */
	offset = n_frame_buf * (height + addition) - 1;
	end = (struct ioh_dma_desc *)
		((u32)device->dma_desc_phy +
		offset * (sizeof(struct ioh_dma_desc)));
	device->desc_end = end;
	ioh_dbg(device, "%s -> desc_end = 0x%08x",
					__func__, (u32)device->desc_end);

	spin_unlock_irqrestore(&(device->dev_lock), flags);

	ioh_dbg(device, "%s ended", __func__);

	return;
}

static s32 ioh_vin_alloc_dma_desc(struct ioh_vin_device *device)
{
	int size;

	ioh_dbg(device, "%s start", __func__);

	size = MAX_DESC_NUM * sizeof(struct ioh_dma_desc);
	size = size * MAX_N_FRAME_BUF;

	device->dma_desc_virt = dma_alloc_coherent(
					&device->p_device->dev,
					size,
					&device->dma_desc_phy,
					GFP_ATOMIC);

	if (!device->dma_desc_virt) {
		ioh_err(device, "%s -> dma_alloc_coherent failed", __func__);
		return -ENOMEM;
	}

	ioh_dbg(device, "%s dma_desc_virt = 0x%08x",
					__func__, (u32)device->dma_desc_virt);
	ioh_dbg(device, "%s dma_desc_phy = 0x%08x",
					__func__, (u32)device->dma_desc_phy);
	ioh_dbg(device, "%s size = %d", __func__, size);
	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static s32 ioh_vin_free_dma_desc(struct ioh_vin_device *device)
{
	int size;

	ioh_dbg(device, "%s start", __func__);

	size = MAX_DESC_NUM * sizeof(struct ioh_dma_desc);
	size = size * MAX_N_FRAME_BUF;

	dma_free_coherent(&device->p_device->dev,
				size,
				device->dma_desc_virt,
				device->dma_desc_phy);

	device->dma_desc_virt = 0;
	device->dma_desc_phy = 0;

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static int ioh_vin_restart_dma(struct ioh_vin_device *device)
{
	s32 channel = device->dma_channel;
	int err;

	ioh_dbg(device, "%s start", __func__);
	
	if (device->frame_index == 0)
		err = ioh_set_dma_desc(channel,
				device->desc_start[1],
				device->desc_end);
	else if (device->frame_index == 1)
		err = ioh_set_dma_desc(channel,
				device->desc_start[2],
				device->desc_end);
	else
		err = ioh_set_dma_desc(channel,
				device->desc_start[0],
				device->desc_end);

	ioh_dbg(device, "%s -> ioh_set_dma_desc returned (%d)", __func__, err);

	err = ioh_enable_dma(channel);
	ioh_dbg(device, "%s -> ioh_enable_dma returned (%d)", __func__, err);

	ioh_vin_start_interrupt(device);
	ioh_vin_start_dma(device);

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

/* ------------------------------------------------------------------
 *	Streaming Operations
 * ------------------------------------------------------------------*/
#ifdef V2G_BRIDGE_IF
int ioh_video_in_register_callback(void *callback, void *token)
{
	ioh_vin_v2g_bridge_callback = callback;
	ioh_vin_v2g_bridge_token = token;

	return 0;
}
EXPORT_SYMBOL(ioh_video_in_register_callback);

static void ioh_vin_call_callback(struct ioh_vin_device *device)
{
	int retval;
	ioh_dbg(device, "%s", __func__);

	if (ioh_vin_v2g_bridge_callback) {

		retval = (ioh_vin_v2g_bridge_callback)
			(ioh_vin_v2g_bridge_token, device->frame_index);

		COUNTUP(count_v2g_callback);

		ioh_dbg(device, "%s -> ioh_vin_v2g_bridge_callback "
				"returned %d", __func__, retval);
	} else {
		ioh_dbg(device, "%s -> ioh_vin_v2g_bridge_callback "
				"is null", __func__);
	}

	return;
}

#endif /* V2G_BRIDGE_IF */

static void ioh_vin_dma_complete(struct ioh_vin_device *device)
{
	int *frame_index = &device->frame_index;
	int ret;
	s32 channel = device->dma_channel;

	ioh_dbg(device, "%s start. frame_index is %d", __func__, *frame_index);

	if (refresh_period >= 0 && device->refresh_count >= refresh_period) {

		ioh_dbg(device, "%s -> refresh check", __func__);

		device->refresh_count = 0;

		ret = ioh_vin_check_desc(device);
		if (ret == VIN_CHECK_NG) {
			err_flag = 1;
			ioh_dbg(device, "%s -> Descriptor check is failed, "
					"and refresh the image", __func__);
			COUNTUP(count_image_refresh);
			ioh_disable_dma(channel);
			ioh_vin_setup_hw(device);
			ioh_vin_start_interrupt(device);
			return;
		} else if (ret == VIN_CHECK_NO_DESC) {
			return;
		}
	}

	if (IOH_DMA_ABORT == device->dma_status) {
		ioh_err(device, "%s -> DMA transfer failed", __func__);

		if (device->dma_fail_count == 1) {
			ioh_err(device, "%s -> DMA transfer failed "
					"consecutively", __func__);
			return;
		}
		device->dma_fail_count += 1;
	}

	ioh_dbg(device, "%s -> DMA transfer completed successfully", __func__);

	device->bufs.frame_buffer[*frame_index].data_size
		= device->pix_format.bytesperline;

#ifdef V2G_BRIDGE_IF
	ioh_vin_call_callback(device);
#endif /* V2G_BRIDGE_IF */


	*frame_index += 1;
	*frame_index = *frame_index % n_frame_buf;

	device->dma_fail_count = 0;

	ioh_dbg(device, "%s ended", __func__);
	return;
}

/* ------------------------------------------------------------------
 *	Other Operations
 * ------------------------------------------------------------------*/
static int ioh_vin_open(struct ioh_vin_device *device)
{
	int err;
	ioh_dbg(device, "%s start", __func__);

	ioh_vin_assert_reset(device);
	ioh_vin_de_assert_reset(device);
	ioh_vin_init_intenb(device);

	device->dma_channel =
		ioh_request_dma(device->p_device, IOH_DMA_TX_DATA_REQ0);
	if (device->dma_channel < 0) {
		err = device->dma_channel;
		device->dma_channel = -1;
		ioh_err(device, "%s -> ioh_request_dma failed(%d)",
							__func__, err);
		goto out;
	}
	ioh_dbg(device, "%s -> ioh_request_dma invoked "
			"successfully(%d)", __func__, device->dma_channel);

	err = request_irq(device->irq, ioh_vin_interrupt, IRQF_SHARED,
				IOH_VIN_DRV_NAME, (void *)device);
	if (err) {
		ioh_err(device, "%s -> request_irq failed(%d)", __func__, err);
		goto out_free_dma;
	}

	/* Device hardware initialization. */
	device->cur_hw_config_in
		= ioh_vin_config_tables[device->cur_input].hw_config_in;

	device->cur_hw_config_out
		= ioh_vin_config_tables[device->cur_input].hw_config_out;

	err = ioh_vin_setup_hw(device);
	if (err) {
		ioh_err(device, "%s -> ioh_vin_setup_hw failed(%d)",
							__func__, err);
		goto out_free_irq;
	}

	ioh_dbg(device, "%s ended", __func__);
	return 0;

out_free_irq:
	free_irq(device->irq, (void *)device);
	ioh_dbg(device, "%s -> free_irq invoked successfully", __func__);

out_free_dma:
	if (device->dma_channel >= 0) {
		ioh_free_dma(device->dma_channel);
		device->dma_channel = -1;

		ioh_dbg(device, "%s -> ioh_free_dma "
				"invoked successfully.", __func__);
	}
out:
	ioh_dbg(device, "%s returns %d.", __func__, err);
	return err;
}

static int ioh_vin_close(struct ioh_vin_device *device)
{
	ioh_dbg(device, "%s start", __func__);

	ioh_vin_cleanup_hw(device);

	free_irq(device->irq, (void *)device);

	ioh_free_dma(device->dma_channel);
	device->dma_channel = -1;

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}


static int ioh_vin_cap_start(struct ioh_vin_device *device)
{
	struct ioh_dma_mode_param dma_mode = {
		.TransferDirection = IOH_DMA_DIR_IN_TO_OUT,
		.DMASizeType = IOH_DMA_SIZE_TYPE_32BIT,
		.DMATransferMode = DMA_SCATTER_GATHER_MODE
	};
	int err;

	ioh_dbg(device, "%s start", __func__);

	device->refresh_count = 0;
	device->frame_index = 0;
	cap_stop_flag = 0;

	err = ioh_vin_setup_hw(device);
	if (err) {
		ioh_err(device, "%s -> ioh_vin_setup_hw "
				"failed(%d)", __func__, err);
		return err;
	}

	err = ioh_vin_alloc_dma_desc(device);
	if (err) {
		ioh_err(device, "%s -> ioh_vin_alloc_dma_desc "
				"failed(%d)", __func__, err);
		return err;
	}

	err = ioh_set_dma_mode(device->dma_channel, dma_mode);
	if (err) {
		ioh_err(device, "%s -> ioh_set_dma_mode "
				"failed(%d)", __func__, err);
		goto out;
	}

	err = ioh_dma_set_callback(device->dma_channel,
					ioh_vin_dma_callback,
					(unsigned long)device);
	if (err) {
		ioh_err(device, "%s -> ioh_dma_set_callback "
				"failed(%d)", __func__, err);
		goto out;
	}

	ioh_vin_make_dma_descriptors(device);

	cap_start_flag = 1;

	ioh_vin_start_interrupt(device);
	ioh_dbg(device, "%s ended", __func__);
	return 0;

out:
	ioh_vin_free_dma_desc(device);

	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}

static void ioh_vin_cap_stop(struct ioh_vin_device *device)
{
	ioh_dbg(device, "%s start", __func__);

	cap_stop_flag = 1;

	ioh_vin_stop_interrupt(device);
	ioh_vin_stop_dma(device);

	msleep(80);

	ioh_disable_dma(device->dma_channel);

	ioh_vin_free_dma_desc(device);

#ifdef OPERATION_COUNT
//	ioh_info(device, "count_dma_callback is %d", count_dma_callback);
	ioh_info(device, "count_overflow_ipt is %d", count_overflow_ipt);
//	ioh_info(device, "count_overflow_frame is %d", count_overflow_frame);
//	ioh_info(device, "count_image_refresh is %d" , count_image_refresh);
//	ioh_info(device, "count_address_idle is %d", count_address_idle);
//	ioh_info(device, "count_address_read is %d", count_address_read);
//	ioh_info(device, "count_address_wait is %d", count_address_wait);
//	ioh_info(device, "count_address_access is %d", count_address_access);
//	ioh_info(device, "count_address_other is %d", count_address_other);
//	ioh_info(device, "count_address_match is %d", count_address_match);
//	ioh_info(device, "count_address_nomatch is %d", count_address_nomatch);
//	ioh_info(device, "count_v2g_callback is %d", count_v2g_callback);
#endif /* OPERATION_COUNT */

	ioh_dbg(device, "%s ended", __func__);
}

/* ------------------------------------------------------------------
 *	Videobuf operations
 * ------------------------------------------------------------------*/
static int buffer_setup(struct videobuf_queue *vq,
			unsigned int *count, unsigned int *size)
{
	struct ioh_vin_device *device = vq->priv_data;
	ioh_dbg(device, "%s start", __func__);

	ioh_dbg(device, "%s, before count=%d, size=%d\n", __func__,
							*count, *size);
	*size = MAX_HEIGHT * MAX_BYTESPERLINE;

	if (0 == *count)
		*count = 32;

	while (*size * *count > vid_limit * 1024 * 1024)
		(*count)--;

	ioh_dbg(device, "%s, after  count=%d, size=%d\n", __func__,
							*count, *size);
	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct ioh_vin_buffer *buf)
{
	struct ioh_vin_device *device = vq->priv_data;
	ioh_dbg(device, "%s start", __func__);

	ioh_dbg(device, "%s, state: %i\n", __func__, buf->vb.state);

	if (in_interrupt())
		BUG();

	videobuf_vmalloc_free(&buf->vb);
	ioh_dbg(device, "free_buffer: freed\n");
	buf->vb.state = VIDEOBUF_NEEDS_INIT;
	ioh_dbg(device, "%s ended", __func__);
}

static int buffer_prepare(struct videobuf_queue *vq,
			struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct ioh_vin_device *device = vq->priv_data;
	struct ioh_vin_buffer *buf =
				container_of(vb, struct ioh_vin_buffer, vb);
	int rc;
	ioh_dbg(device, "%s start", __func__);

	ioh_dbg(device, "%s, field=%d\n", __func__, field);

	buf->vb.size = device->pix_format.sizeimage;

	ioh_dbg(device, "%s, bsize=%d\n", __func__, buf->vb.bsize);
	ioh_dbg(device, "%s,  size=%ld\n", __func__, buf->vb.size);

	if (0 != buf->vb.baddr  &&  buf->vb.bsize < buf->vb.size)
		return -EINVAL;

	buf->vb.width  = device->pix_format.width;
	buf->vb.height = device->pix_format.height;
	buf->vb.field  = field;

	ioh_dbg(device, "%s, state=%d\n", __func__, buf->vb.state);

	if (VIDEOBUF_NEEDS_INIT == buf->vb.state) {
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}

	buf->vb.state = VIDEOBUF_PREPARED;

	ioh_dbg(device, "%s ended", __func__);
	return 0;

fail:
	free_buffer(vq, buf);
	return rc;
}

static void buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	struct ioh_vin_device *device = vq->priv_data;
	struct ioh_vin_buffer    *buf  =
				container_of(vb, struct ioh_vin_buffer, vb);

	ioh_dbg(device, "%s", __func__);

	buf->vb.state = VIDEOBUF_QUEUED;
	list_add_tail(&buf->vb.queue, &device->vidq_active);
}

static void buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	struct ioh_vin_device *device = vq->priv_data;
	struct ioh_vin_buffer   *buf  =
				container_of(vb, struct ioh_vin_buffer, vb);
	ioh_dbg(device, "%s", __func__);

	free_buffer(vq, buf);
}

static struct videobuf_queue_ops ioh_vin_video_qops = {
	.buf_setup      = buffer_setup,
	.buf_prepare    = buffer_prepare,
	.buf_queue      = buffer_queue,
	.buf_release    = buffer_release,
};

/* ------------------------------------------------------------------
 *	IOCTL vidioc handling
 * ------------------------------------------------------------------*/
static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	strcpy(cap->driver, "ioh_vin");
	strcpy(cap->card, "ioh_vin");
	strlcpy(cap->bus_info, device->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = IOH_VIN_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct ioh_vin_device *device = priv;
	const struct ioh_vin_config *cfg;
	ioh_dbg(device, "%s", __func__);

	/* The current setting is the only acceptable parameters. */
	if (f->index != 0)
		return -EINVAL;

	cfg = &ioh_vin_config_tables[device->cur_input];

	strlcpy(f->description, cfg->vin_input.name, sizeof(f->description));
	f->pixelformat = cfg->pix_format.pixelformat;
	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	f->fmt.pix = device->pix_format;

	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *fmt)
{
	struct ioh_vin_device *device = priv;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	ioh_dbg(device, "%s start", __func__);
	print_v4l2_pix_format(device, pix);

	/* This driver ignores the input, and returns the current parameters. */
	*pix = device->pix_format;

	print_v4l2_pix_format(device, pix);

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}


static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *fmt)
{
	struct ioh_vin_device *device = priv;
	int ret;

	ioh_dbg(device, "%s start", __func__);

	print_v4l2_pix_format(device, &device->pix_format);

	if (test_bit(IOH_VIN_FLAG_START_CAP, &device->s_flags))
		return -EBUSY;

	ret = vidioc_try_fmt_vid_cap(file, device, fmt);

	if (ret < 0)
		return ret;

	/* This driver ignores the input, and returns the current parameters. */
	/* So, the replacement of pix_format is unnecessary */

	device->vb_vidq.field = fmt->fmt.pix.field;	/* */

	print_v4l2_pix_format(device, &device->pix_format);

	ioh_dbg(device, "%s ended(%d)", __func__, ret);
	return ret;
}

static int vidioc_reqbufs(struct file *file, void *priv,
				struct v4l2_requestbuffers *p)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	return videobuf_reqbufs(&device->vb_vidq, p);
}

static int vidioc_querybuf(struct file *file,
					void *priv, struct v4l2_buffer *p)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	return videobuf_querybuf(&device->vb_vidq, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	return videobuf_qbuf(&device->vb_vidq, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	return videobuf_dqbuf(&device->vb_vidq, p,
				file->f_flags & O_NONBLOCK);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct ioh_vin_device *device = priv;
	int err;

	ioh_dbg(device, "%s", __func__);

	if (device->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (i != device->type)
		return -EINVAL;

	err = ioh_vin_cap_start(device);
	if (err)
		return -EINVAL;

	set_bit(IOH_VIN_FLAG_START_CAP, &device->s_flags);

	return videobuf_streamon(&device->vb_vidq);
}

static int vidioc_streamoff(struct file *file,
					void *priv, enum v4l2_buf_type i)
{
	struct ioh_vin_device *device = priv;

	ioh_dbg(device, "%s", __func__);

	if (device->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (i != device->type)
		return -EINVAL;

	ioh_vin_cap_stop(device);

	clear_bit(IOH_VIN_FLAG_START_CAP, &device->s_flags);

	return videobuf_streamoff(&device->vb_vidq);
}

static int vidioc_enum_input(struct file *file, void *priv,
				struct v4l2_input *inp)
{
	struct ioh_vin_device *device = priv;
	const struct v4l2_input *input_table;
	ioh_dbg(device, "%s", __func__);

	if (inp->index >= ARRAY_SIZE(ioh_vin_config_tables))
		return -EINVAL;

	input_table = &ioh_vin_config_tables[inp->index].vin_input;

	strlcpy(inp->name, input_table->name, sizeof(inp->name));
	inp->type = input_table->type;
	inp->std = input_table->std;

	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct ioh_vin_device *device = priv;
	ioh_dbg(device, "%s", __func__);

	*i = device->cur_input;

	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct ioh_vin_device *device = priv;

	ioh_dbg(device, "%s start", __func__);

	if (test_bit(IOH_VIN_FLAG_START_CAP, &device->s_flags))
		return -EBUSY;

	if (i >= ARRAY_SIZE(ioh_vin_config_tables))
		return -EINVAL;

	device->cur_input = i;

	device->pix_format = ioh_vin_config_tables[i].pix_format;

	ioh_dbg(device, "%s index is %d", __func__, i);
	print_v4l2_pix_format(device, &device->pix_format);

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static long vidioc_default(struct file *p_file, void *priv,
		int command, void *param)
{
	struct ioh_vin_device *device = priv;
	int err = -EAGAIN;

	ioh_dbg(device, "%s start", __func__);

	switch (command) {

	/* For getting the buffer size. */
	case IOH_VIDEO_GET_BUFFER_SIZE:
		ioh_dbg(device, "%s IOH_VIDEO_GET_BUFFER_SIZE", __func__);
		{
			unsigned long buffer_size;
			buffer_size = ioh_vin_get_buffer_size(device);
			memcpy((void *)param, (void *)&buffer_size,
							sizeof(buffer_size));
			err = 0;
		}
		break;

	/* For getting the frame buffer info. */
	case IOH_VIDEO_GET_FRAME_BUFFERS:
		ioh_dbg(device, "%s IOH_VIDEO_GET_FRAME_BUFFERS", __func__);
		{
			struct ioh_video_in_frame_buffers buffers;
			buffers = ioh_vin_get_frame_buffers(device);
			memcpy((void *)param, (void *)&buffers,
							sizeof(buffers));
			err = 0;
		}
		break;

	default:
		ioh_err(device, "%s -> Invalid ioctl command", __func__);
		err = -EINVAL;
		break;

	}
	/* End of switch */

	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}

/* ------------------------------------------------------------------
 *	File Operations for the Device
 * ------------------------------------------------------------------*/

static int ioh_vin_v4l2_open(struct file *file)
{
	struct ioh_vin_device *device = video_drvdata(file);
	int err = 0;

	mutex_lock(&device->s_mutex);

	ioh_dbg(device, "%s start", __func__);

	if (test_bit(IOH_VIN_FLAG_OPENED, &device->s_flags)) {
		err = -EBUSY;
		ioh_err(device, "%s -> Device already opened(%d)",
			__func__, err);
		goto out;
	}


	ioh_info(device, "open %s type=%s\n",
		video_device_node_name(device->vfd),
		v4l2_type_names[V4L2_BUF_TYPE_VIDEO_CAPTURE]);

	file->private_data = device;

	device->type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	videobuf_queue_vmalloc_init(&device->vb_vidq,
			&ioh_vin_video_qops, NULL, &device->slock,
			device->type, V4L2_FIELD_ANY,
			sizeof(struct ioh_vin_buffer), device, NULL);

	err = ioh_vin_open(device);
	if (err) {
		ioh_err(device, "%s -> ioh_vin_open failed(%d)",
				__func__, err);
		goto out;
	}

	set_bit(IOH_VIN_FLAG_OPENED, &device->s_flags);
out:
	mutex_unlock(&device->s_mutex);
	return err;
}

static int ioh_vin_v4l2_close(struct file *file)
{
	struct ioh_vin_device *device = file->private_data;
	struct video_device  *vdev = video_devdata(file);

	mutex_lock(&device->s_mutex);

	ioh_dbg(device, "%s start", __func__);

	if (test_bit(IOH_VIN_FLAG_OPENED, &device->s_flags)) {
		ioh_vin_close(device);

		videobuf_stop(&device->vb_vidq);
		videobuf_mmap_free(&device->vb_vidq);
	}

	clear_bit(IOH_VIN_FLAG_OPENED, &device->s_flags);

	ioh_info(device, "close called (device=%s)\n",
				video_device_node_name(vdev));

	mutex_unlock(&device->s_mutex);
	return 0;
}

static unsigned int ioh_vin_v4l2_poll(struct file *file,
					struct poll_table_struct *wait)
{
	struct ioh_vin_device *device = file->private_data;
	struct videobuf_queue *q = &device->vb_vidq;
	unsigned int ret;

	ioh_dbg(device, "%s start\n", __func__);

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != device->type)
		return POLLERR;

	ret = videobuf_poll_stream(file, q, wait);

	ioh_dbg(device, "%s ret=0x%04x\n", __func__, ret);

	return ret;
}

static int ioh_vin_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ioh_vin_device *device = file->private_data;
	int ret;

	ioh_dbg(device, "%s mmap called, vma=0x%08lx\n",
				__func__, (unsigned long)vma);

	ret = videobuf_mmap_mapper(&device->vb_vidq, vma);

	ioh_dbg(device, "%s vma start=0x%08lx, size=%ld, ret=%d\n",
		__func__, (unsigned long)vma->vm_start,
		(unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
		ret);

	return ret;
}

static long ioh_vin_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct ioh_vin_device *device = video_drvdata(file);
	long ret;

	mutex_lock(&device->s_mutex);

	ret = video_ioctl2(file, cmd, arg);

	mutex_unlock(&device->s_mutex);

	return ret;
}

static const struct v4l2_file_operations ioh_vin_fops = {
	.owner		= THIS_MODULE,
	.open		= ioh_vin_v4l2_open,
	.release	= ioh_vin_v4l2_close,
	.poll		= ioh_vin_v4l2_poll,
	.unlocked_ioctl	= ioh_vin_ioctl,
	.mmap		= ioh_vin_mmap,
};

static const struct v4l2_ioctl_ops ioh_vin_ioctl_ops = {
	.vidioc_querycap	= vidioc_querycap,
	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap	= vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap	= vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap	= vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs		= vidioc_reqbufs,
	.vidioc_querybuf	= vidioc_querybuf,
	.vidioc_qbuf		= vidioc_qbuf,
	.vidioc_dqbuf		= vidioc_dqbuf,
	.vidioc_enum_input	= vidioc_enum_input,
	.vidioc_g_input		= vidioc_g_input,
	.vidioc_s_input		= vidioc_s_input,
	.vidioc_streamon	= vidioc_streamon,
	.vidioc_streamoff	= vidioc_streamoff,
	.vidioc_default		= vidioc_default,
};

static struct video_device ioh_vin_template = {
	.name		= "ioh_vin",
	.fops		= &ioh_vin_fops,
	.ioctl_ops	= &ioh_vin_ioctl_ops,
	.release	= video_device_release_empty,

	.tvnorms	= V4L2_STD_NTSC_M,
	.current_norm	= V4L2_STD_NTSC_M,
};

/* ------------------------------------------------------------------
 *	PCI interface Operations
 * ------------------------------------------------------------------*/
static int __devinit ioh_vin_set_frame_buffer(struct ioh_vin_device *device)
{
	int err;

	ioh_dbg(device, "%s start", __func__);

	if (n_frame_buf < MIN_N_FRAME_BUF)
		n_frame_buf = MIN_N_FRAME_BUF;
	if (n_frame_buf > MAX_N_FRAME_BUF)
		n_frame_buf = MAX_N_FRAME_BUF;

	err = ioh_vin_alloc_frame_buffer(device);

	if (err) {
		ioh_err(device, "%s -> ioh_vin_alloc_frame_buffer "
				"failed(%d)", __func__, err);
		goto out;
	}

out:
	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}
static int ioh_vin_clr_frame_buffer(struct ioh_vin_device *device)
{
	int err;

	ioh_dbg(device, "%s start", __func__);

	err = ioh_vin_free_frame_buffer(device);

	if (err) {
		ioh_err(device, "%s -> ioh_vin_free_frame_buffer "
				"failed(%d)", __func__, err);
		goto out;
	}
out:
	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}

static int __devinit ioh_vin_init_device(struct ioh_vin_device *device)
{
	struct video_device *vfd;
	int err;

	ioh_dbg(device, "%s start", __func__);

	spin_lock_init(&device->slock);
	spin_lock_init(&device->dev_lock);
	mutex_init(&device->s_mutex);

	mutex_lock(&device->s_mutex);

	ioh_vin_assert_reset(device);
	ioh_vin_de_assert_reset(device);
	ioh_vin_init_intenb(device);

	device->s_flags = 0;

	device->dma_channel = -1;
	device->cur_input = INPUT_DEFAULT;
	device->pix_format = ioh_vin_config_tables[INPUT_DEFAULT].pix_format;

	err = ioh_vin_set_frame_buffer(device);
	if (err) {
		ioh_err(device, "%s -> ioh_vin_set_frame_buffer "
				"failed(%d)", __func__, err);
		goto ini_ret;
	}

	snprintf(device->v4l2_dev.name, sizeof(device->v4l2_dev.name),
			"%s", IOH_VIN_DRV_NAME);
	err = v4l2_device_register(NULL, &device->v4l2_dev);
	if (err)
		goto clr_frame;

	INIT_LIST_HEAD(&device->vidq_active);

	err = -ENOMEM;
	vfd = video_device_alloc();
	if (!vfd)
		goto unreg_dev;

	*vfd = ioh_vin_template;

	err = video_register_device(vfd, VFL_TYPE_GRABBER, video_nr);
	if (err)
		goto rel_vdev;

	video_set_drvdata(vfd, device);

	device->vfd = vfd;

	v4l2_info(&device->v4l2_dev, "V4L2 device registered as %s\n",
						video_device_node_name(vfd));

#ifdef V2G_BRIDGE_IF
	ioh_vin_v2g_bridge_token = NULL;
	ioh_vin_v2g_bridge_callback = NULL;
#endif /* V2G_BRIDGE_IF */

	mutex_unlock(&device->s_mutex);

	ioh_dbg(device, "%s ended", __func__);
	return 0;

rel_vdev:
	video_device_release(vfd);
unreg_dev:
	v4l2_device_unregister(&device->v4l2_dev);
clr_frame:
	ioh_vin_clr_frame_buffer(device);
ini_ret:
	mutex_unlock(&device->s_mutex);

	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return err;
}

static int __devexit ioh_vin_remove(struct ioh_vin_device *device)
{
	ioh_dbg(device, "%s start", __func__);

	ioh_vin_clr_frame_buffer(device);

	v4l2_info(&device->v4l2_dev, "V4L2 device unregistering %s\n",
					video_device_node_name(device->vfd));
	video_unregister_device(device->vfd);
	v4l2_device_unregister(&device->v4l2_dev);

	ioh_dbg(device, "%s ended", __func__);
	return 0;
}

static int __devinit ioh_vin_pci_probe(struct pci_dev *pdev,
						const struct pci_device_id *id)
{
	int err;
	struct ioh_vin_device *device;

	printk(KERN_DEBUG "%s start", __func__);

	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device) {
		printk(KERN_ERR "ioh_video_in : %s "
				"kzalloc(device) failed", __func__);
		err = -ENOMEM;
		goto out;
	}

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR "ioh_video_in : %s "
				"pci_enable_device failed (%d)",
				__func__, err);
		goto out_free_device;
	}

	pci_set_master(pdev);

	device->physical_address = pci_resource_start(pdev, 1);
	if (0 == device->physical_address) {
		printk(KERN_ERR "ioh_video_in : %s "
				"Cannot obtain the physical address",
				__func__);
		err = -ENOMEM;
		goto out_pcidev;
	}

	device->base_address = pci_iomap(pdev, 1, 0);
	if (device->base_address == NULL) {
		printk(KERN_ERR "ioh_video_in : %s "
				"pci_iomap failed(0x%08x)",
				__func__, (u32) device->base_address);
		err = -ENOMEM;
		goto out_pcidev;
	}

	device->p_device = pdev;
	device->irq = pdev->irq;

	pci_set_drvdata(pdev, device);

	err = ioh_vin_init_device(device);
	if (err) {
		printk(KERN_ERR "ioh_video_in : %s "
				"ioh_vin_init_device failed(%d)",
				__func__, err);
		goto out_iounmap;
	}

	ioh_dbg(device, "%s ended(%d)", __func__, err);
	return 0;

out_iounmap:
	pci_iounmap(pdev, device->base_address);
out_pcidev:
	pci_disable_device(pdev);
out_free_device:
	kfree(device);
out:
	printk(KERN_DEBUG "%s ended(%d)", __func__, err);
	return err;
}

static void __devexit ioh_vin_pci_remove(struct pci_dev *pdev)
{
	struct ioh_vin_device *device;

	device = (struct ioh_vin_device *)pci_get_drvdata(pdev);

	mutex_lock(&device->s_mutex);

	ioh_vin_remove(device);

	pci_iounmap(pdev, device->base_address);

	pci_disable_device(pdev);

	pci_set_drvdata(pdev, NULL);

	mutex_unlock(&device->s_mutex);

	kfree(device);
}

static struct pci_device_id ioh_video_pcidev_id[] __devinitdata = {
	{PCI_DEVICE(PCI_VENDOR_ID_IOH, PCI_DEVICE_ID_IVI_VIDEOIN)},
	{PCI_DEVICE(PCI_VENDOR_ID_IOH, PCI_DEVICE_ID_MP_VIDEOIN)},
	{}
};

MODULE_DEVICE_TABLE(pci, ioh_video_pcidev_id);

static struct pci_driver ioh_video_driver = {
	.name     = "ioh_video_in",
	.id_table = ioh_video_pcidev_id,
	.probe    = ioh_vin_pci_probe,
	.remove   = __devexit_p(ioh_vin_pci_remove),
};

static int __init ioh_vin_pci_init(void)
{
	int err;

	err = pci_register_driver(&ioh_video_driver);

	if (err) {
		printk(KERN_ERR "ioh_video_in : "
				"pci_register_driver failed (%d)", err);
	}
	return err;
}

static void __exit ioh_vin_pci_exit(void)
{
	pci_unregister_driver(&ioh_video_driver);
}

module_init(ioh_vin_pci_init);
module_exit(ioh_vin_pci_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IOH video-in PCI Driver");

