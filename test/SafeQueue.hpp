/* 
 * File:   SafeQueue.hpp
 * Author: Barath Kannan
 *
 * Created on April 21, 2016, 10:28 PM
 */

#ifndef BSIGNALS_SAFEQUEUE_HPP
#define BSIGNALS_SAFEQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>

namespace BSignals{ namespace details{
template <class T>
class SafeQueue{
public:
    
    //Default constructed object is returned when forced stop is required
    //Otherwise, constructor parameters can be provided
    template<typename ...Args>
    SafeQueue(Args&& ...args) : shutdownObject(args...), terminateFlag (false), q(), m(), c() {}
    
    ~SafeQueue(){
        stop();
    }
    
    void enqueue(const T &t){
        std::unique_lock<std::mutex> lock(m);
        q.push(t);
        lock.unlock();
        c.notify_one();
    }
    
    T dequeue(void){
        std::unique_lock<std::mutex> lock(m);
        if (terminateFlag) return shutdownObject;
        while (q.empty()){
            c.wait(lock);
            if (terminateFlag) return shutdownObject;
        }
        T val = q.front();
        q.pop();
        return val;
    }
    
    std::vector<T> dequeueAll(){
        std::unique_lock<std::mutex> lock(m);
        while (q.empty()){
            c.wait(lock);
            if (terminateFlag){
                std::vector<T> ret(1);
                ret[0] = shutdownObject;
                return ret;
            }
        }
        std::vector<T> ret(q.size());
        for (uint32_t i=0; !q.empty(); ++i){
            ret[i] = q.front();
            q.pop();
        }
        return ret;
    }

    std::pair<T, bool> waitForDequeue(std::chrono::duration<double> timeout){
        std::pair<T, bool> ret;
        ret.second = false;
        std::unique_lock<std::mutex> lock(m);
        if (terminateFlag){
            return ret;
        }
        bool success = (c.wait_for(lock, timeout) == std::cv_status::no_timeout);
        if (!success) return ret;
        ret.first = q.front();
        q.pop();
        ret.second = true;
        return ret;
    }
    
    std::pair<T, bool> nonBlockingDequeue(void){
        std::unique_lock<std::mutex> lock(m);
        std::pair<T, bool> ret;
        if (!q.empty()){
            ret.first = q.front();
            q.pop();
            ret.second = true;
        }
        else{
            ret.second = false;
        }
        
        return ret;
    }
    
    void clear(){
        std::lock_guard<std::mutex> lock(m);
        while (!q.empty()){
            q.pop();
        }
    }
    
    void wakeWaiters(){
        c.notify_all();
    }
    
    void wait(){
        std::unique_lock<std::mutex> lock(m);
        if (terminateFlag) return;
        if (q.empty()){
            c.wait(lock);
        }
    }
    
    bool wait(std::chrono::duration<double> timeout){
        auto start = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::mutex> lock(m);
        if (!q.empty()) return true;
        if (terminateFlag) return false;
        auto now = std::chrono::high_resolution_clock::now();
        return (c.wait_for(lock, timeout - (now - start)) == std::cv_status::no_timeout);
    }
    
    void stop(){
        terminateFlag = true;
        wakeWaiters();
    }
    
    bool isStopped(){
        return terminateFlag;
    }
    
    unsigned int size(){
        std::lock_guard<std::mutex> lock(m);
        return q.size();
    }
    
    bool isEmpty(){
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }
    
private:
    T shutdownObject;
    bool terminateFlag;
    std::queue<T> q;
    std::mutex m;
    std::condition_variable c;
};
}}
#endif /* BSIGNALS_SAFEQUEUE_HPP */

