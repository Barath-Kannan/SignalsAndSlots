/* 
 * File:   Wheel.hpp
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 12:50 AM
 */

#ifndef BSIGNALS_WHEEL_HPP
#define BSIGNALS_WHEEL_HPP

#include <atomic>

namespace BSignals{ namespace details{

//T must be default constructable
template <class T, uint32_t N>
class Wheel{
public:
    
    T& getSpoke() noexcept{
        return wheelymajig[fetchWrapIncrement()];
    }
    
    T& getSpokeRandom() noexcept{
        return wheelymajig[std::rand()%N];
    }
    
    T& getSpoke(uint32_t index) noexcept{
        return wheelymajig[index];
    }
    
    uint32_t getIndex() noexcept{
        return fetchWrapIncrement();
    }
    
    const uint32_t size() noexcept{
        return N;
    }

private:
    inline uint32_t fetchWrapIncrement() noexcept{
        uint32_t oldValue = currentElement.load();
        while (!currentElement.compare_exchange_weak(oldValue, (oldValue+1 == N) ? 0 : oldValue+1));
        return oldValue;
    }
    char padding0[64];
    std::atomic<uint32_t> currentElement{0};
    char padding1[64];
    std::array<T, N> wheelymajig;
    char padding2[64];
};    
}}

#endif /* BSIGNALS_WHEEL_HPP */

