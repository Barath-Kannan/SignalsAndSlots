/* 
 * File:   SafeQueueTest.h
 * Author: kannanb
 *
 * Created on April 21, 2016, 11:37 AM
 */

#ifndef SAFEQUEUETEST_H
#define SAFEQUEUETEST_H

#include <gtest/gtest.h>

class SafeQueueTest : public testing::Test{
public:
    virtual void SetUp();
    virtual void TearDown();
    
};

class SafeQueueTest_Usage_Parametrized : public SafeQueueTest, public testing::WithParamInterface<std::pair<uint32_t, uint32_t> >{
};

#endif /* SAFEQUEUETEST_H */

