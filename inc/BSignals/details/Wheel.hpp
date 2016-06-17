/* 
 * File:   Wheel.hpp
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 12:50 AM
 */

#ifndef WHEEL_HPP
#define WHEEL_HPP

#include <atomic>

namespace BSignals{ namespace details{

//T must be default constructable
template <class T, uint32_t N>
class Wheel{
public:
    
    T& getSpoke(){
        return wheelymajig[fetchWrapIncrement()];
    }
    
    T& getSpoke(uint32_t index){
        return wheelymajig[index];
    }
    
    const uint32_t size(){
        return N;
    }

private:
    uint32_t fetchWrapIncrement(){
        uint32_t oldValue = currentElement.load();
        uint32_t newValue;
        do {
            newValue = (oldValue+1)%N;
        } while (!currentElement.compare_exchange_weak(oldValue, newValue));
        return oldValue;
    }
    std::atomic<uint32_t> currentElement{0};
    std::array<T, N> wheelymajig;
};    
}}

#endif /* WHEEL_HPP */

