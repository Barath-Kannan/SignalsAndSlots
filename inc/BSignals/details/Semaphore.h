/* 
 * File:   Semaphore.h
 * Author: Barath Kannan
 *
 * Created on 30 May 2016, 6:49 PM
 */

#ifndef BSIGNALS_SEMAPHORE_H
#define BSIGNALS_SEMAPHORE_H

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace BSignals{ namespace details{
class Semaphore{
public:
    Semaphore(uint32_t size);
    ~Semaphore();
    
    void acquire();
    void acquireAll();
    void release();
    
private:
    std::mutex semMutex;
    std::condition_variable semCV;
    std::atomic<uint32_t> semCounter;
    const uint32_t initial;
};
}}
#endif /* BSIGNALS_SEMAPHORE_H */

