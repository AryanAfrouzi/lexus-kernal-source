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

#ifndef __IOH_VIDEO_IN_H__
#define __IOH_VIDEO_IN_H__

/* includes */
#include <linux/ioctl.h>

/*
 *	ioh_video_in_frame_buffer
 *	The structure for holding the video frame data.
 */
struct ioh_video_in_frame_buffer {
	int index;		/* Buffer index */
	unsigned int virt_addr;	/* Frame Buffer virtaul address */
	unsigned int phy_addr;	/* Frame Buffer Physical address */
	unsigned int data_size;	/* data size */
};

/*
 *	Maximum frame buffers to be allocated.
 */
#define MAXIMUM_FRAME_BUFFERS		(5)

/*
 *	ioh_video_in_frame_buffers
 *	The structure of some frame buffers.
 */

struct ioh_video_in_frame_buffers {
	struct ioh_video_in_frame_buffer frame_buffer[MAXIMUM_FRAME_BUFFERS];
};

/*
 *	VIDEO_IN_IOCTL_MAGIC
 *	Outlines the byte value used to define the differnt ioctl commands.
 */
#define VIDEO_IN_IOCTL_MAGIC	'V'
#define BASE			BASE_VIDIOC_PRIVATE

/*
 *	IOH_VIDEO_GET_BUFFER_SIZE
 *	Outlines the value specifing the ioctl command for
 *	getting the buffer size.
 *	This ioctl command is issued for getting the buffer
 *	size. The expected parameter is a
 *	user level address which points to a variable of type
 *	unsigned long, to which the buffer
 *	size has to be updated.
 */
#define IOH_VIDEO_GET_BUFFER_SIZE	(_IOR(VIDEO_IN_IOCTL_MAGIC,\
BASE + 21, unsigned long))

/*
 *	IOH_VIDEO_GET_FRAME_BUFFERS
 *	Outlines the value specifing the ioctl command for
 *	read the information of the frame buffers.
 *	This ioctl command is issued to get the information of
 *	frame buffers.
 *	The expected parameter is a user level address which
 *	points to a variable of type struct
 *	ioh_video_in_frame_buffer.
 */
#define IOH_VIDEO_GET_FRAME_BUFFERS	(_IOR(VIDEO_IN_IOCTL_MAGIC,\
BASE + 24, struct ioh_video_in_frame_buffers))


#endif
