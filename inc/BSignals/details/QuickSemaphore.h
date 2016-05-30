
/* 
 * File:   QuickSemaphore.h
 * Author: Barath Kannan
 *
 * Created on 30 May 2016, 6:49 PM
 */

#ifndef QUICKSEMAPHORE_H
#define QUICKSEMAPHORE_H

#include <atomic>
#include <mutex>
#include <condition_variable>

class QuickSemaphore{
public:
    QuickSemaphore(uint32_t size);
    ~QuickSemaphore();
    
    void acquire();
    void release();
private:
    std::mutex semMutex;
    std::condition_variable semCV;
    std::atomic<uint32_t> semCounter;
};

#endif /* QUICKSEMAPHORE_H */

