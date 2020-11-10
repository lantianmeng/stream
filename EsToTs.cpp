#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "EsToTs.hpp"
#include "CircularQueue.hpp"

#define BUF_SIZE        (16 * 1024)         // ffpmeg每次从原始数据缓冲区中取的原始数据大小，单位字节.
#define DATAQUEUE_SIZE  (2 * 1024 * 1024)   // 原始数据缓冲区的大小，单位字节.

#define LOGD(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#define ERR_INFO_SIZE   64

namespace htc
{

	static std::string err2str( int32_t err )
	{
		char szErrInfo[ERR_INFO_SIZE]={0};
		av_make_error_string( szErrInfo, ERR_INFO_SIZE, err);
		return std::string(szErrInfo);
	}

    int32_t lockmgr(void **mtx, enum AVLockOp op)
    {
        switch( op ) 
        {
        case AV_LOCK_CREATE:
            {
                *mtx = new boost::mutex();
                if(!*mtx)
                    return 1;
                return 0;
            }
        case AV_LOCK_OBTAIN:
            {
                ((boost::mutex*)*mtx)->lock();
                return 0;
            }
        case AV_LOCK_RELEASE:
            {
                ((boost::mutex*)*mtx)->unlock();
                return 0;
            }
        case AV_LOCK_DESTROY:
            {
                boost::mutex* pMtx = (boost::mutex*)(*mtx);
                if ( pMtx )
                {
                    delete pMtx;
                    pMtx = 0;
                }
                return 0;
            }
        default:
            break;
        }
        return 1;
    }

CEsToTs::CEsToTs( Ssp::IStreamData* serHandler )
:m_serHandler( serHandler )
, m_quit(true)
, m_streamData()
, m_fmtCtxIn(NULL)
, m_inputVideoIndex(-1)
, m_fmtCtxOut(NULL)
, m_outputVideoIndex(-1)
, m_thread()
, m_streamBuff( NULL )
, m_buildPacketLen( 0 )
, m_streamBuffLen( 0 )
{

}


CEsToTs::~CEsToTs()
{

}

void CEsToTs::streamHandlerThread()
{
	bool ret = false;
	if ( initInput() && initOutput() )
	{
		ret = transform();
	}

	finiOutput();
	finiInput();

	if (( !ret ) && ( m_serHandler))
	{
		m_serHandler->streamBreak();
	}

}

bool CEsToTs::transform()
{
	int32_t ret          = 0;
	int64_t last_pts     = 0;

	AVStream* in_stream  = NULL;
	AVStream* out_stream = NULL;

	AVPacket pkt;
	av_init_packet( &pkt );
	bool findKey = false;
	while ( true && false == m_quit )
	{
		ret = av_read_frame( m_fmtCtxIn, &pkt );
		if ( ret < 0 ) 
		{
			if ( ret == AVERROR_EOF || url_feof( m_fmtCtxIn->pb ) || m_fmtCtxIn->pb->error )
			{
				LOGE("CStreamAdapterHk av_read_frame error1: %s. \n", err2str(ret).c_str());
				break;
			}
			else
			{
				LOGE("CStreamAdapterHk av_read_frame error2: %s. \n", err2str(ret).c_str());
				continue;
			}
		}


		if ( pkt.stream_index == STREAM_INDEX_INVAIL )
		{
			av_packet_unref( &pkt );
			continue;
		}

		if ( pkt.stream_index != m_inputVideoIndex )
		{
			av_packet_unref( &pkt );
			continue;
		}

		if ( !findKey )
		{  // 找到关键帧后开始封包处理，否则可能会有问题
			if ( pkt.flags & AV_PKT_FLAG_KEY ){
				findKey = true;
			}
			else 
			{
				av_packet_unref( &pkt );
				continue;
			}
		}
		// 内部生成时间戳
		AVRational time_base = m_fmtCtxIn->streams[m_inputVideoIndex]->time_base;
		AVRational frame_rate = m_fmtCtxIn->streams[m_inputVideoIndex]->r_frame_rate;
		if ( time_base.num == 0 || time_base.den == 0 || frame_rate.num == 0 || frame_rate.den == 0 )
		{
			LOGE("CStreamAdapterHk time_base:%d/%d or frame_rate:%d/%d error. \n", time_base.num, time_base.den, frame_rate.num, frame_rate.den );
			av_packet_unref( &pkt );
			continue;
		}
		int64_t duration = 1 / ( av_q2d( frame_rate ) * av_q2d( time_base ) );
		pkt.pts = last_pts + duration;
		pkt.dts = pkt.pts;  
		pkt.duration = duration;  
		last_pts = pkt.pts;

		in_stream  = m_fmtCtxIn->streams[m_inputVideoIndex];
		out_stream = m_fmtCtxOut->streams[m_outputVideoIndex];
		pkt.stream_index = m_outputVideoIndex;

		// copy packet
		pkt.pts = av_rescale_q_rnd( pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );  
		pkt.dts = av_rescale_q_rnd( pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );  
		pkt.duration = av_rescale_q( pkt.duration, in_stream->time_base, out_stream->time_base );  
		pkt.pos = -1;

		ret = av_interleaved_write_frame( m_fmtCtxOut, &pkt );
		if ( ret < 0 )
		{ 
			av_packet_unref( &pkt );
			LOGE("CStreamAdapterHk av_interleaved_write_frame error: %s. \n", err2str(ret).c_str());
			break;
		}

		av_packet_unref( &pkt );
	}

	av_write_trailer( m_fmtCtxOut );

	return (ret < 0) ? false : true;
}

bool CEsToTs::initInput()
{
	int32_t ret = 0;
	do 
	{
		m_fmtCtxIn = avformat_alloc_context();
		if ( !m_fmtCtxIn )
		{
			ret = AVERROR_UNKNOWN;
			LOGE("CRtmpStreamHandler avformat_alloc_context error: %s. \n", err2str(ret).c_str());
			break;
		}

		AVInputFormat* inputFmt = NULL;

		// 推流
		uint8_t* pBuf = ( uint8_t* )av_mallocz( sizeof(uint8_t) * BUF_SIZE );
		if ( !pBuf )
		{
			ret = AVERROR_UNKNOWN;  
			LOGE("CEsToTs av_mallocz error: %s. \n", err2str(ret).c_str());
			break;
		}

		AVIOContext* ioCtx = avio_alloc_context( pBuf, BUF_SIZE, 0, this, readData, NULL, NULL );
		if ( !ioCtx )
		{
			av_free( pBuf );
			pBuf = NULL;

			ret = AVERROR_UNKNOWN;
			LOGE("CEsToTs in avio_alloc_context error: %s. \n", err2str(ret).c_str());
			break;
		}

		ret = av_probe_input_buffer( ioCtx, &inputFmt, NULL, NULL, 0, 0 );
		if ( ret < 0 )
		{
			av_free( pBuf );
			av_free( ioCtx );
			ioCtx = NULL;

			LOGE("CEsToTs in av_probe_input_buffer error: %s. \n", err2str(ret).c_str());
			break;
		}

		m_fmtCtxIn->pb = ioCtx;
		m_fmtCtxIn->flags=AVFMT_FLAG_CUSTOM_IO;

		ret = avformat_open_input( &m_fmtCtxIn,  NULL, inputFmt, NULL );
		if ( ret < 0 )
		{
			LOGE("CEsToTs avformat_open_input error: %s. \n", err2str(ret).c_str());
			break;
		}

		std::string name;
		std::string long_name;
		if ( inputFmt && inputFmt->name )
		{
			name = inputFmt->name;
			long_name = inputFmt->long_name ? inputFmt->long_name : "none";
		}
		else if ( m_fmtCtxIn->iformat && m_fmtCtxIn->iformat->name )
		{
			name = m_fmtCtxIn->iformat->name;
			long_name = m_fmtCtxIn->iformat->long_name ? m_fmtCtxIn->iformat->long_name : "none";
		}
		else
		{
			LOGE("CEsToTs cann't probe stream format. \n");

			ret = AVERROR_UNKNOWN;
			break;
		}


		m_fmtCtxIn->max_analyze_duration = 2 * AV_TIME_BASE;
		ret = avformat_find_stream_info( m_fmtCtxIn, NULL );
		if ( ret < 0 )
		{
			LOGE("CEsToTs avformat_find_stream_info error: %s. \n", err2str(ret).c_str());
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

		av_dump_format( m_fmtCtxIn, 0, NULL, 0 );

		if ( m_inputVideoIndex == STREAM_INDEX_INVAIL )
		{
			LOGE("CEsToTs not find video stream. \n");

			ret = AVERROR_STREAM_NOT_FOUND;
			break;
		}

		ret = url_feof(m_fmtCtxIn->pb);
		if ( ret < 0 )
		{
			LOGE("CEsToTs avio_feof error: %s. \n", err2str(ret).c_str());
			break;
		}

	} while ( false );

	return (ret < 0) ? false : true;
}

void CEsToTs::finiInput()
{
	if ( !m_fmtCtxIn )
	{
		return;
	}
	if ( ( m_fmtCtxIn->flags & AVFMT_FLAG_CUSTOM_IO ) && m_fmtCtxIn->pb )
	{
		if ( m_fmtCtxIn->pb->buffer )
		{
			av_free( m_fmtCtxIn->pb->buffer );
			m_fmtCtxIn->pb->buffer = NULL;
		}
		//avio_context_free( &m_fmtCtxIn->pb );
		av_free(m_fmtCtxIn->pb);
	}

	avformat_close_input(&m_fmtCtxIn);
	//avformat_free_context( m_fmtCtxIn );
	m_fmtCtxIn = NULL;
}

bool CEsToTs::initOutput()
{
	int32_t ret = 0;

	do
	{
		ret = avformat_alloc_output_context2( &m_fmtCtxOut, NULL, "mpegts", NULL );
		if ( ret < 0 )
		{
			LOGE("CEsToTs avformat_alloc_output_context2 error: %s. \n", err2str(ret).c_str());
			break;
		}

		AVOutputFormat *outputFmt = m_fmtCtxOut->oformat;
		if ( !outputFmt )
		{
			ret = AVERROR_UNKNOWN;  
			LOGE("CEsToTs outputFmt is empty error: %s. \n", err2str(ret).c_str());
			break;
		}

		unsigned char* outbuffer=NULL;
		outbuffer=(unsigned char*)av_malloc(32*1024);  // 32K
		if (NULL == outbuffer){
			ret = AVERROR_UNKNOWN;  
			LOGE("CEsToTs outputFmt alloc output memory fail \n");
			break;
		}

		AVIOContext *avio_out=NULL;
		avio_out =avio_alloc_context(outbuffer, 32768, 1, (void*)this,NULL,writeData,NULL); 
		if(avio_out==NULL)
		{
			ret = AVERROR_UNKNOWN;  
			av_free(outbuffer);
			LOGE("CEsToTs outputFmt avio_alloc_context fail \n");
			break;
		}

		m_fmtCtxOut->pb=avio_out; 
		m_fmtCtxOut->flags=AVFMT_FLAG_CUSTOM_IO;

		ret = AVERROR_UNKNOWN;
		for ( uint32_t i = 0; i < m_fmtCtxIn->nb_streams; ++i )
		{
			if ( i == m_inputVideoIndex ){
				AVStream *in_stream = m_fmtCtxIn->streams[i];
				AVStream *out_stream = avformat_new_stream(m_fmtCtxOut, in_stream->codec->codec);
				if (!out_stream) {
					std::cout << "CEsToTs Failed allocating output stream" << std::endl;
					break;
				}
				ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
				if (ret < 0) {
					std::cout << "CEsToTs Failed to copy context from input to output stream codec context" << std::endl;
					break;
				}
				out_stream->codec->codec_tag = 0;
				out_stream->start_time = 0;
				out_stream->avg_frame_rate = in_stream->avg_frame_rate;

				if (m_fmtCtxOut->oformat->flags & AVFMT_GLOBALHEADER)
					out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

				m_outputVideoIndex = m_fmtCtxOut->nb_streams-1;
				break;
			}
		}
		av_dump_format(m_fmtCtxOut, 0, NULL, 1);
		ret = avformat_write_header(m_fmtCtxOut, NULL);
	} while ( false );

	return (ret < 0) ? false : true;
}

void CEsToTs::finiOutput()
{
	if ( !m_fmtCtxOut )
	{
		return;
	}

	if ( ( m_fmtCtxOut->flags & AVFMT_FLAG_CUSTOM_IO ) &&  m_fmtCtxOut->pb )
	{
		if ( m_fmtCtxOut->pb->buffer )
		{
			av_free( m_fmtCtxOut->pb->buffer );
			m_fmtCtxOut->pb->buffer = NULL;
		}
		//avio_context_free( &m_fmtCtxOut->pb );
		av_free(m_fmtCtxOut->pb);  // 释放自己分配内存
	}

	avformat_free_context( m_fmtCtxOut );
	m_fmtCtxOut = NULL;
}

int32_t CEsToTs::readData( void* opaque, uint8_t* buf, int32_t size )
{
	CEsToTs* pThis = ( CEsToTs* )opaque;
	if ( !pThis )
	{
		return -1;
	}

	int32_t ret = 0;

	do
	{
		ret = pThis->m_streamData.popdata( buf, size ); // 没有读够,则返回-1,以中止循环.
		//std::cout << "ffmpeg read data size [ " << size << " ret [ " << ret << " ]" << std::endl;

		if ( ret < 0 )
		{
			return -1;
		}
	} while ( (ret == 0)  && (!pThis->m_quit));

	return ret;
}

int32_t CEsToTs::writeData( void *opaque, uint8_t *buf, int32_t buf_size )
{
	CEsToTs* pThis = ( CEsToTs* )opaque;
	if ( !pThis )
	{
		return -1;
	}

	if ( !pThis->m_serHandler )
	{
		return -1;
	}

	pThis->m_serHandler->sendTransStream( (char*)buf, buf_size );
	return 0;

}

bool CEsToTs::init()
{

	if ( !m_streamData.init( DATAQUEUE_SIZE ) )
	{
		return false;
	}

	m_quit = false;

	try{
		m_thread.reset( new (std::nothrow) boost::thread( boost::bind( &CEsToTs::streamHandlerThread, this ) ) );
	}
	catch(...)
	{
	}

	if ( !m_thread )
	{
		m_quit = true;
		m_streamData.reset( false );
		return false;
	}

	return true;
}

bool CEsToTs::fini()
{
	m_quit = true;

	if ( m_fmtCtxIn && m_fmtCtxIn->pb )
	{
		m_fmtCtxIn->pb->eof_reached = 1;
	}

	m_streamData.reset( false );

	//printf("CDevSerHandler [%lu] fini join Enter. \n", (uint32_t)this);

	if ( m_thread )
	{
		m_thread->join();
		m_thread.reset();
	}

	//printf("CDevSerHandler [%lu] fini join Leave. \n", (uint32_t)this);

	m_streamData.fini();
	if ( m_streamBuff )
	{
		delete []m_streamBuff;
		m_streamBuff =  NULL;
		m_buildPacketLen = 0;
		m_streamBuffLen = 0;
	}
	return true;
}

bool CEsToTs::inputStream( char *buffer, size_t bufSize )
{
	if ( m_quit )
	{
		return false;
	}

	int ret = m_streamData.pushdata(( uint8_t*)buffer, bufSize);
	return true;
}


bool CEsToTs::ffmpeg_init()
{
#ifndef NDEBUG
    av_log_set_callback(NULL);
#endif
    //avfilter_register_all();
    av_register_all();

    //CPacketQueue::init_flush_pkt();

    if ( av_lockmgr_register( lockmgr ) ) 
    {
        fprintf( stderr, "Could not initialize lock manager!\n");

        return false;
    }
    return true;
}

void CEsToTs::ffmpeg_fini()
{
    av_lockmgr_register( NULL );
}

}