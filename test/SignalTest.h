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

#endif /* BSIGNALS_SIGNALTEST_H */
