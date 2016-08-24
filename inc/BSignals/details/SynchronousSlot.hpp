/* 
 * File:   SynchronousSlot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 9:22 PM
 */

#ifndef BSIGNALS_SYNCHRONOUSSLOT_HPP
#define BSIGNALS_SYNCHRONOUSSLOT_HPP

#include "BSignals/details/Slot.hpp"

namespace BSignals{ namespace details{

template <typename... Args>
class SynchronousSlot : public Slot<Args...>{
public:
    SynchronousSlot(std::function<void(Args...)> f) : Slot<Args...>(f){}
    
    void execute(const Args& ... args){
        this->slotFunction(args...);
    }
    
    ExecutorScheme getScheme() const{
        return ExecutorScheme::SYNCHRONOUS;
    }
};

}}
#endif /* BSIGNALS_SYNCHRONOUSSLOT_HPP */

