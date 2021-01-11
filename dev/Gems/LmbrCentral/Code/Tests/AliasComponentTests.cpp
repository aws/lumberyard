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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AliasComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>

using namespace AZ;
using namespace LmbrCentral;

namespace UnitTest
{
    class AliasComponentTest
        : public ScopedAllocatorSetupFixture
    {

    public:
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_aliasComponentDescriptor;
        AZ::IO::LocalFileIO m_io;
        AZ::IO::FileIOBase* m_previousIo;

    public:
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_aliasComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(AliasComponent::CreateDescriptor());
            m_aliasComponentDescriptor->Reflect(m_serializeContext.get());

            m_previousIo = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_io);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_previousIo);

            m_aliasComponentDescriptor.reset();
            m_serializeContext.reset();
        }
    };

    void VerifyAlias(const char* alias, AZ::IO::FileIOBase& io, const char* expectedValue)
    {
        const char* value = io.GetAlias(alias);

        ASSERT_NE(value, nullptr);
        ASSERT_GT(strlen(value), 0);

        AZStd::string stringResult = value;
        AZStd::string expectedString = expectedValue;

        ASSERT_TRUE(AZ::StringFunc::Path::Normalize(stringResult));
        ASSERT_TRUE(AZ::StringFunc::Path::Normalize(expectedString));
        ASSERT_STREQ(stringResult.c_str(), expectedString.c_str());
    }

    void VerifyAliases(AZ::IO::FileIOBase& io)
    {
        VerifyAlias("@root@", io, "C:/dev");
        VerifyAlias("@assets@", io, "C:/dev/assets");
        VerifyAlias("@user@", io, "C:/dev/user");
        VerifyAlias("@log@", io, "C:/dev/user/log");
        VerifyAlias("@engroot@", io, "C:/engine");
        VerifyAlias("@devroot@", io, "C:/dev/devroot");
        VerifyAlias("@devassets@", io, "C:/dev/devroot/assets");
    }

    TEST_F(AliasComponentTest, AliasesSetInConfig_AppliedAfterComponentActivate)
    {
        Entity entity;

        entity.CreateComponent<AliasComponent>();

        AliasConfiguration config;
        
        config.m_rootPath = "c:/dev/";
        config.m_assetsPath = "@root@/assets";
        config.m_userPath = "@root@/user";
        config.m_logPath = "@user@/log";
        // Leaving this one out since it will attempt to write a lock-file on disk
        //config.m_cachePath = "@user@/cache";
        config.m_engineRootPath = "c:/engine";
        config.m_devRootPath = "@root@/devroot";
        config.m_devAssetsPath = "@devroot@/assets";

        AliasConfigurationStorage::SetConfig(config);

        entity.Init();
        entity.Activate();

        VerifyAliases(m_io);
    }

    TEST_F(AliasComponentTest, SetPartial_Updates)
    {
        Entity entity;

        entity.CreateComponent<AliasComponent>();

        AliasConfiguration config;

        config.m_rootPath = "c:/dev/";
        config.m_assetsPath = "@root@/assets";
        config.m_userPath = "@root@/user";
        config.m_logPath = "c:/log";

        AliasConfigurationStorage::SetConfig(config);

        // Reset
        config = {};

        config.m_logPath = "@user@/log"; // This one will test overwriting a previous alias
        config.m_engineRootPath = "c:/engine";
        config.m_devRootPath = "@root@/devroot";
        config.m_devAssetsPath = "@devroot@/assets";

        AliasConfigurationStorage::SetConfig(config);

        entity.Init();
        entity.Activate();

        VerifyAliases(m_io);
    }

    TEST_F(AliasComponentTest, SetPartial_Clear)
    {
        Entity entity;

        entity.CreateComponent<AliasComponent>();

        AliasConfiguration config;

        config.m_rootPath = "c:/dev/";
        config.m_assetsPath = "@root@/assets";
        config.m_userPath = "@root@/user";
        config.m_logPath = "@user@/log";

        AliasConfigurationStorage::SetConfig(config);

        // Reset
        config = {};

        config.m_engineRootPath = "c:/engine";
        config.m_devRootPath = "c:/dev/devroot";
        config.m_devAssetsPath = "c:/dev/devroot/assets";

        AliasConfigurationStorage::SetConfig(config, true);

        entity.Init();
        entity.Activate();

        VerifyAlias("@engroot@", m_io, "C:/engine");
        VerifyAlias("@devroot@", m_io, "C:/dev/devroot");
        VerifyAlias("@devassets@", m_io, "C:/dev/devroot/assets");
    }
}
