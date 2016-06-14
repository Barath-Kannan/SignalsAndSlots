/* 
 * File:   BasicTimer.h
 * Author: Barath Kannan
 *
 * Created on April 21, 2016, 12:09 PM
 */

#ifndef BASICTIMER_H
#define BASICTIMER_H

#include <chrono>

namespace BSignals{ namespace details{
class BasicTimer{
public:
    BasicTimer();
    bool start();
    bool stop();
    
    bool isRunning();
    double getElapsedSeconds();
    double getElapsedMilliseconds();
    double getElapsedMicroseconds();
    double getElapsedNanoseconds();
    
    std::chrono::duration<double> getElapsedDuration();
    
protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin, end;
    bool running;
};
}}
#endif /* BASICTIMER_H */

