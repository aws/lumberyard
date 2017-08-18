
#include "StdAfx.h"

#include <AzTest/AzTest.h>

class HMDFrameworkTest
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

TEST_F(HMDFrameworkTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
