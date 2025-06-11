#include "capture.h"


int capture_frame(const char* device,const char* output)
{
    int fd;
    int ret;
    struct v4l2_format image_fmt;
    struct v4l2_capability image_capability;
    size_t buffer_size;
    unsigned char* image_buffer;
    FILE *outfile;

    //1 打开设备 - 使用open()打开设备文件
    fd = open(device,O_RDWR);
    if (fd < 0)
    {
        perror("Open camera  device fail:\n");
        close(fd);
        return -1;
    }
    printf("Open camera device success:\n");
    printf("\n");
    
    //2 查询能力 - 确认设备支持什么功能
    ret = ioctl(fd,VIDIOC_QUERYCAP,&image_capability);
    if (ret < 0)
    {
        perror("Get capability fail:");
        close(fd);
        return -1;
    }
    printf("USB Device : Driver name:%s\n",image_capability.driver);
    printf("USB Device : Card name:%s\n",image_capability.card);

    //检查USB支不支持R/W,现在很多摄像头只支持streaming模式,不支持简单的read()模式
    printf("USB Device   Capability check:\n");
    if (image_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        printf("Support video capture\n");
    if (image_capability.capabilities & V4L2_CAP_READWRITE)
        printf("Support read/write mode\n");
    else
        printf("Don't support read/write mode，must use streaming!!!!!\n");
    if (image_capability.capabilities & V4L2_CAP_STREAMING)
        printf("support streaming mode!!!\n");
    printf("\n");


    //3 设置格式 - 告诉设备我们要什么格式的图像 目前用的是YUV格式
    memset(&image_fmt,0,sizeof(struct v4l2_format));
    image_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    image_fmt.fmt.pix.width = 640;
    image_fmt.fmt.pix.height = 360;
    image_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    ret = ioctl(fd,VIDIOC_S_FMT,&image_fmt);
    if (ret < 0)
    {
        perror("Set image format fail:");
        close(fd);
        return -1;
    }
    printf("Set image format success: %dx%d YUYV , Size:%d Bytes\n", image_fmt.fmt.pix.width, image_fmt.fmt.pix.height,image_fmt.fmt.pix.sizeimage);
    printf("\n");

    //4 采集数据 - 不支持read()方式的话,映射内存获取图像数据
    if (!(image_capability.capabilities & V4L2_CAP_READWRITE))
    {
        //申请缓冲区 - 告诉驱动我们需要几个缓冲区

        //内存映射 - 把驱动的缓冲区映射到我们的进程空间

        //缓冲区入队 - 把缓冲区交给驱动

        //开始采集 - 启动数据流

        //缓冲区出队 - 获取有数据的缓冲区

    }
    else
    {
        // 分配缓冲区存储图像数据 
        buffer_size = image_fmt.fmt.pix.sizeimage;
        image_buffer = malloc(buffer_size);
        if (!image_buffer)
        {
            perror("Malloc image size error:\n");
            close(fd);
            return -1;
        }
        printf("Malloc image size success!!!\n");
        printf("\n");

        
        // 直接从设备读取一帧数据
        int bytes_read = read(fd,image_buffer,buffer_size);
        if (bytes_read == -1)
        {
            perror("Read image error:");
            free(image_buffer);
            close(fd);
            return -1;
        }
        printf("Read image success!!!\n");
        
        // 保存原始YUYV数据到文件
        outfile = fopen(output,"wb");
        if (!outfile)
        {
            perror("Open outfile error:\n");
            free(image_buffer);
            close(fd);
            return -1;
        }

        ret = fwrite(image_buffer,1,bytes_read,outfile);
        fclose(outfile);
        free(image_buffer);
    }


    // 清理资源
    close(fd);
    return 0;
}