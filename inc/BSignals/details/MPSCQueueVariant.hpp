
/* 
 * File:   MPSCQueueVariant.hpp
 * Author: Barath Kannan
 *
 * Created on 17 June 2016, 7:25 PM
 */

#ifndef MPSCQUEUEVARIANT_HPP
#define MPSCQUEUEVARIANT_HPP

#include <atomic>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <assert.h>
#include <list>
#include <vector>
#include <iostream>
using std::cout;
using std::endl;

namespace BSignals{ namespace details{

template<typename T>
class MPSCQueueVariant{
public:
    
    MPSCQueueVariant(){
        availableBuffers.emplace_back();
    }
    
    void enqueue(const T& input){
        for (;;){
            _mutex.lock_shared();
            //locking if there is nothing to write to
            if (availableBuffers.empty()){
                _mutex.unlock_shared();
                _mutex.lock();
                while (availableBuffers.empty()){
                    availableBuffers.emplace_back();
                }
                _mutex.unlock();
                _mutex.lock_shared();
            }
            if (availableBuffers.front().put(input)){
                _cv.notify_one();
                _mutex.unlock_shared();
                return;
            }
            _mutex.unlock_shared();
            _mutex.lock();
            if (!availableBuffers.empty() && !(availableBuffers.front().put(input))){
                fullBuffers.splice(fullBuffers.end(), availableBuffers, availableBuffers.begin());
            }
            _mutex.unlock();
        }
    }

    bool dequeue(T& output){
        for (;;){
            _mutex.lock_shared();
            if (!fullBuffers.empty()){
                if (fullBuffers.front().get(output)){
                    _mutex.unlock_shared();
                    return true;
                }
                
                _mutex.unlock_shared();
                _mutex.lock();
                if (!fullBuffers.empty()){
                    if (fullBuffers.front().get(output)){
                        _mutex.unlock();
                        return true;
                    }
                    availableBuffers.splice(availableBuffers.end(), fullBuffers, fullBuffers.begin());
                }
                _mutex.unlock();
            }
            else{
                bool ret = (!availableBuffers.empty() && availableBuffers.front().get(output));
                _mutex.unlock_shared();
                return ret;
            }
        }
    }
    
    void blockingDequeue(T& output){
        std::unique_lock<std::shared_timed_mutex> lock(_mutex);
        for (;;){
            if (!fullBuffers.empty() && fullBuffers.front().get(output)) return;
            if (!availableBuffers.empty() && availableBuffers.front().get(output)) return;
            _cv.wait(lock);
        }
    }
    
private:

    struct bufferNodeVariant{
        T data;
        uint32_t state{0};
    };

    template <uint32_t N=1000>
    class bufferNodeVector{
    public:
        bool put(const T& item){
            uint32_t index = currentWriteElement.fetch_add(1);
            if (index >= N) return false;
            elements[index].data = item;
            elements[index].state = 1;
            return true;
        }
        
        //assume only 1 reader
        bool get(T& item){
            if (currentReadElement >= N || elements[currentReadElement-1].state == 0){
                return false;
            }
            if (elements[currentReadElement-1].state == 1){
                item = elements[currentReadElement-1].data;
                elements[currentReadElement-1].state = 2;
                return true;
            }
            uint32_t index = currentReadElement++;
            if (index >= N || elements[index].state == 0) return false;
            item = elements[index].data;
            elements[index].state = 2;
            return true;
        }
        
    private:
        mutable std::atomic<uint32_t> currentWriteElement{0};
        mutable uint32_t currentReadElement{1};
        mutable std::array<bufferNodeVariant, N> elements;
        
    };

    std::list<bufferNodeVector<>> availableBuffers;
    std::list<bufferNodeVector<>> fullBuffers;
    
    std::shared_timed_mutex _mutex;
    std::shared_timed_mutex _blockMutex;
    std::condition_variable_any _cv;
    
};
}}

#endif /* MPSCQUEUEVARIANT_HPP */

