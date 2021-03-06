# 视频监控与音视频基础知识
- demo需求
<br>解码h264文件，（yuv数据或rgb数据），播放
<br>，转成ts流（udp），播放
<br>，存储，rtp推送（ts）
<br>，存储，rtmp推送 （后面有相关的资料）
<br>，存储，rtsp推送
  + 关于格式信息
  h264码流（裸流）：NALU
  [H264格式 详细介绍](https://blog.csdn.net/shixin_0125/article/details/78940402)
  + 参考资料
  <br>[ffmpeg文件流](https://www.cnblogs.com/leisure_chn/p/10623968.html)
  <br>[流媒体基础知识TS流 PS流 ES流区别](https://blog.csdn.net/xswy1/article/details/81609427)
  <br>[FFmpeg解码H264视频裸流(直接可用)](https://blog.csdn.net/lizhijian21/article/details/80495684)
  <br>[ffmpeg解码和SDL播放](https://blog.csdn.net/leixiaohua1020/article/details/8652605) **这个得重点学习和理解**，另涉及SDL库
- 设计
 + 概要设计 or 方案论证
<br>若是从socket收到的数据,就不用ffmpeg读取数据了，只需要用它解码就行了
<br>socket收到数据后进行解析，组成一帧，放到帧队列，然后从帧队列中取帧数据，条用ffmpeg解码函数，就能解出数据，然后再用directshow进行播放。就完成了

<br>可以不用filter 直接搞定
<br>接收是一部分 解码时一部分 显示是一部分
<br>接收可以用jrtplib，解码可以用ffmpeg 显示可以用directdraw
<br>filter可以直接用ffdshow就可以


<br>///////////////
<br>GB28181 [视频监控平台与国标28181](https://blog.csdn.net/songxiao1988918)
<br>视频知识基础：TS流和PS流 YUV的理解 
<br>[流媒体基础知识TS流 PS流 ES流区别](https://blog.csdn.net/xswy1/article/details/81609427)
<br>RTP head 中区分：PS/TS流/ES流 h264码流，甚至可以包含音频数据

<br>///////////
<br>[音视频开发入门篇](https://www.cnblogs.com/wei-chen-linux/p/11097763.html)
<br>
<br>////ffmpeg
<br>[ffmpeg文件流](https://www.cnblogs.com/leisure_chn/p/10623968.html)
<br>[[总结]FFMPEG视音频编解码零基础学习方法](https://blog.csdn.net/leixiaohua1020/article/details/15811977)
<br>在这个之下，得好好的理解，来思考自己学习和前进的方向
<br>[字节跳动视频编解码面经](https://blog.csdn.net/huster1446/article/details/101542085?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase)

<br>[实时流（直播流）播放、上墙（大屏播放）解决方案](https://www.cnblogs.com/xiaozhi_5638/p/8664841.html)  **关注点在人**，貌似是公司的一个leader

<br>//音视频 中的多篇blog
<br>[WebRTC学习](https://blog.csdn.net/caoshangpa/category_9267799.html)

<br>/////////
<br>[h264码流（裸流)转ts流](https://www.cnblogs.com/dyan1024/p/10224538.html)
<br>[YUV RGB色彩空间](https://www.cnblogs.com/huxiaopeng/p/5653257.html)

<br>//视频数据，需要缓存
<br>[用circular_buffer实现的播放缓存队列](https://blog.csdn.net/mo4776/article/details/80089017)
<br>[走读Webrtc 中的视频JitterBuffer(一)](https://blog.csdn.net/mo4776/article/details/108570043)


<br>高清摄像头 双码流：主码流和子码流  QCIF/CIF/D1编码。主码流用于本地录像存储，子码流用于网络传输
<br>图像质量（图像清晰程度）：分辨率、码率、帧率

<br>1080P/720P/D1/CIF/QCIF  分辨率不一样
<br>1080P是两百万像bai素的，分辨率达到du1920*1080；
<br>720P是一百万像素的，分辨率达到960*720/1280*720；
<br>D1的分zhi辨率达到704*576；
<br>CIF的分辨率达到352*288；
<br>SCIF没听dao说，应该是QCIF，QCIF的分辨率达到176*144。

- 技术  关于rtmp流媒体
<br>ffmpeg将h264文件转为rtmp流，推流给rtmp服务器（nginx-rtmp模块），rtmp客户端拉流（vlc）

<br>[rtmp传输h.264视频的必备知识（一）](https://blog.csdn.net/dqxiaoxiao/article/details/94820599)
<br>[转换H264视频流到RTMP服务器](https://blog.csdn.net/hcmsdc/article/details/82178937)

<br>关于RTMP
<br>现有demo（h264码流），修改为作为rtmp服务器推送rtmp码流（flv/f4v）
<br>[windows下搭建rtmp流媒体服务](https://blog.csdn.net/qq_32381727/article/details/81078213?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.add_param_isCf&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.add_param_isCf)

<br>[使用ffmpeg循环推流(循环读取视频文件)推送RTMP服务器的方法](https://blog.csdn.net/cai6811376/article/details/74783269)

<br>ffmpeg版本低于2.5,不支持循环播的参数-stream_loop -1 [](https://www.cnblogs.com/zzugyl/p/7607557.html)
<br>[推流工具OBS](https://blog.csdn.net/xiejiashu/article/details/68483758)
<br>[安装报错系统缺少OBS Studio的运行时组件](https://www.fujieace.com/jingyan/runtime-components.html)

<br>[基于FFmpeg进行RTMP推流（一）](https://www.jianshu.com/p/69eede147229)

+ rtmp协议， nginx-rtmp的相关配置
 
<br>[带你吃透RTMP协议](https://www.jianshu.com/p/b2144f9bbe28)
//
<br>nginx-rtmp配置存在问题时，nginx可以启动。但推流失败。通过抓包即可分析

<br>//关于nginx-rtmp drop_idle_publisher配置
<br>[在实战中使用nginx-rtmp遇到的TCP连接问题分析](https://www.cnblogs.com/harlanc/p/10896015.html)
<br>[nginx.conf中关于nginx-rtmp-module配置指令详解](https://www.cnblogs.com/lidabo/p/7099501.html)

<br>//可以配置all/record/redirect/drop  **drop断流功能**
<br>[nginx-rtmp如何实现关闭推流，停止正在直播的视频？ ](https://bbs.csdn.net/topics/392008696)
<br>[【nginx-rtmp】01、控制模块（Control module）](https://blog.csdn.net/envon123/article/details/76588690)

<br>//nginx proxy_pass
<br>http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_pass

<br>  +http-flv的方式，来播放rtmp流
<br>[用FFMPEG推流时，自动判断是否停止推流](http://blog.sina.com.cn/s/blog_88f05dbd0102ww48.html)

<br>[关于直播，所有的技术细节都在这里了](http://blog.chinaunix.net/uid-9407839-id-5748156.html)

<br>[基于nginx-rtmp-module模块实现的HTTP-FLV直播模块nginx-http-flv-module（一）](https://blog.csdn.net/winshining/article/details/74910586)
<br>[nginx-http-flv-module 基于 nginx-rtmp-module 的流媒体服务器。](https://github.com/winshining/nginx-http-flv-module/blob/master/README.CN.md)
<br>
<br>[使用rtmp协议推送H264裸码流](https://blog.csdn.net/zzqgtt/article/details/81603543)

- 一些常常使用的命令
<br>ffmpeg -re -i tmp8.mp4 -vcodec copy -f flv rtmp://192.168.3.123:1935/live

<br>ps -ef |grep nginx
<br>netstat -ano |grep 1935

<br>tcpdump port 1935 -w ./11.cap

<br>//jrtplib jthread ffmpeg ddraw/dsound
<br>//sip库  pjsip


- 保活机制
    + tcp的保活机制
    + http的保活机制
