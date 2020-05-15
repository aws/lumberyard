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

#include "LyShine_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/IRendererMock.h>
#include <Mocks/ITextureMock.h>
#include <I3DEngine.h> // needed for SRenderingPassInfo
#include <LyShineModule.h>
#include <Sprite.h>

namespace
{
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::_;

    class LyShineTest
        : public ::testing::Test
    {
    protected:
        LyShineTest()
            : m_application()
            , m_systemEntity(nullptr)
        {
        }

        void SetUp() override
        {
            m_priorEnv = gEnv;

            m_data = AZStd::make_unique<DataMembers>();
            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pRenderer = &m_data->m_renderer;
            gEnv = &m_data->m_stubEnv;

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            appDesc.m_stackRecordLevels = 20;

            AZ::ComponentApplication::StartupParameters appStartup;
            appStartup.m_createStaticModulesCallback =
                [](AZStd::vector<AZ::Module*>& modules)
                {
                    modules.emplace_back(new LyShine::LyShineModule);
                };

            m_systemEntity = m_application.Create(appDesc, appStartup);
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        void TearDown() override
        {
            m_application.Destroy();
            m_data.reset();
            gEnv = m_priorEnv;
        }

        AZ::ComponentApplication m_application;
        AZ::Entity* m_systemEntity;

        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            NiceMock<IRendererMock> m_renderer;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };

    TEST_F(LyShineTest, Sprite_CanAcquireRenderTarget)
    {
        // initialize to create the static sprite cache
        CSprite::Initialize();

        const char* renderTargetName = "$test";
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(_, _)).WillRepeatedly(Return(nullptr));

        CSprite* sprite = CSprite::CreateSprite(renderTargetName);
        ASSERT_NE(sprite, nullptr);

        ITexture* texture = sprite->GetTexture();
        EXPECT_EQ(texture, nullptr);

        ITextureMock* mockTexture = new NiceMock<ITextureMock>;

        // expect the sprite acquires the texture and increments the reference count
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(_, _)).WillOnce(Return(mockTexture));
        EXPECT_CALL(*mockTexture, AddRef()).Times(1);

        texture = sprite->GetTexture();
        EXPECT_EQ(texture, mockTexture);

        // the sprite should attempt to release the texture by calling ReleaseResourceAsync
        EXPECT_CALL(m_data->m_renderer, ReleaseResourceAsync(_)).Times(1);

        delete sprite;
        sprite = nullptr;

        CSprite::Shutdown();
        delete mockTexture;
    }

    TEST_F(LyShineTest, Sprite_CanCreateWithExistingRenderTarget)
    {
        // initialize to create the static sprite cache
        CSprite::Initialize();

        ITextureMock* mockTexture = new NiceMock<ITextureMock>;

        const char* renderTargetName = "$test";
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(_, _)).WillRepeatedly(Return(mockTexture));

        // the sprite should increment the ref count on the texture
        EXPECT_CALL(*mockTexture, AddRef()).Times(1);

        CSprite* sprite = CSprite::CreateSprite(renderTargetName);
        ASSERT_NE(sprite, nullptr);

        ITexture* texture = sprite->GetTexture();
        EXPECT_EQ(texture, mockTexture);

        // the sprite should attempt to release the texture by calling ReleaseResourceAsync
        EXPECT_CALL(m_data->m_renderer, ReleaseResourceAsync(_)).Times(1);

        delete sprite;
        sprite = nullptr;

        CSprite::Shutdown();
        delete mockTexture;
    }
}

AZ_UNIT_TEST_HOOK();