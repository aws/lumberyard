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
#include "StdAfx.h"
#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/I3DEngineMock.h>
#include <Mocks/IRendererMock.h>


#include "PostEffectGroup.h"

namespace UnitTest
{
    class PostEffectGroupTest
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            using ::testing::_;
            
            // capture prior state.
            m_priorEnv = gEnv;

            UnitTest::AllocatorsTestFixture::SetUp();

            m_data = AZStd::make_unique<DataMembers>();

            // override state with our needed test mocks
            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pRenderer = &m_data->m_renderer;
            m_data->m_stubEnv.p3DEngine = &m_data->m_3DEngine;
            m_data->m_stubEnv.pTimer = &m_data->m_timer;
            
            gEnv = &(m_data->m_stubEnv);
            
            ON_CALL(m_data->m_3DEngine, GetPostEffectGroups())
                .WillByDefault(Invoke(this, &PostEffectGroupTest::GetGroupManager));
            ON_CALL(m_data->m_3DEngine, SetPostEffectParam(_,_,_))
                .WillByDefault(Invoke(this, &PostEffectGroupTest::SetPostEffectParam));
            ON_CALL(m_data->m_timer, GetFrameTime(_))
                .WillByDefault(::testing::Return(0.016f));

            m_postEffectGroups = std::make_unique<PostEffectGroupManager>();
        }

        void TearDown() override
        {
            m_postEffectGroups.reset();
            
            // Remove the other mocked systems
            m_data.reset();

            UnitTest::AllocatorsTestFixture::TearDown();

            // restore the global state that we modified during the test.
            gEnv = m_priorEnv;
        }

    protected:
        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            ::testing::NiceMock<I3DEngineMock> m_3DEngine;
            ::testing::NiceMock<IRendererMock> m_renderer;
            ::testing::NiceMock<TimerMock> m_timer;
        };
        
        IPostEffectGroupManager* GetGroupManager() const
        {
            return m_postEffectGroups.get();
        }
        
        void SetPostEffectParam(const char* pParam, float fValue, bool bForceValue = false)
        {
            AZStd::string testParamStr("TestParamA");

            if (testParamStr.compare(pParam) == 0)
            {
                m_testParamAValue = fValue;
            }
        }
        
        void SimulateRenderFrame()
        {
            m_postEffectGroups->BlendWithParameterCache();
            m_postEffectGroups->SyncMainWithRender();
        }

        AZStd::unique_ptr<DataMembers> m_data;

        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        std::unique_ptr<PostEffectGroupManager> m_postEffectGroups;
        
        float m_testParamAValue = 0.0f;
    };


    TEST_F(PostEffectGroupTest, PostEffectGroup_Initialization_FT)
    {
        IPostEffectGroupManager* groupManager = gEnv->p3DEngine->GetPostEffectGroups();
        ASSERT_NE(groupManager, nullptr);
        
        // Should be initialized with a group "Base"
        IPostEffectGroup* postEffectBaseGroup = groupManager->GetGroup("Base");
        EXPECT_TRUE(postEffectBaseGroup != nullptr);
    }
    
    TEST_F(PostEffectGroupTest, PostEffectGroup_BlendTest_FT)
    {
        IPostEffectGroupManager* groupManager = gEnv->p3DEngine->GetPostEffectGroups();
        ASSERT_NE(groupManager, nullptr);
        EXPECT_EQ(m_testParamAValue, 0.0f);
        
        IPostEffectGroup* postEffectBaseGroup = groupManager->GetGroup("Base");
        ASSERT_NE(postEffectBaseGroup, nullptr);
        
        constexpr float firstParamAValue = 1.0f;
        constexpr float secondParamAValue = 5.0f;
        
        // Blend
        postEffectBaseGroup->SetParam("TestParamA", firstParamAValue);
        SimulateRenderFrame();
        EXPECT_NEAR(m_testParamAValue, firstParamAValue, 0.01f);
        
        postEffectBaseGroup->SetParam("TestParamA", secondParamAValue);
        SimulateRenderFrame();
        EXPECT_NEAR(m_testParamAValue, secondParamAValue, 0.01f);
    }
}

