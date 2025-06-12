#include <stdio.h>
#include <sys/stat.h>
#include "capture.h"


int main(int argc, char *argv[])
{
    int result = 0;
    //阶段一:采集图片
    result = capture_frame(USB_CAMERA_PATH,OUTPUT_PICTURE);
    if (result == 0)
    {
        printf("Stage1:capture_frame success!!!!!!!\n");
    }
    else
    {
        printf("Stage1:capture_frame have error!!!!!!!\n");
    }

    //转换格式 yuv->rgb
    result = convert_yuv_to_rgb(OUTPUT_PICTURE, OUTPUT_RGB, 640, 360);
    if (result == 0)
    {
        printf("Stage2:convert success!!!!!!!\n");
    }
    else
    {
        printf("Stage2:convert have error!!!!!!!\n");
    }
    
    
    return 0;
}
