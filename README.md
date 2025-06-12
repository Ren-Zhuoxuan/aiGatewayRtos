# 项目简介
​	此项目基于3568,目的是构建AI边缘网关设备,第一阶段能采集摄像头图像，运行简单目标检测（RKNN或OpenCV），然后通过串口或网络协议,结合STM32,发送检测结果（如位置、状态）



# 安装环境

开发板基于RK3568,系统目前是debian11

开发板需要安装:

```
sudo apt install gedit gnome-terminal build-essential v4l-utils libjpeg-dev ffmpeg -y
```



使用下面命令查看图片

```
打开yuv
ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x360 output/frame.yuv
打开rgb
ffplay -f rawvideo -pixel_format rgb24 -video_size 640x360 frame.rgb
```





# 项目结构

main.c: 程序入口
include/: 函数声明头文件
src/: 图像采集实现
output/: 保存图像文件



