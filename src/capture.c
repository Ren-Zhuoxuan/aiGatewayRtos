#include "capture.h"


int capture_frame(const char* device,const char* output)
{
    int fd;
    int ret;
    struct v4l2_format image_fmt;
    struct v4l2_capability image_capability;
    
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
        //4.1 申请缓冲区 - 告诉驱动我们需要几个缓冲区
        struct v4l2_requestbuffers v4l2_req;
        memset(&v4l2_req,0,sizeof(v4l2_req));
        v4l2_req.count = 1;
        v4l2_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_req.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(fd,VIDIOC_REQBUFS,&v4l2_req);
        if (ret < 0)
        {
            perror("Request buffer error:");
            close(fd);
            return -1;
        }
        printf("Request buffer count : %d\n",v4l2_req.count);

        //4.2 查询并且映射缓冲区 - 把驱动的缓冲区映射到我们的进程空间
        struct v4l2_buffer v4l2_buf;
        memset(&v4l2_buf,0,sizeof(v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = 0;

        ret = ioctl(fd,VIDIOC_QUERYBUF,&v4l2_buf);
        if (ret < 0)
        {
            perror("Query buffer error:");
            close(fd);
            return -1;
        }
        printf("Query buffer success!!!\n");
        
        //内存映射  ------有疑问 待学习
        void *mmap_buffer = mmap(NULL,v4l2_buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd,v4l2_buf.m.offset);
        if (mmap_buffer == MAP_FAILED)
        {
            perror("Mmap buffer error :");
            close(fd);
            return -1;
        }
        printf("Mmap buffer success, size:%d!!!\n",v4l2_buf.length);
    
        //4.3 缓冲区入队 - 把缓冲区交给驱动 
        ret = ioctl(fd,VIDIOC_QBUF,&v4l2_buf);
        if (ret < 0)
        {
            perror("Buffer enter queue error:");
            munmap(mmap_buffer,v4l2_buf.length);
            close(fd);
            return -1;
        }
        

        //4.4 开始采集 - 启动数据流
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd,VIDIOC_STREAMON,&type);
        if (ret < 0)
        {
            perror("Start get data error:");
            munmap(mmap_buffer,v4l2_buf.length);
            close(fd);
            return -1;
        }


        //4.5 缓冲区出队 - 获取有数据的缓冲区
        ret = ioctl(fd,VIDIOC_DQBUF,&v4l2_buf);
        if (ret < 0)
        {
            perror("Buffer dequeue error:");
            munmap(mmap_buffer,v4l2_buf.length);
            close(fd);
            return -1;
        }

        printf("Success get one picture!!!\n");

        outfile = fopen(OUTPUT_PICTURE,"wb");
        if (!outfile)
        {
            perror("Open outfile error:\n");
            munmap(mmap_buffer,v4l2_buf.length);
            close(fd);
            return -1;
        }

        ret = fwrite(mmap_buffer,1,v4l2_buf.bytesused,outfile);
        fclose(outfile);


        ret = ioctl(fd,VIDIOC_STREAMOFF,&type);
        munmap(mmap_buffer,v4l2_buf.length);
    }
    else
    {
        // 分配缓冲区存储图像数据 
        size_t buffer_size = image_fmt.fmt.pix.sizeimage;
        unsigned char* image_buffer = malloc(buffer_size);
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

    printf("获取YUV图像成功\n");
    // 清理资源
    close(fd);
    return 0;
}