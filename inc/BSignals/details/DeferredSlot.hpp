/* 
 * File:   DeferredSlot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 11:37 PM
 */

#ifndef BSIGNALS_DEFERREDSLOT_HPP
#define BSIGNALS_DEFERREDSLOT_HPP

#include "BSignals/details/Slot.hpp"
#include "BSignals/details/MPSCQueue.hpp"

namespace BSignals{ namespace details{

template <typename... Args>
class DeferredSlot : public Slot<Args...>{
public:
    DeferredSlot(std::function<void(Args...)> f, std::shared_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>> dq, uint32_t slotId)
    : Slot<Args...>(f), deferredQueue(dq), id(slotId){}
    
    void execute(const Args& ... args){
        deferredQueue->enqueue({[this, args...](){this->slotFunction(args...);}, id});
    }

    ExecutorScheme getScheme() const{
        return ExecutorScheme::DEFERRED_SYNCHRONOUS;
    }
    
private:
    std::shared_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>> deferredQueue{nullptr};
    const uint32_t id;
};
}}

#endif /* BSIGNALS_DEFERREDSLOT_HPP */

