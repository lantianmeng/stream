#ifndef FILEDRIVER_H_
#define FILEDRIVER_H_

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include <libavutil/avstring.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libavutil/parseutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>
#include <libavutil/error.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <iostream>
#include <string>

class IStreamData
{
public:
	virtual bool streamData(uint8_t * &data, int size) = 0;
};

//ffmpeg解码h264
//转为ts流（udp）
//转为ts流（rtp）
class CFileDriver
{
public:
	CFileDriver(std::string fileName, IStreamData *handle):m_fileName(fileName), m_handle(handle) {}
	~CFileDriver(){}

	bool init()
	{
		av_register_all();

		int32_t ret = 0;

		do 
		{
			m_fmtCtxIn = avformat_alloc_context();
			if ( !m_fmtCtxIn )
			{
				ret = AVERROR_UNKNOWN;
				//LOGE("CRtmpStreamHandler avformat_alloc_context error: %s. \n", err2str(ret).c_str());
				std::cout << "avformat_alloc_context error" << std::endl;
				break;
			}

			ret = avformat_open_input( &m_fmtCtxIn,  m_fileName.c_str(), NULL, NULL );
			if ( ret < 0 )
			{
				//LOGE("CEsToTs avformat_open_input error: %s. \n", err2str(ret).c_str());
				std::cout << "avformat_open_input error" << std::endl;
				break;
			}

			ret = avformat_find_stream_info( m_fmtCtxIn, NULL );
			if ( ret < 0 )
			{
				//LOGE("CEsToTs avformat_find_stream_info error: %s. \n", err2str(ret).c_str());
				std::cout << "avformat_find_stream_info error" << std::endl;
				break;
			}

			for ( uint32_t i = 0; i < m_fmtCtxIn->nb_streams; ++i )
			{
				AVStream *in_stream = m_fmtCtxIn->streams[i];
				if ( !in_stream )
				{
					continue;
				}

				if ( in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO )
				{  
					m_inputVideoIndex = i;
					break;
				}  
				else if ( in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO )
				{  
				}
			}

			//打印流格式
			av_dump_format(m_fmtCtxIn, 0, m_fileName.c_str(), 0);

			//解码

		} while (0);

		return (ret < 0) ? false : true;
	}

	//读取帧数据
	int readStreamData()
	{
		AVPacket pkt;
		av_init_packet( &pkt );

		int32_t ret = 0;

		while(true)
		{
			ret = av_read_frame( m_fmtCtxIn, &pkt );
			if (ret < 0)
			{
				if (ret == AVERROR_EOF)
				{
					std::cout << "av_read_frame end of file" << std::endl;
					break;
				}
				std::cout << "av_read_frame error" << std::endl;
				break;
			}

			if ( pkt.stream_index == -1 )
			{
				av_packet_unref( &pkt );
				continue;
			}

			if ( pkt.stream_index != m_inputVideoIndex )
			{
				av_packet_unref( &pkt );
				continue;
			}

			m_handle->streamData(pkt.data, pkt.size);

		}

		return ret;
	}
private:
	std::string  m_fileName;
	IStreamData *m_handle;
	AVFormatContext*                       m_fmtCtxIn;
	uint32_t                               m_inputVideoIndex;
	
	
};

#endif
