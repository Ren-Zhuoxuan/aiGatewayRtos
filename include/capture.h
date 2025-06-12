#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <jpeglib.h>
#include <linux/videodev2.h>
#include <sys/mman.h>  // for mmap

#define USB_CAMERA_PATH           "/dev/video0"
#define OUTPUT_PICTURE            "output/frame.yuv"
#define OUTPUT_RGB                "output/frame.rgb"

int capture_frame(const char* device,const char* output);
int convert_yuv_to_rgb(const char* yuv_file, const char* rgb_file, 
                       int width, int height);

#endif