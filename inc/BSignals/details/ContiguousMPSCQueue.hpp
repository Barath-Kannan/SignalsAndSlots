/* 
 * File:   ContiguousMPSCQueue.hpp
 * Author: Barath Kannan
 *
 * Created on 19 June 2016, 7:52 PM
 */

#ifndef CONTIGUOUSMPSCQUEUE_HPP
#define CONTIGUOUSMPSCQUEUE_HPP

#include <atomic>

namespace BSignals{ namespace details{

template<typename T, uint32_t N=100>
class ContiguousMPSCQueue{
public:
    bool enqueue(const T& item){
        const uint32_t currentWrite = wIndex.load(std::memory_order_acquire);
        uint32_t nextNode = (currentWrite+1 == elements.size()) ? 0 : currentWrite+1;            
        if (nextNode != rIndex.load(std::memory_order_acq_rel)){
            elements[currentWrite] = item;
            wIndex.store(nextNode, std::memory_order::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool dequeue(T & output){
        const uint32_t currentRead = rIndex.load(std::memory_order_relaxed);
        if (currentRead == wIndex.load(std::memory_order_acquire)) return false;
        uint32_t nextRecord = ((currentRead + 1) == elements.size()) ? 0 : currentRead+1;
        output = std::move(elements[currentRead]);
        rIndex.store(nextRecord, std::memory_order_release);
        return true;
    }
    
private:
    std::array<T, N> elements;
    std::atomic<uint32_t> rIndex{0};
    std::atomic<uint32_t> wIndex{0};
};
}}


#endif /* CONTIGUOUSMPSCQUEUE_HPP */

