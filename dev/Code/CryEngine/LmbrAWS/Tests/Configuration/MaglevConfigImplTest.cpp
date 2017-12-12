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
#include <StdAfx.h>

#if !defined(_RELEASE)

#include <AzTest/AzTest.h>
#include <Configuration/MaglevConfigImpl.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <AzCore/std/smart_ptr/make_shared.h>

using namespace LmbrAWS;

static string GetTemporaryFilePath()
{
#ifdef _WIN32
    char s_tempName[L_tmpnam + 1];

    /*
    Prior to VS 2014, tmpnam/tmpnam_s generated root level files ("\filename") which were not appropriate for our usage, so for the windows version, we prepended a '.' to make it a
    tempfile in the current directory.  Starting with VS2014, the behavior of tmpnam/tmpnam_s was changed to be a full, valid filepath based on the
    current user ("C:\Users\username\AppData\Local\Temp\...").
    See the tmpnam section in http://blogs.msdn.com/b/vcblog/archive/2014/06/18/crt-features-fixes-and-breaking-changes-in-visual-studio-14-ctp1.aspx
    for more details.
    */
#if _MSC_VER >= 1900
    tmpnam_s(s_tempName, L_tmpnam);
#else
    s_tempName[0] = '.';
    tmpnam_s(s_tempName + 1, L_tmpnam);
#endif // _MSC_VER

    return string(s_tempName);
#else
    return string(std::tmpnam(nullptr));
    ;
#endif /*_WIN32*/
}

class MaglevConfigPersistenceLayerCryPakImplCustomPath
    : public MaglevConfigPersistenceLayerCryPakImpl
{
public:
    string GetImplementationFilePath()
    {
        return MaglevConfigPersistenceLayerCryPakImpl::GetConfigFilePath();
    }

    void SetFilePath(const string& filePath)
    {
        m_filePath = filePath;
    }
protected:

    string GetConfigFilePath() const override
    {
        return m_filePath;
        ;
    }
private:
    string m_filePath;
};

TEST(LmbrAWS_MaglevConfigPersistenceLayerCryPakImplTest, TestGetConfigPath)
{
    MaglevConfigPersistenceLayerCryPakImplCustomPath persistenceLayer;
    ICryPak* cryPak = gEnv->pCryPak;

    ASSERT_TRUE(cryPak != nullptr);
    ASSERT_STREQ("maglevConfig.cfg", persistenceLayer.GetImplementationFilePath().c_str());
}

// NOTE: integration test; requires actual implementation of ICryPak
INTEG_TEST(LmbrAWS_MaglevConfigPersistenceLayerCryPakImplTest, TestFilePersistence)
{
    MaglevConfigPersistenceLayerCryPakImplCustomPath persistenceLayer;
    string filePath = GetTemporaryFilePath();
    persistenceLayer.SetFilePath(filePath);

    Aws::Utils::Json::JsonValue jsonValue = persistenceLayer.LoadJsonDoc();
    ASSERT_EQ(0u, jsonValue.GetAllObjects().size());

    jsonValue.WithString("TestValue", "TestValue");
    persistenceLayer.PersistJsonDoc(jsonValue);

    jsonValue = persistenceLayer.LoadJsonDoc();
    persistenceLayer.Purge();
    ASSERT_EQ(1u, jsonValue.GetAllObjects().size());
    ASSERT_STREQ("TestValue", jsonValue.GetString("TestValue").c_str());
}

class MaglevConfigPersistenceLayerMock
    : public MaglevConfigPersistenceLayer
{
public:
    MaglevConfigPersistenceLayerMock()
        : m_loadCalledCount(0)
        , m_persistCalledCount(0) {}
    Aws::Utils::Json::JsonValue LoadJsonDoc() const override { m_loadCalledCount++; return m_currentValue; }
    void PersistJsonDoc(const Aws::Utils::Json::JsonValue& value) override { m_currentValue = value; m_persistCalledCount++; }
    void Purge() override {}

    const Aws::Utils::Json::JsonValue& GetLastTouchedJsonValue() { return m_currentValue; }
    unsigned GetLoadCalledCount() { return m_loadCalledCount; }
    unsigned GetPersistCalledCount() { return m_persistCalledCount; }

private:
    Aws::Utils::Json::JsonValue m_currentValue;
    mutable unsigned m_loadCalledCount;
    unsigned m_persistCalledCount;
};

