/* 
 * File:   SignalTest_Parametrized.h
 * Author: Barath Kannan
 *
 * Created on 25 August 2016, 8:28 AM
 */

#ifndef SIGNALTEST_PARAMETRIZED_H
#define SIGNALTEST_PARAMETRIZED_H

#include "SignalTest.h"
#include "BSignals/details/BasicTimer.h"
#include <atomic>

struct SignalTestParameters{
    uint32_t nConnections;
    uint32_t nEmissions;
    uint32_t nOperations;
    uint32_t nEmitters;
    bool threadSafe;
    BSignals::ExecutorScheme scheme;
};

class SignalTestParametrized : public SignalTest,
        public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t, uint32_t, bool, BSignals::ExecutorScheme> >{
public:
    virtual void SetUp();
    virtual void TearDown();
protected:
    SignalTestParameters params;
    BSignals::details::BasicTimer bt, bt2;
    std::atomic<uint32_t> counter{0};
    std::atomic<uint32_t> actual{0};
    std::atomic<uint32_t> completedFunctions{0};
    typedef uint64_t sigType;
};

#endif /* SIGNALTEST_PARAMETRIZED_H */

