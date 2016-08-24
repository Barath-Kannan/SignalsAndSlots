/* 
 * File:   Slot.hpp
 * Author: Barath Kannan
 *
 * Created on 24 August 2016, 8:07 PM
 */

#ifndef BSIGNALS_SLOT_HPP
#define BSIGNALS_SLOT_HPP

#include <atomic>
#include <functional>
#include <utility>
#include "BSignals/ExecutorScheme.h"

namespace BSignals{ namespace details{
    
template <typename... Args>
class Slot{
public:
    Slot(std::function<void(Args...)> f) : slotFunction(f){};
    virtual ~Slot(){};
    virtual void execute(const Args& ... args) = 0;
    virtual ExecutorScheme getScheme() const = 0;
    
    bool isAlive() const{
        return (alive.load(std::memory_order_acquire));
    }
    
    void markForDeath(){
        alive.store(false, std::memory_order_release);
    }
    
protected:    
    template<std::size_t... Is>
    void callFuncWithTuple(const std::tuple<Args...>& tuple, std::index_sequence<Is...>) {
        slotFunction(std::get<Is>(tuple)...);
    }
    
    std::function<void(Args...)> slotFunction;
    std::atomic<bool> alive{true};
};

}}

#endif /* BSIGNALS_SLOT_HPP */

