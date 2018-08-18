#include "StdAfx.h"
#include <gtest/gtest.h>

// minimal export required for AzTestScanner
extern "C" __declspec(dllexport) int AzRunUnitTests()
{
    int argc = 0;
    wchar_t* argv[1] = { L"" };

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(MetricsPluginSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}
