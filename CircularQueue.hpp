#ifndef CIRCULARQUEUE_HPP_
#define CIRCULARQUEUE_HPP_

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
//#include "../include/VtsProxyItf.hpp"

namespace htc
{

enum EQueuestate
{
    EQUEUESTATE_EMPTY,
    EQUEUESTATE_NORMAL,
    EQUEUESTATE_FULL,
};

class CCircularQueue
{
public:
    CCircularQueue();
    ~CCircularQueue();
public:
    bool init( uint32_t len );
    void fini();

    void reset( bool stopwork );

    int32_t pushdata( uint8_t *buf, uint32_t len );         // 返回0：推送数据失败
    int32_t popdata( uint8_t *buf, uint32_t len );          // 还是按len来读,但是不等len满,直接读完    

    uint32_t getsize();

private:
    uint8_t*                        m_buf;
    uint32_t                        m_bufcapacity;
    uint32_t                        m_write_ptr;
    uint32_t                        m_read_ptr;
    bool                            m_iswork;
    EQueuestate                     m_queuestate;//0空 1正常 2满

    boost::mutex                    m_mutex;
    boost::condition_variable_any   m_cond;
};

}

#endif//CIRCULARQUEUE_HPP_
