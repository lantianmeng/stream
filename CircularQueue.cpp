#include "CircularQueue.hpp"

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <math.h>

extern "C"
{
#include <libavutil/mem.h>
}

namespace htc
{

CCircularQueue::CCircularQueue()
: m_buf ( NULL )
, m_bufcapacity ( 0 )
, m_write_ptr ( 0 )
, m_read_ptr ( 0 )
, m_iswork ( false )
, m_queuestate ( EQUEUESTATE_EMPTY )
, m_mutex()
, m_cond()
{
}

CCircularQueue::~CCircularQueue()
{
}

bool CCircularQueue::init( uint32_t len )
{
    if ( len == 0 )
    {
        return false;
    }

    if ( !m_buf )
    {
        m_buf = (uint8_t*)av_mallocz( sizeof(uint8_t) * len );
    }
    else if ( len != m_bufcapacity )
    {
        av_free( m_buf );
        m_buf = NULL;

        // 重新分配
        m_buf = (uint8_t*)av_mallocz( sizeof(uint8_t) * len );
    }

    if ( !m_buf )
    {
        return false;
    }

    {
        boost::mutex::scoped_lock lock( m_mutex );
        m_iswork      = true;
        m_queuestate  = EQUEUESTATE_EMPTY;
        m_read_ptr    = 0;
        m_write_ptr   = 0;
        m_bufcapacity = len;
    }

    return true;
}

void CCircularQueue::fini()
{
    {
        boost::mutex::scoped_lock lock( m_mutex );

        m_iswork     = false;
        m_queuestate = EQUEUESTATE_EMPTY;
        m_read_ptr    = 0;
        m_write_ptr   = 0;
        m_bufcapacity = 0;
    }

    m_cond.notify_all();


    if ( m_buf )
    {
        av_free( m_buf );

        m_buf = NULL;
    }
}

int32_t CCircularQueue::pushdata( uint8_t *buf, uint32_t len )
{
    if ( buf == NULL || len == 0 )
    {
        return 0;
    }

    {
        boost::mutex::scoped_lock lock( m_mutex );

        if ( !m_iswork )
        {
            return 0;
        }

        if ( m_queuestate == EQUEUESTATE_FULL )
        {
            //std::cout << "CCircularQueue pushdata overflow0..." << std::endl;
            return 0;
        }

        uint8_t* writePos = m_buf + m_write_ptr;

        if ( ( m_write_ptr + len ) > m_bufcapacity )
        {
            // 超出尾部,且空间不足.
            if ( m_write_ptr + len - m_bufcapacity > m_read_ptr )
            {
                //std::cout << "CCircularQueue pushdata overflow1..." << std::endl;
                return 0;
            }

            // 超出尾部,且空间足够.
            memcpy( writePos, buf, sizeof(uint8_t) * ( m_bufcapacity - m_write_ptr ) );
            memcpy( m_buf, buf + ( m_bufcapacity - m_write_ptr ), sizeof(uint8_t) * ( len - ( m_bufcapacity - m_write_ptr ) ) );
        }
        else
        {
            // 未超出尾部,但空间不足
            if ( m_read_ptr > m_write_ptr && m_write_ptr + len > m_read_ptr )
            {
                //std::cout << "CCircularQueue pushdata overflow2..." << std::endl;
                return 0;
            }

            memcpy( writePos, buf, sizeof(uint8_t) * len );
        }
        m_write_ptr = ( m_write_ptr + len ) % m_bufcapacity;

        m_write_ptr == m_read_ptr ? ( m_queuestate = EQUEUESTATE_FULL ) : ( m_queuestate = EQUEUESTATE_NORMAL );
    }

    m_cond.notify_all();

    return len;
}

int32_t CCircularQueue::popdata( uint8_t *buf, uint32_t len )
{
    if ( buf == NULL || len == 0 )
    {
        return -1;
    }

    boost::mutex::scoped_lock lock( m_mutex );

    if ( !m_iswork )
    {
        return -1;
    }

    if ( m_queuestate == EQUEUESTATE_EMPTY )
    {
        if ( !m_cond.timed_wait( lock, boost::posix_time::milliseconds( 100 ) ) ) 
        {
            // wait.
        }
        if ( !m_iswork )
        {
            return -1;
        }

        return 0;
    }

    uint32_t existingLen = len;
    uint8_t* src         = m_buf + m_read_ptr;
    bool     wrap        = false;
    uint32_t pos         = m_write_ptr;

    if ( pos <= m_read_ptr )
    {
        pos += m_bufcapacity;

        if ( m_read_ptr + len > m_bufcapacity )
        {
            wrap = true;
        }
    }

    if ( m_read_ptr + len > pos )
    {
        existingLen = pos - m_read_ptr;
    }
    else
    {
        existingLen = len;
    }

    if ( wrap )
    {
        wrap = false;

        if ( m_read_ptr + existingLen > m_bufcapacity )
        {
            wrap = true;
        }
    }

    if ( wrap )
    {
        memcpy( buf, src, sizeof(uint8_t) * ( m_bufcapacity - m_read_ptr ) );
        memcpy( buf + ( m_bufcapacity - m_read_ptr ), m_buf, sizeof(uint8_t) * ( existingLen - ( m_bufcapacity - m_read_ptr ) ) );
    }
    else
    {
        memcpy( buf, src, sizeof(uint8_t) * existingLen );
    }
    m_read_ptr = ( m_read_ptr + existingLen ) % m_bufcapacity;

    m_write_ptr == m_read_ptr ? ( m_queuestate = EQUEUESTATE_EMPTY ) : ( m_queuestate = EQUEUESTATE_NORMAL );

    return existingLen;
}

void CCircularQueue::reset( bool stopwork )
{
    {
        boost::mutex::scoped_lock lock( m_mutex );
        if ( stopwork )
        {
            m_iswork = false;
        }

        m_queuestate = EQUEUESTATE_EMPTY;        
        m_read_ptr   = 0;
        m_write_ptr  = 0;
    }

    m_cond.notify_all();
}

uint32_t CCircularQueue::getsize()
{
    uint32_t size = 0xFFFFFFFFu;

    boost::mutex::scoped_lock lock( m_mutex );
    
    switch ( m_queuestate )
    {
    case EQUEUESTATE_EMPTY:
        {
            size = 0;
        }
        break;
    case EQUEUESTATE_NORMAL:
        {
            if ( m_read_ptr < m_write_ptr )
            {
                size = m_write_ptr - m_read_ptr;
            }
            else
            {
                size =  m_bufcapacity - m_read_ptr + m_write_ptr;
            }
        }
        break;
    case EQUEUESTATE_FULL:
        {
            size = m_bufcapacity;
        }
        break;
    default:
        break;
    }

    return size;
}

} // namespace egw
