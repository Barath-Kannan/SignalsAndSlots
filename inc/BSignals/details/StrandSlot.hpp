/* 
 * File:   StrandSlot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 8:17 PM
 */

#ifndef BSIGNALS_STRANDSLOT_HPP
#define BSIGNALS_STRANDSLOT_HPP

#include <thread>
#include <iostream>
#include "BSignals/details/Slot.hpp"
#include "BSignals/details/WheeledThreadPool.h"
#include "BSignals/details/MPSCQueue.hpp"

namespace BSignals{ namespace details{

template <typename... Args>
class StrandSlot : public Slot<Args...>{
public:
    StrandSlot(std::function<void(Args...)> f) : Slot<Args...>(f){
        strandThread = std::thread(&StrandSlot<Args...>::queueListener, this);
    }
    
    ~StrandSlot(){
        stop = true;
        strandQueue.enqueue(std::tuple<Args...>{});
        strandThread.join();
    }
    
    void execute(const Args& ... args){
        strandQueue.enqueue(std::tuple<Args...>(args...));
    }
    
    ExecutorScheme getScheme() const{
        return ExecutorScheme::STRAND;
    }
    
private:
    void queueListener(){
        std::tuple<Args...> tuple{};
        auto maxWait = WheeledThreadPool::getMaxWait();
        std::chrono::duration<double> waitTime = std::chrono::nanoseconds(1);
        while (!stop){
            if (strandQueue.dequeue(tuple)){
                if (!stop) this->callFuncWithTuple(tuple, std::index_sequence_for<Args...>());
                waitTime = std::chrono::nanoseconds(1);
            }
            else{
                std::this_thread::sleep_for(waitTime);
                waitTime*=2;
            }
            if (waitTime > maxWait){
                strandQueue.blockingDequeue(tuple);
                if (!stop) this->callFuncWithTuple(tuple, std::index_sequence_for<Args...>());
                waitTime = std::chrono::nanoseconds(1);
            }
        }
    }

    MPSCQueue<std::tuple<Args...>> strandQueue;
    std::thread strandThread;
    bool stop{false};
};

}}

#endif /* BSIGNALS_STRANDSLOT_HPP */

