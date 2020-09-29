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
#include "Maestro_precompiled.h"

#if !defined(_RELEASE)

#include <AzTest/AzTest.h>

#include <Mocks/I3DEngineMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ICVarMock.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/ITimeOfDayMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <MaestroSystemComponent.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimValueType.h>

#include "Cinematics/AnimSequence.h"
#include "Cinematics/AnimPostFXNode.h"
#include "Cinematics/CharacterTrack.h"
#include "Cinematics/CharacterTrackAnimator.h"
#include "Cinematics/TimeOfDayNode.h"
#include "Cinematics/TimeOfDayNodeDescription.h"

namespace TimeOfDayNodeTest
{
    const float START_KEY_TIME = 0.0f;
    const float END_KEY_TIME = 1.0f;

    using TODParamID = ITimeOfDay::ETimeOfDayParamID;

    using ::testing::NiceMock;
    using ::testing::Mock;
    using ::testing::_;
    using ::testing::Return;
    using ::testing::Args;
    using ::testing::Pointee;
    using ::testing::ElementsAreArray;


    /**
     * This class is neccesary to adapt testing of C-Style vec3s
     * EXPECT_CALL requires a parameter for number of elements in C-Style arrays
     */
    class ITimeOfDayMock_SetVariableValueGTestAdapter
        : public ITimeOfDayMock
    {
    public:
        MOCK_METHOD3(SetVariableValueVec3,
            void(int nIndex, float fValue[3], int len));

        void SetVariableValue(int nIndex, float fValue[3]) override
        {
            // Linear search of m_controlParams to find index
            // for parameter in m_nodeParams
            auto paramDesc = m_curNodeDesc->m_controlParams;
            int paramIndex = 0;
            for (; paramIndex < paramDesc.size(); ++paramIndex)
            {
                if (nIndex == paramDesc.at(paramIndex)->m_paramId)
                    break;
            }

            AnimValueType valueType = m_curNodeDesc->m_nodeParams.at(paramIndex).valueType;

            if (valueType == AnimValueType::Float)
            {
                ITimeOfDayMock::SetVariableValue(nIndex, fValue);
            }
            else if (valueType == AnimValueType::RGB)
            {
                SetVariableValueVec3(nIndex, fValue, 3);
            }
        }

        // Used to lookup ParamType of value
        CTODNodeDescription* m_curNodeDesc;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    //! Testing sub-class
    class TimeOfDayNode_Test
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            // Sets up CryString Allocator
            m_primitiveAllocators.ActivateAllocators();

            m_priorEnv = gEnv;

            m_data = AZStd::make_unique<DataMembers>();

            // Unneccesary to return to CVar just returns nullptr to exit early
            ON_CALL(m_data->m_console, GetCVar(_))
                .WillByDefault(Return(nullptr));

            ON_CALL(m_data->m_3DEngine, GetTimeOfDay())
                .WillByDefault(Return(&m_data->m_timeOfDay));

            // Always return true info isn't actually used it just needs to think it retrieved it
            ON_CALL(m_data->m_timeOfDay, GetVariableInfo(_, _))
                .WillByDefault(Return(true));

            gEnv = &m_data->m_stubEnv;
            m_data->m_stubEnv.pSystem = &m_data->m_system;
            m_data->m_stubEnv.pConsole = &m_data->m_console;
            m_data->m_stubEnv.p3DEngine = &m_data->m_3DEngine;

            m_movieSystem = new CMovieSystem();
            m_sequence = new CAnimSequence(m_movieSystem, 0);
        }
        void TearDown() override
        {
            delete m_movieSystem;
            delete m_sequence;

            m_data.reset();

            gEnv = m_priorEnv;

            // Clear all containers with CyStrings to deallocate them before the 
            // CryStringAllocator is destroyed. This has to be done because they
            // are static allocations so their destructors are called much later.
            CAnimPostFXNode::ClearControlParams();
            CMovieSystem::ClearEnumToStringMaps();

            m_primitiveAllocators.DeactivateAllocators();
        }

