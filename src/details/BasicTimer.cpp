#include "BSignals/details/BasicTimer.h"
#include <chrono>

BasicTimer::BasicTimer()
    : running(false) {}

double BasicTimer::getElapsedSeconds() {
    return std::chrono::duration<double>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedMilliseconds() {
    return std::chrono::duration<double, std::milli>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedMicroseconds() {
    return std::chrono::duration<double, std::micro>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedNanoseconds() {
    return std::chrono::duration<double, std::nano>(getElapsedDuration()).count();
}

std::chrono::duration<double> BasicTimer::getElapsedDuration() {
    if (!running){
        std::chrono::duration<double> elapsed = end-begin;
        return elapsed;
    }
    auto n = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = n-begin;
    return elapsed;
}

bool BasicTimer::isRunning() {
    return running;
}

bool BasicTimer::start() {
    if (running) return false;
    running = true;
    begin = std::chrono::high_resolution_clock::now();
    return true;
}

bool BasicTimer::stop() {
    if (!running) return false;
    end = std::chrono::high_resolution_clock::now();
    running = false;
    return true;
}
