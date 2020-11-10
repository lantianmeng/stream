# stream
- 需求
<br>解码h264文件，（yuv数据或rgb数据），播放
- 技术点
  + H264码流（文件）格式 
    <br>[H264格式 详细介绍](https://blog.csdn.net/shixin_0125/article/details/78940402)
  + ffmpeg解码H264码流（yuv数据和rgb数据）
    <br>[FFmpeg解码H264视频裸流(直接可用)](https://blog.csdn.net/lizhijian21/article/details/80495684)  **貌似有点问题，demo中已经修改了**
    <br>[FFmpeg: FFmepg中的sws_scale() 函数分析](https://www.cnblogs.com/yongdaimi/p/10715830.html),这里有讲解AVFrame data中数据的含义以及转换RGB时参数的含义
  + directDraw播放yuv数据
    <br>directDraw显示yuv相关的参考资料（CStreamHandler::showYUV），demo 注释中有列出
  + qt 播放rgb数据
    <br>解码后的数据，缓存队列RGBWnd.h  boost::circular_buffer
	<br>[Qt--QLabel显示视频，CPU占比问题小结](https://blog.csdn.net/qq_38880380/article/details/80925902)
    <br>[openGL显示视频 ](https://blog.csdn.net/wanghualin033/article/details/79683836)
