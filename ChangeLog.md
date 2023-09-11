# 变更日志

## 2023-09-11
  * acl: Rename demo base64 to coder.

## 2023-09-05
  * acl  : test https
  * asio2: continue to develop socks5 proxy function 2.

## 2023-08-26
  * libdrm: build: bump version to 2.4.116
  * acl   : mod test sample.

## 2023-08-18
  * acl   : Merge branch 'master' of gitee:zsxxsz/acl into gitee-master
  * libdrm: nouveau: add interface to make buffer objects global
  * libv4l: Add --get/--set-ctrl support for INTEGER and INTEGER64 arrays

## 2023-08-14
  * 更新rkmpp源码
  * cpp-tbox: fix:修复TcpConnector::stop()的错误
  * acl     : test db module in pkv.

## 2023-08-05
  * 更新rkmpp源码
  * cpp-tbox: eat(base):优化LogBacktrace(),添加日志等级参数

## 2023-07-30
  * cpp-tbox: Update log.h
  * libdrm  : xf86drm: add drmSyncobjEventfd

## 2023-07-21
  * linux-rga-multi: update to 1.9.3_[0]
  * cpp-tbox       : tidy: 添加文件头
  * acl            : Optimize and test pkv.

## 2023-06-30
  * 更新rkmpp源码
  * live555更新版本到2023-06-20

## 2023-06-13
  * live555更新版本到2023-06-10

## 2023-05-29
  * libv4l: sync with latest media staging tree

## 2023-05-26
  * 更新rkmpp源码

## 2023-04-21
  * 更新rkmpp源码

## 2023-03-31
  * live555更新版本到2023-03-20
  * 基于AX620A实现简单的MP4解码

## 2023-03-16
  * asio2: 更新版本

## 2023-03-15
  * 基于AX620A实现一个简单的算力盒的应用

## 2023-03-13
  * 重新根据AX620A的pipeline构建源码，新建EMS案例

## 2023-03-12
  * 调整AX620A的pipeline源码

## 2023-03-11
  * 增加liblog
  * 更新xlib版本至2.76.0
  * 增加英伟达jetson系列的案例集成

## 2023-03-09
  * 新增新的RtspServer接口
  * 更新RKMPP内容

## 2023-03-07
  * 封装AXERA NPU接口，隐藏上下文实例句柄
  * 将会新信息更新至IVPS(AXERA)

## 2023-03-06
  * 调整AXERA media API接口参数，适应无摄像头或有摄像头的初始化
  * 增加AXERA NPU推理接口，当前存在不合理，需要调整
  * 增加使用live555接口实现的RTSP服务器

## 2023-03-05
  * 封装AX620A的MPI接口
  * 修复RTSP接口问题(每个URL增加TAG标识)

## 2023-03-03
  * 实现在帧上绘制操作
  * AX620A库先简化(去除gstreamer)
  * 调整日志信息，调整NPU目录结构，将共性的提取出来共用，避免冗余
  * 修改MPI的接口，将Region合入MediaApi中，统一抽象接口

## 2023-03-02
  * v4l-utils增加rockchip的补丁，同时其插件支持libv4l-rkmpp
  * 编译使用于rv1126的opencv默认不依赖任何的第三方库
  * 在EMS案例上实现camera->npu->rtsp流程；同时实现rtsp客户端拉取网络流进行视频解码推理显示(只实现至拉流后的数据操作)

## 2023-03-01
  * 增加贡献作者记录
  * 增加变更日志记录
  * 修改EMS案例的构造函数(实际修改mpi的基类接口，避免后续的复杂)
  * 调整一些目录结构，以便同一厂商的接口可以共用
  * 在EMS案例上实现一些简单的检测算法
  * 优化简化版的RTSP服务器接口
