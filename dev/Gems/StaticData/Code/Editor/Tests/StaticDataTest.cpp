#include <AzTest/AzTest.h>

class StaticDataEditorTest
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

TEST_F(StaticDataEditorTest, Sanityest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
