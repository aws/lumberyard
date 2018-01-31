// Copyright 2017 FragLab Ltd. All rights reserved.

#include "StdAfx.h"

#include <AzTest/AzTest.h>

class MarshalersTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(MarshalersTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
