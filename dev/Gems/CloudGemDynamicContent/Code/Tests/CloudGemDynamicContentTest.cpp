/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "CloudGemDynamicContent_precompiled.h"

#include <DynamicContentTransferManager.h>
#include <PresignedURL/PresignedURLBus.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/time.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzFramework/UnitTest/FrameworkTestTypes.h>

#include <AZTestShared/Utils/Utils.h>

#include <Mocks/ISystemMock.h>
#include <Mocks/ICryPakMock.h>

// the Dynamic Content Assert Absorber can be used to absorb asserts, errors, messages and warnings during unit tests.
class DynamicContentAssertAbsorber
    : public AZ::Test::AssertAbsorber
{
public:
    bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);

        ++m_assertCount;
        return true;
    }

    bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(window);
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);

        ++m_errorCount;
        return true;
    }

    bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(window);
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);

        ++m_warningCount;
        return true;
    }

    bool OnPrintf(const char* window, const char* fileName) override
    {
        AZ_UNUSED(window);
        AZ_UNUSED(fileName);

        return true;
    }

    void clear()
    {
        m_assertCount = 0;
        m_warningCount = 0;
        m_errorCount = 0;
    }

    int m_assertCount = 0;
    int m_errorCount = 0;
    int m_warningCount = 0;
};

class MockDynamicContentTransferManager
    : public CloudCanvas::DynamicContent::DynamicContentTransferManager
    , public CloudCanvas::PresignedURLRequestBus::Handler
{
public:
    MockDynamicContentTransferManager()
    {
        CloudCanvas::PresignedURLRequestBus::Handler::BusConnect();
    }

    ~MockDynamicContentTransferManager()
    {
        CloudCanvas::PresignedURLRequestBus::Handler::BusDisconnect();
    }

    bool UpdateFileStatusList(const AZStd::vector<AZStd::string>& requestList, bool autoDownload = false) override
    {
        using namespace CloudCanvas::DynamicContent;

        for (const auto& thisFile : requestList)
        {
            AZStd::shared_ptr<DynamicContentFileInfo> contentFileInfoPtr =
                AZStd::make_shared<DynamicContentFileInfo>(thisFile, "test/" + thisFile);
            contentFileInfoPtr->SetRequestURL("https://s3.amazonaws.com/dynamic-content-test/" + thisFile);
            contentFileInfoPtr->SetUrlCreationTimestamp(AZStd::GetTimeUTCMilliSecond());
            SetFileInfo(contentFileInfoPtr);

            if (autoDownload && !Download(thisFile.c_str(), false))
            {
                return false;
            }
        }

        return true;
    }

    bool Download(const AZStd::string& fileName, bool forceDownload) { return RequestDownload(fileName, forceDownload); }

    void RequestDownloadSignedURL(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) override {};

    AZ::Job* RequestDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) override { return nullptr; };

    AZStd::unordered_map<AZStd::string, AZStd::string> GetQueryParameters(const AZStd::string& signedURL)
    {
        using namespace CloudCanvas::DynamicContent;

        AZStd::unordered_map<AZStd::string, AZStd::string> keyValuePairs;
        keyValuePairs[presignedUrlLifeTimeKey] = AZStd::to_string(GetPresignedUrlLifeTime());

        return keyValuePairs;
    }

    void SetPresignedUrlLifeTime(AZ::u64 presignedUrlLifeTime)
    {
        m_presignedUrlLifeTime = presignedUrlLifeTime;
    }

    AZ::u64 GetPresignedUrlLifeTime() const
    {
        return m_presignedUrlLifeTime;
    }

private:
    AZ::u64 m_presignedUrlLifeTime;
};

class CloudGemDynamicContentTest
    : public UnitTest::ScopedAllocatorsFileIOFixture
{
protected:
    void SetUp() override
    {
        // Setup Mocks on a stub environment
        m_priorEnv = gEnv;
        memset(&m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_stubEnv.pSystem = &m_system;
        m_stubEnv.pCryPak = &m_cryPak;

        // DynamicContentFileInfo uses GetISystem()->GetGlobalEnvironment() so mock it 
        ON_CALL(m_system, GetGlobalEnvironment())
            .WillByDefault(::testing::Return(&m_stubEnv));

        gEnv = &m_stubEnv;

        m_transferManager = aznew MockDynamicContentTransferManager();

        m_assertAbsorber = new DynamicContentAssertAbsorber();
        m_assertAbsorber->clear();

        AZ::IO::FileIOBase::GetInstance()->SetAlias("@user@", UnitTest::GetTestFolderPath().c_str());
    }

    void TearDown() override
    {
        m_assertAbsorber->clear();
        delete m_assertAbsorber;
        m_assertAbsorber = nullptr;

        m_transferManager->ClearAllContent();
        delete m_transferManager;
        m_transferManager = nullptr;

        gEnv = m_priorEnv;
    }

    MockDynamicContentTransferManager* m_transferManager = nullptr;
    DynamicContentAssertAbsorber* m_assertAbsorber = nullptr;

private:
    ::testing::NiceMock<SystemMock> m_system;
    ::testing::NiceMock<CryPakMock> m_cryPak;

    SSystemGlobalEnvironment m_stubEnv;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;
};

TEST_F(CloudGemDynamicContentTest, DownloadWithValidPresignedUrl_PassValidation_DownloadSuccessfully)
{
    m_transferManager->SetPresignedUrlLifeTime(100);
    m_transferManager->UpdateFileStatus("test.shared.pak", false);

    ASSERT_FALSE(m_transferManager->RequestUrlExpired("test.shared.pak"));
    ASSERT_TRUE(m_transferManager->Download("test.shared.pak", false));

    ASSERT_EQ(m_assertAbsorber->m_assertCount, 0);
    ASSERT_EQ(m_assertAbsorber->m_warningCount, 0);
    ASSERT_EQ(m_assertAbsorber->m_errorCount, 0);
}

TEST_F(CloudGemDynamicContentTest, DownloadWithExpiredPresignedUrl_RetryDownload_DownloadSuccessfully)
{
    m_transferManager->SetPresignedUrlLifeTime(1);
    m_transferManager->UpdateFileStatus("test.shared.pak", false);

    AZStd::this_thread::sleep_for(AZStd::chrono::seconds(2));
    ASSERT_TRUE(m_transferManager->RequestUrlExpired("test.shared.pak"));
    ASSERT_TRUE(m_transferManager->Download("test.shared.pak", false));

    ASSERT_EQ(m_assertAbsorber->m_assertCount, 0);
    ASSERT_EQ(m_assertAbsorber->m_warningCount, 1);
    ASSERT_EQ(m_assertAbsorber->m_errorCount, 0);
}

TEST_F(CloudGemDynamicContentTest, DownloadWithExpiredPresignedUrl_ReachMaximumRetryTimes_FailToDownload)
{
    using namespace CloudCanvas::DynamicContent;

    m_transferManager->SetPresignedUrlLifeTime(0);
    m_transferManager->UpdateFileStatus("test.shared.pak", false);

    ASSERT_TRUE(m_transferManager->RequestUrlExpired("test.shared.pak"));
    ASSERT_FALSE(m_transferManager->Download("test.shared.pak", false));

    ASSERT_EQ(m_assertAbsorber->m_assertCount, 0);
    ASSERT_EQ(m_assertAbsorber->m_warningCount, fileDownloadRetryMax);
    ASSERT_EQ(m_assertAbsorber->m_errorCount, 1);
}

AZ_UNIT_TEST_HOOK();
