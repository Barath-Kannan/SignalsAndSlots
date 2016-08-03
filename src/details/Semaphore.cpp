#include "BSignals/details/Semaphore.h"

using BSignals::details::Semaphore;

Semaphore::Semaphore(uint32_t size)
: semCounter(size), initial(size) {}

Semaphore::~Semaphore() {}

void Semaphore::acquire() {
    std::unique_lock<std::mutex> lock(semMutex);
    while (semCounter <= 0){
        semCV.wait(lock);
    }
    --semCounter;
}

void Semaphore::release() {
    ++semCounter;
    semCV.notify_one();
}

void Semaphore::acquireAll() {
    for (uint32_t i=0; i<initial; i++){
        acquire();
    }
}