        void TODNodeInterpTest(AnimNodeType nodeType);

        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            NiceMock<SystemMock> m_system;
            NiceMock<ConsoleMock> m_console;
            NiceMock<CVarMock> m_cvarMock;
            NiceMock<I3DEngineMock> m_3DEngine;
            NiceMock<ITimeOfDayMock_SetVariableValueGTestAdapter> m_timeOfDay;
        };

        AZStd::unique_ptr<DataMembers> m_data;

        Maestro::MaestroAllocatorScope m_primitiveAllocators;

        CMovieSystem* m_movieSystem;
        CAnimSequence* m_sequence;

        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };

    //! Uses TODNodeDescription to generate a test for interpolating all of the variables in the node
    void TimeOfDayNode_Test::TODNodeInterpTest(AnimNodeType nodeType)
    {
        NiceMock<ITimeOfDayMock_SetVariableValueGTestAdapter>& todMock = m_data->m_timeOfDay;
        IAnimNode* todNode = m_sequence->CreateNode(nodeType);
        CTODNodeDescription* nodeDesc = CAnimTODNode::GetTODNodeDescription(nodeType);
        todMock.m_curNodeDesc = nodeDesc;

        SAnimContext animContext = SAnimContext();
        // time is the only variable used in interpolation
        animContext.time = 0.0f;
        float startVector[3] = { 0.0f, 0.0f, 0.0f };
        float endVector[3] = { 1.0f, 1.0f, 1.0f };
        AZ::Vector3 startVec3 = AZ::Vector3::CreateFromFloat3(startVector);
        AZ::Vector3 endVec3 = AZ::Vector3::CreateFromFloat3(endVector);
        float startFloat = 0.0f;
        float endFloat = 1.0f;

        todNode->CreateDefaultTracks();
        int paramCount = nodeDesc->m_controlParams.size();

        ASSERT_EQ(todNode->GetTrackCount(), paramCount);

        // Only setup expected calls for todNode initialization and add keys to interpolate between
        for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            IAnimTrack* track = todNode->GetTrackByIndex(paramIndex);
            AnimValueType valueType = track->GetValueType();
            TODParamID paramId = nodeDesc->m_controlParams.at(paramIndex)->m_paramId;

            if (valueType == AnimValueType::Float)
            {
                track->SetValue(START_KEY_TIME, startFloat);
                track->SetValue(END_KEY_TIME, endFloat);

                EXPECT_CALL(m_data->m_timeOfDay,
                    SetVariableValue(static_cast<int>(paramId), Pointee(startFloat)));
            }
            else if (valueType == AnimValueType::RGB)
            {
                track->SetValue(START_KEY_TIME, startVec3);
                track->SetValue(END_KEY_TIME, endVec3);

                EXPECT_CALL(m_data->m_timeOfDay,
                    SetVariableValueVec3(static_cast<int>(paramId), _, _))
                    // C-Style arrays require an element count in gTest
                    .With(Args<1, 2>(ElementsAreArray(startVector, 3)));
            }
        }

        todNode->Activate(true);
        todNode->Animate(animContext);
        // Clear expections to ensure we don't get false positives
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(&todMock));

        // Setup expected calls for todNode initialization
        for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            IAnimTrack* track = todNode->GetTrackByIndex(paramIndex);
            AnimValueType valueType = track->GetValueType();
            TODParamID paramId = nodeDesc->m_controlParams.at(paramIndex)->m_paramId;

            if (valueType == AnimValueType::Float)
            {
                EXPECT_CALL(m_data->m_timeOfDay,
                    SetVariableValue(static_cast<int>(paramId), Pointee(endFloat)));
            }
            else if (valueType == AnimValueType::RGB)
            {
                EXPECT_CALL(m_data->m_timeOfDay,
                    SetVariableValueVec3(static_cast<int>(paramId), _, _))
                    .With(Args<1, 2>(ElementsAreArray(endVector, 3)));
            }
        }

        animContext.time = 1.0f;
        todNode->Animate(animContext);
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(&todMock));
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Tests for each type of node
    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_SunNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_Sun);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_FogNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_Fog);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_VolumetricFogNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_VolumetricFog);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_SkyLightNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_SkyLight);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_NightSkyNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_NightSky);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_NightSkyMultiplierNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_NightSkyMultiplier);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_CloudShadingNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_CloudShading);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_SunRaysEffectNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_SunRaysEffect);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_AdvancedTODNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_AdvancedTOD);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_FiltersNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_Filters);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_DepthOfFieldNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_DepthOfField);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_ShadowsNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_Shadows);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_ObsoleteNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_Obsolete);
    }

    TEST_F(TimeOfDayNode_Test, TimeOfDayNode_Interpolation_HDRNode_FT)
    {
        TODNodeInterpTest(AnimNodeType::TOD_HDR);
    }

}; // namespace TimeOfDayNodeTest

#endif // !defined(_RELEASE)
