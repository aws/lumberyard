
#include "StdAfx.h"

#include <AzTest/AzTest.h>

class CloudTest
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

TEST_F(CloudTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
