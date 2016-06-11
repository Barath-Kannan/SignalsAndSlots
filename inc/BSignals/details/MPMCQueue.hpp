/* 
 * File:   MPMCQueue.hpp
 * Author: Barath Kannan
 * Multi-producer Multi-consumer Fixed Size Queue
 * Stored internally as a vector for fast enqueue/dequeue
 * Minimum size of 250 elements
 * Created on 10 May 2016, 11:33 PM
 */

#ifndef MPMCQUEUE_HPP
#define MPMCQUEUE_HPP

#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace BSignals{ namespace details{
template <class T>
class MPMCQueue{
public: 
     
    template<typename ...Args>
    MPMCQueue(uint32_t size = 100000, Args&& ...args)
    : shutdownObject(args...), terminateFlag (false), length(size > 250 ? size : 250), data(length), available(length), locks(length), cvs(length)
    {
        writeIndex = 0;
        readIndex = 0;
        for (uint32_t i=0; i<length; i++){
            available[i] = 0;
        }
    }
    
    ~MPMCQueue(){
        stop();
    }
    
    void enqueue(T t){
        uint32_t indx = fetchWrapIncrement(writeIndex);
        std::unique_lock<std::mutex> lock(locks[indx]);
        while (available[indx]){
            cvs[indx].wait(lock);
            if (terminateFlag)
                return;
        }
        data[indx] = t;
        available[indx] = 1;
        cvs[indx].notify_one();
    }
    
    T dequeue(){
        uint32_t indx = fetchWrapIncrement(readIndex);
        ScopedRead sr(this, indx);
        return sr.get();
    }
    
    void clear(){
        writeIndex=0;
        readIndex=0;
    }
    
    bool isEmpty(){
        for (uint32_t i=0; i<length; i++){
            if (available[i]){
                return true;
            }
        }
        return false;
    }
    
    uint32_t size(){
        uint32_t count = 0;
        for (uint32_t i=0; i<length; i++){
            if (available[i])
                count++;
        }
        return count;
    }
    
    void stop(){
        terminateFlag = true;
        for (uint32_t i=0; i<length; i++){
            std::lock_guard<std::mutex> lock(locks[i]);
            cvs[i].notify_all();
        }
    }
    
    bool isStopped(){
        return terminateFlag;
    }
       
    uint32_t getReadIndex(){
        return readIndex;
    }
    uint32_t getWriteIndex(){
        return writeIndex;
    }
    
private:
    uint32_t fetchWrapIncrement(std::atomic<uint32_t> &shared){
        uint32_t oldValue = shared.load();
        uint32_t newValue;
        do {
            newValue = (oldValue+1)%length;
        } while (!shared.compare_exchange_strong(oldValue, newValue));
        return oldValue;
    }
    
    class ScopedRead{
        public:
            ScopedRead(MPMCQueue *mpmcq, uint32_t index)
            : lock(mpmcq->locks[index]) {
                q = mpmcq;
                indx = index;
        }
            
            ~ScopedRead(){
                q->available[indx] = 0;
                (q->cvs[indx]).notify_one();
            }
            
            T &get(){
                if (q->terminateFlag)
                    return q->shutdownObject;
                while (!q->available[indx]){
                    q->cvs[indx].wait(lock);
                    if (q->terminateFlag)
                        return q->shutdownObject;
                }
                return q->data[indx];
            }
            
        private:
            uint32_t indx;
            MPMCQueue *q;
            std::unique_lock<std::mutex> lock;
    };
    T shutdownObject;
    bool terminateFlag;
    uint32_t length;
    std::vector<T> data;
    std::vector<uint32_t> available;
    std::atomic<uint32_t> writeIndex;
    std::atomic<uint32_t> readIndex;
    std::vector<std::mutex> locks;
    std::vector<std::condition_variable> cvs;
};
}}
#endif /* MPMCQUEUE_HPP */