TEST(LmbrAWS_MaglevConfigImplTest, TestObjectParsingAndPersistence)
{
    auto persistenceLayer = AZStd::make_shared<MaglevConfigPersistenceLayerMock>();
    MaglevConfigImpl maglevConfigImpl(persistenceLayer);
    maglevConfigImpl.LoadConfig();
    ASSERT_EQ(1u, persistenceLayer->GetLoadCalledCount());
    ASSERT_EQ(0u, persistenceLayer->GetPersistCalledCount());

    ASSERT_EQ("", maglevConfigImpl.GetAnonymousIdentityPoolId());
    ASSERT_EQ("", maglevConfigImpl.GetAuthenticatedIdentityPoolId());
    ASSERT_EQ("", maglevConfigImpl.GetAwsAccountId());
    ASSERT_EQ(Aws::Region::US_EAST_1, maglevConfigImpl.GetDefaultRegion());
    ASSERT_EQ(0u, maglevConfigImpl.GetAuthProvidersToSupport().size());

    static const char* ANON_POOL_ID = "anonymousPoolId";
    static const char* AUTH_POOL_ID = "authenticatedPoolId";
    static const char* AMZN_PROVIDER = "www.amazon.com";
    static const char* FB_PROVIDER = "graph.facebook.com";
    static const char* ACT_ID = "565477434";

    maglevConfigImpl.SetAnonymousIdentityPoolId(ANON_POOL_ID);
    maglevConfigImpl.SetAuthenticatedIdentityPoolId(AUTH_POOL_ID);
    maglevConfigImpl.SetAuthProvidersToSupport({ AMZN_PROVIDER, FB_PROVIDER });
    maglevConfigImpl.SetAwsAccountId(ACT_ID);
    maglevConfigImpl.SetDefaultRegion(Aws::Region::EU_CENTRAL_1);
    maglevConfigImpl.SaveConfig();

    ASSERT_EQ(2u, persistenceLayer->GetLoadCalledCount());
    ASSERT_EQ(1u, persistenceLayer->GetPersistCalledCount());

    auto jsonValue = persistenceLayer->GetLastTouchedJsonValue();
    ASSERT_TRUE(jsonValue.ValueExists("AnonymousIdentityPoolId"));
    ASSERT_EQ(ANON_POOL_ID, jsonValue.GetString("AnonymousIdentityPoolId"));
    ASSERT_TRUE(jsonValue.ValueExists("AuthenticatedIdentityPoolId"));
    ASSERT_EQ(AUTH_POOL_ID, jsonValue.GetString("AuthenticatedIdentityPoolId"));
    ASSERT_TRUE(jsonValue.ValueExists("AwsAccountId"));
    ASSERT_EQ(ACT_ID, jsonValue.GetString("AwsAccountId"));
    ASSERT_TRUE(jsonValue.ValueExists("DefaultRegion"));
    ASSERT_EQ("eu-central-1", jsonValue.GetString("DefaultRegion"));
    ASSERT_TRUE(jsonValue.ValueExists("AuthProvidersToSupport"));
    ASSERT_EQ(2u, jsonValue.GetArray("AuthProvidersToSupport").GetLength());
    ASSERT_EQ(AMZN_PROVIDER, jsonValue.GetArray("AuthProvidersToSupport")[0].AsString());
    ASSERT_EQ(FB_PROVIDER, jsonValue.GetArray("AuthProvidersToSupport")[1].AsString());

    maglevConfigImpl.LoadConfig();
    ASSERT_EQ(ANON_POOL_ID, maglevConfigImpl.GetAnonymousIdentityPoolId());
    ASSERT_EQ(AUTH_POOL_ID, maglevConfigImpl.GetAuthenticatedIdentityPoolId());
    ASSERT_EQ(ACT_ID, maglevConfigImpl.GetAwsAccountId());
    ASSERT_EQ(Aws::Region::EU_CENTRAL_1, maglevConfigImpl.GetDefaultRegion());
    Aws::Vector<Aws::String> providersList = { AMZN_PROVIDER, FB_PROVIDER };
    ASSERT_EQ(providersList, maglevConfigImpl.GetAuthProvidersToSupport());
    ASSERT_EQ(3u, persistenceLayer->GetLoadCalledCount());
    ASSERT_EQ(1u, persistenceLayer->GetPersistCalledCount());
}

#endif /* !defined(_RELEASE)*/
