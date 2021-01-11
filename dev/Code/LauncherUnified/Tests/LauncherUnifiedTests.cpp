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

#include "Launcher_precompiled.h"
#include <IGameStartup.h>
#include <AzTest/AzTest.h>
#include <AzCore/Component/EntityBus.h>
#include "../Launcher.h"
#include "../../CryEngine/CryCommon/IEditorGame.h"

class UnifiedLauncherTestFixture
    : public ::testing::Test
{
public:
    static bool m_appStartCalled;

protected:
    void SetUp() override {}
    void TearDown() override {}
};

bool UnifiedLauncherTestFixture::m_appStartCalled = false;

TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoAddArgument_NoCommandLineFunctions_Success)
{
    LumberyardLauncher::PlatformMainInfo test;

    EXPECT_STREQ(test.m_commandLine, "");
    EXPECT_EQ(test.m_argC, 0);
}

TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoAddArgument_ValidParams_Success)
{
    LumberyardLauncher::PlatformMainInfo test;

    const char* testArguments[] = { "-arg", "value1", "-arg2", "value2", "-argspace", "value one"};
    for (const char* testArgument : testArguments)
    {
        test.AddArgument(testArgument);
    }

    EXPECT_STREQ(test.m_commandLine, "-arg value1 -arg2 value2 -argspace \"value one\"");
    EXPECT_EQ(test.m_argC, 6);
    EXPECT_STREQ(test.m_argV[0], "-arg");
    EXPECT_STREQ(test.m_argV[1], "value1");
    EXPECT_STREQ(test.m_argV[2], "-arg2");
    EXPECT_STREQ(test.m_argV[3], "value2");
    EXPECT_STREQ(test.m_argV[4], "-argspace");
    EXPECT_STREQ(test.m_argV[5], "value one");
}


TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoCopyCommandLineArgCArgV_ValidParams_Success)
{
    LumberyardLauncher::PlatformMainInfo test;

    const char* constTestArguments[] = { "-arg", "value1", "-arg2", "value2", "-argspace", "value one" };
    char** testArguments = const_cast<char**>(constTestArguments);
    int testArgumentCount = AZ_ARRAY_SIZE(constTestArguments);

    test.CopyCommandLine(testArgumentCount,testArguments);

    EXPECT_STREQ(test.m_commandLine, "-arg value1 -arg2 value2 -argspace \"value one\"");
    EXPECT_EQ(test.m_argC, 6);
    EXPECT_STREQ(test.m_argV[0], "-arg");
    EXPECT_STREQ(test.m_argV[1], "value1");
    EXPECT_STREQ(test.m_argV[2], "-arg2");
    EXPECT_STREQ(test.m_argV[3], "value2");
    EXPECT_STREQ(test.m_argV[4], "-argspace");
    EXPECT_STREQ(test.m_argV[5], "value one");
}

struct EntitySystemListener;

struct TestAccessEntity : public AZ::Entity
{
    friend struct EntitySystemListener;
};

struct GameMock : IGame
{
    bool Init(IGameFramework* pFramework) override;
    void Shutdown() override;
    void PlayerIdSet(EntityId playerId) override;
    void LoadActionMaps(const char* filename) override;
    EntityId GetClientActorId() const override;
    IGameFramework* GetIGameFramework() override;
};

bool GameMock::Init(IGameFramework* /*pFramework*/)
{
    return true;
}

void GameMock::Shutdown()
{
}

void GameMock::PlayerIdSet(EntityId /*playerId*/)
{
}

void GameMock::LoadActionMaps(const char* /*filename*/)
{
}

EntityId GameMock::GetClientActorId() const
{
    return {};
}

IGameFramework* GameMock::GetIGameFramework()
{
    return {};
}

struct GameStartupMock : IGameStartup
{
    IGameRef Init(SSystemInitParams& /*startupParams*/) override
    {
        return IGameRef(&m_gamePtr);
    }

    void Shutdown() override
    {
    }

    GameMock m_game;
    IGame* m_gamePtr = &m_game;
};

struct EditorGameMock : EditorGameRequestBus::Handler
{
    ~EditorGameMock()
    {
        BusDisconnect();
    }

    IGameStartup* CreateGameStartup() override
    {
        return &m_startupMock;
    }

    IEditorGame* CreateEditorGame() override
    {
        return nullptr;
    }

    GameStartupMock m_startupMock;
};

struct EntitySystemListener : AZ::EntitySystemBus::Handler
{
    EntitySystemListener()
    {
        BusConnect();
    }

    ~EntitySystemListener()
    {
        BusDisconnect();
    }

    void OnEntityActivated(const AZ::EntityId& id) override
    {
        if(id == AZ::SystemEntityId)
        {
            TestAccessEntity::DeactivateComponent(*m_component);
            m_editorGameMock.BusConnect();
        }
    }

    void OnEntityInitialized(const AZ::EntityId& id) override
    {
        const auto LegacyGameInterfaceSystemComponentGuid = AZ::Uuid("{6F11A7F2-4091-46EA-A77A-D1E918D8EDFE}");

        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);

        if (entity)
        {
            // LegacyGameInterfaceSystemComponent
            auto component = entity->FindComponent(LegacyGameInterfaceSystemComponentGuid);

            if (component)
            {
                m_component = component;
            }
        }
    }

    AZ::Entity* m_systemEntity;
    AZ::Component* m_component;
    EditorGameMock m_editorGameMock;
};

TEST_F(UnifiedLauncherTestFixture, StarterLauncher_AliasesAreConfigured)
{
    UnifiedLauncherTestFixture::m_appStartCalled = false;

    EntitySystemListener entitySystemListener;
    
    LumberyardLauncher::PlatformMainInfo mainInfo;
    mainInfo.AddArgument("test");
    mainInfo.m_onPostAppStart = []()
    {
        auto io = AZ::IO::FileIOBase::GetInstance();

        ASSERT_TRUE(io);

        const char* rootAlias = io->GetAlias("@root@");
        const char* assetsAlias = io->GetAlias("@assets@");
        const char* userAlias = io->GetAlias("@user@");
        const char* logAlias = io->GetAlias("@log@");
        const char* cacheAlias = io->GetAlias("@cache@");

        ASSERT_NE(rootAlias, nullptr);
        ASSERT_NE(assetsAlias, nullptr);
        ASSERT_NE(userAlias, nullptr);
        ASSERT_NE(logAlias, nullptr);
        ASSERT_NE(cacheAlias, nullptr);

        ASSERT_GT(strlen(rootAlias), 0);
        ASSERT_GT(strlen(assetsAlias), 0);
        ASSERT_GT(strlen(userAlias), 0);
        ASSERT_GT(strlen(logAlias), 0);
        ASSERT_GT(strlen(cacheAlias), 0);

        UnifiedLauncherTestFixture::m_appStartCalled = true;
    };
    LumberyardLauncher::Run(mainInfo);

    ASSERT_TRUE(UnifiedLauncherTestFixture::m_appStartCalled);
}
