
#include "Footsteps_precompiled.h"

#include <AzTest/AzTest.h>

class FootstepsTest
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

TEST_F(FootstepsTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
