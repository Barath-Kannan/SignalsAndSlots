#include "BSignals/details/QuickSemaphore.h"

QuickSemaphore::QuickSemaphore(uint32_t size)
: semCounter(size) {}

QuickSemaphore::~QuickSemaphore() {}

void QuickSemaphore::acquire() {
    uint32_t oldValue = semCounter.load();
    uint32_t newValue;
    do {
        if (oldValue <= 0){
            std::unique_lock<std::mutex> lock(semMutex);
            while (semCounter < 0){
                semCV.wait(lock);
            }
        }
        newValue = oldValue-1;
    } while (!semCounter.compare_exchange_weak(oldValue, newValue));
}


void QuickSemaphore::release() {
    std::unique_lock<std::mutex> lock(semMutex);
    semCounter++;
    semCV.notify_one();
}
