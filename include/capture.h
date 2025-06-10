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

#define USB_CAMERA_PATH "/dev/video0"
#define OUTPUT_PICTURE  "output/frame.jpg"

int capture_frame(const char* device,const char* output);


#endif