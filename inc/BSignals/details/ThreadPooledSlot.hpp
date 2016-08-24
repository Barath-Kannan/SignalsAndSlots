/* 
 * File:   ThreadPooledSlot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 10:20 PM
 */

#ifndef BSIGNALS_THREADPOOLEDSLOT_HPP
#define BSIGNALS_THREADPOOLEDSLOT_HPP

#include "BSignals/details/Slot.hpp"
#include "BSignals/details/WheeledThreadPool.h"

namespace BSignals{ namespace details{

template <typename... Args>
class ThreadPooledSlot : public Slot<Args...>{
public:
    ThreadPooledSlot(std::function<void(Args...)> f, std::function<bool()> validityCheck = nullptr)
    : Slot<Args...>(f), checkIfValid(validityCheck){
        WheeledThreadPool::startup();
    }
    
    void execute(const Args& ... args){
        WheeledThreadPool::run([this, args...](){
            if (!checkIfValid || checkIfValid()){
                this->slotFunction(args...);
            }
        });
    }
    
    ExecutorScheme getScheme() const{
        return ExecutorScheme::THREAD_POOLED;
    }
    
private:
    std::function<bool()> checkIfValid;
};

}}

#endif /* BSIGNALS_THREADPOOLEDSLOT_HPP */

