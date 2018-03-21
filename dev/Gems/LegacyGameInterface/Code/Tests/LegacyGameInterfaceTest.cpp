
#include "LegacyGameInterface_precompiled.h"

#include <AzTest/AzTest.h>

class LegacyGameInterfaceTest
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

TEST_F(LegacyGameInterfaceTest, ExampleTest)
{
    // This module only contains implementation of an interface defined in the engine.
    // Activating these modules would require spinning up the whole engine, making this an integration test and not a unit test.
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
