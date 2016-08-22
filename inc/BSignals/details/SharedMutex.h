/* 
 * File:   SharedMutex.h
 * Author: Barath Kannan
 *
 * Created on 23 August 2016, 6:42 AM
 */

#ifndef SHAREDMUTEX_H
#define SHAREDMUTEX_H

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace BSignals{ namespace details{

class SharedMutex{
public:
    SharedMutex() = default;
    SharedMutex(const SharedMutex& that) = delete;
    void operator=(const SharedMutex&) = delete;
    
    void lock();
    void lock_shared();
    void unlock();
    void unlock_shared();
    
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    uint32_t m_readers{0};
    uint32_t  m_waitingWriters{0};
    bool m_writer{false};
};
    
}}

#endif /* SHAREDMUTEX_H */

