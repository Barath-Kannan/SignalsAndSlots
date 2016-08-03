/* 
 * File:   SignalTest.h
 * Author: kannanb
 *
 * Created on May 10, 2016, 2:59 PM
 */

#ifndef BSIGNALS_SIGNALTEST_H
#define BSIGNALS_SIGNALTEST_H

#include <gtest/gtest.h>
#include "BSignals/Signal.hpp"

class SignalTest : public testing::Test{
public:
    virtual void SetUp();
    virtual void TearDown();
    
};

struct SignalTestParameters{
    uint32_t nConnections;
    uint32_t nEmissions;
    uint32_t nOperations;
    bool threadSafe;
    BSignals::ExecutorScheme scheme;
};

class SignalTestParametrized : public SignalTest,
        public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t, bool, BSignals::ExecutorScheme> >{
};

#endif /* BSIGNALS_SIGNALTEST_H */
