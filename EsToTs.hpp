
#ifndef ESTOTS_HPP_
#define ESTOTS_HPP_


#define CONFIG_AVFILTER 0

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <math.h>

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

#include "CircularQueue.hpp"
#include "RequestStream.hpp"

namespace htc
{
const static int32_t STREAM_INDEX_INVAIL = -1;              // 无效的流索引号

interface ITranStream
{
public:
	virtual ~ITranStream() {}

public:
	virtual bool init() = 0;
	virtual bool fini() = 0;

	virtual bool inputStream( char *buffer, size_t bufSize ) = 0;
};

class CEsToTs : public ITranStream
{
public:
	CEsToTs( Ssp::IStreamData* serHandler );
	virtual ~CEsToTs();

	bool init();
	bool fini();

	bool inputStream( char *buffer, size_t bufSize );

    static bool ffmpeg_init();
    static void ffmpeg_fini();

private:
	void streamHandlerThread();
	bool transform();
	bool initInput();
	void finiInput();
	bool initOutput();    
	void finiOutput(); 

public:
	static int32_t writeData( void *opaque, uint8_t *buf, int32_t buf_size );
	static int32_t readData( void* opaque, uint8_t* buf, int32_t size );

private:
	Ssp::IStreamData*					  m_serHandler;
	bool                                   m_quit;
	CCircularQueue                         m_streamData;
	AVFormatContext*                       m_fmtCtxIn;
	uint32_t                               m_inputVideoIndex;
	AVFormatContext*                       m_fmtCtxOut;  
	uint32_t                               m_outputVideoIndex;
	boost::shared_ptr<boost::thread>       m_thread;

	uint8_t * m_streamBuff;
	uint32_t m_buildPacketLen;
	uint32_t  m_streamBuffLen;

};

}
#endif
