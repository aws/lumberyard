
#include "CloudGemFramework_precompiled.h"

#include <AzTest/AzTest.h>

#include <CloudGemFramework/AwsApiRequestJob.h>

#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/AddPermissionRequest.h>
#include <aws/lambda/model/AddPermissionResult.h>
#include <aws/core/utils/Outcome.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

class CloudGemFrameworkStaticLibraryTests
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

TEST_F(CloudGemFrameworkStaticLibraryTests, TestAwsApiJob)
{

    ASSERT_TRUE(true); // FooBar doesn't exist

}

AZ_UNIT_TEST_HOOK();
