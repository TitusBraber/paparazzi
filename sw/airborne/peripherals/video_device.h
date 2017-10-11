/*
 * Copyright (C) 2015
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file peripherals/video_device.h
 */

#ifndef VIDEO_DEVICE_H
#define VIDEO_DEVICE_H

#include <stdbool.h>
#include <inttypes.h>
#include "modules/computer_vision/lib/vision/image.h"

/* Different video filters */
#ifndef VIDEO_FILTER_DEBAYER ///<To be able to switch it off
#define VIDEO_FILTER_DEBAYER  (0x1 << 0)  ///<Enable software debayer
#endif

#define VIDEO_FILTER_ISP      (0x1 << 1)  ///<Enable ISP

// Main video_thread structure
struct video_thread_t {
  volatile bool is_running;       ///< When the device is running
  struct v4l2_device *dev;        ///< The V4L2 device that is used for the video stream
};

/** V4L2 device settings */
struct video_config_t {
  struct img_size_t output_size;    ///< Output image size
  struct img_size_t sensor_size;    ///< Original sensor size
  struct crop_t crop;           ///< Cropped area definition
  char *dev_name;           ///< path to device
  char *subdev_name;        ///< path to sub device
  uint32_t format;          ///< Video format
  uint32_t subdev_format;   ///< Subdevice video format
  uint8_t buf_cnt;          ///< Amount of V4L2 video device buffers
  uint8_t filters;          ///< filters to use (bitfield with VIDEO_FILTER_x)
  struct video_thread_t thread; ///< Information about the thread this camera is running on
  struct video_listener *cv_listener; ///< The first computer vision listener in the linked list for this video device
  int fps;                  ///< Target FPS
};
extern struct video_config_t dummy_camera;


#endif
