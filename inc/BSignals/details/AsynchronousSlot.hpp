/* 
 * File:   AsynchronousSlot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 11:06 PM
 */

#ifndef BSIGNALS_ASYNCHRONOUSSLOT_HPP
#define BSIGNALS_ASYNCHRONOUSSLOT_HPP

#include <thread>
#include "BSignals/details/Slot.hpp"
#include "BSignals/details/Semaphore.h"

namespace BSignals{ namespace details{

template <typename... Args>
class AsynchronousSlot : public Slot<Args...>{
public:
    AsynchronousSlot(std::function<void(Args...)> f, std::function<bool()> validityCheck = nullptr)
    : Slot<Args...>(f), checkIfValid(validityCheck){}
    
    ~AsynchronousSlot(){
        sem.acquireAll();
    }
    
    void execute(const Args& ... args){
        sem.acquire();
        std::thread slotThread([this, args...](){
            if (!checkIfValid || checkIfValid()){
                this->slotFunction(args...);
            }
            sem.release();
        });
        slotThread.detach();
    }
    
    ExecutorScheme getScheme() const{
        return ExecutorScheme::ASYNCHRONOUS;
    }
    
private:
    std::function<bool()> checkIfValid;
    Semaphore sem{1024};
};

}}

#endif /* BSIGNALS_ASYNCHRONOUSSLOT_HPP */

