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



/**
 * 将YUYV格式转换为RGB格式（为RKNN准备）
 * @param yuv_file 输入的YUV文件路径
 * @param rgb_file 输出的RGB文件路径  
 * @param width 图像宽度
 * @param height 图像高度
 * @return 成功返回0，失败返回-1
 */
int convert_yuv_to_rgb(const char* yuv_file, const char* rgb_file, 
                       int width, int height)
{
    FILE *yuv_fp, *rgb_fp;
    unsigned char *yuv_data, *rgb_data;
    int yuv_size, rgb_size;
    
    printf("Stage2:YUV to RGB convert ===\n");
    printf("Input: %s (%dx%d YUYV)\n", yuv_file, width, height);
    printf("Output: %s (%dx%d RGB)\n", rgb_file, width, height);
    
    // 计算数据大小
    yuv_size = width * height * 2;  // YUYV每像素2字节
    rgb_size = width * height * 3;  // RGB每像素3字节
    
    // 分配内存
    yuv_data = (unsigned char*)malloc(yuv_size);
    rgb_data = (unsigned char*)malloc(rgb_size);
    
    if (!yuv_data || !rgb_data) {
        printf("Error: Memory allocation failed\n");
        if (yuv_data) free(yuv_data);
        if (rgb_data) free(rgb_data);
        return -1;
    }
    
    // 读取YUV文件
    yuv_fp = fopen(yuv_file, "rb");
    if (!yuv_fp) {
        printf("Error: Failed to open YUV file %s\n", yuv_file);
        free(yuv_data);
        free(rgb_data);
        return -1;
    }
    
    int bytes_read = fread(yuv_data, 1, yuv_size, yuv_fp);
    fclose(yuv_fp);
    
    if (bytes_read != yuv_size) {
        printf("Warning: Read byte count %d not equal to expected %d\n", bytes_read, yuv_size);
    }
    
    // YUYV到RGB转换
    printf("Start convert YUYV → RGB...\n");
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j += 2) {
            // YUYV格式：Y0 U0 Y1 V0（4字节表示2个像素）
            int yuv_index = (i * width + j) * 2;
            int rgb_index1 = (i * width + j) * 3;      // 第一个像素
            int rgb_index2 = (i * width + j + 1) * 3;  // 第二个像素
            
            unsigned char y1 = yuv_data[yuv_index];
            unsigned char u  = yuv_data[yuv_index + 1];
            unsigned char y2 = yuv_data[yuv_index + 2];
            unsigned char v  = yuv_data[yuv_index + 3];
            
            // YUV到RGB转换公式
            int r1 = y1 + 1.402 * (v - 128);
            int g1 = y1 - 0.344 * (u - 128) - 0.714 * (v - 128);
            int b1 = y1 + 1.772 * (u - 128);
            
            int r2 = y2 + 1.402 * (v - 128);
            int g2 = y2 - 0.344 * (u - 128) - 0.714 * (v - 128);
            int b2 = y2 + 1.772 * (u - 128);
            
            // 限制RGB值范围 [0, 255]
            rgb_data[rgb_index1]     = (r1 > 255) ? 255 : (r1 < 0) ? 0 : r1;
            rgb_data[rgb_index1 + 1] = (g1 > 255) ? 255 : (g1 < 0) ? 0 : g1;
            rgb_data[rgb_index1 + 2] = (b1 > 255) ? 255 : (b1 < 0) ? 0 : b1;
            
            rgb_data[rgb_index2]     = (r2 > 255) ? 255 : (r2 < 0) ? 0 : r2;
            rgb_data[rgb_index2 + 1] = (g2 > 255) ? 255 : (g2 < 0) ? 0 : g2;
            rgb_data[rgb_index2 + 2] = (b2 > 255) ? 255 : (b2 < 0) ? 0 : b2;
        }
    }
    
    // 保存RGB文件
    rgb_fp = fopen(rgb_file, "wb");
    if (!rgb_fp) {
        printf("Error: Failed to create RGB file %s\n", rgb_file);
        free(yuv_data);
        free(rgb_data);
        return -1;
    }
    
    fwrite(rgb_data, 1, rgb_size, rgb_fp);
    fclose(rgb_fp);
    
    printf("✅ Convert completed!\n");
    printf("RGB file size: %d bytes\n", rgb_size);
    printf("RKNN usable RGB data saved to: %s\n", rgb_file);
    
    // 清理内存
    free(yuv_data);
    free(rgb_data);
    
    return 0;
}