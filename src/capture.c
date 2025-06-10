#include "capture.h"


int capture_frame(const char* device,const char* output)
{
    int fd;
    //打开设备 - 使用open()打开设备文件
    fd = open(device,O_RDWR);
    if (fd < 0)
    {
        printf("Open camera  device fail!!!\n");
        return -1;
    }
    
    //查询能力 - 确认设备支持什么功能
    
    //设置格式 - 告诉设备我们要什么格式的图像
    
    //采集数据 - 读取或映射内存获取图像数据

    return 0;
}