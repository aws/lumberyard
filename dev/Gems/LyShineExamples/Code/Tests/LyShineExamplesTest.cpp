
#include "LyShineExamples_precompiled.h"

#include <AzTest/AzTest.h>

class LyShineExamplesTest
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

TEST_F(LyShineExamplesTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
