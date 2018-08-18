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
#include "CryLegacy_precompiled.h"

#include "Helper.h"

#include "ControllerOpt.h"

#include "CharacterInstance.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"

#include "Command_Buffer.h"
#include "Command_Commands.h"

#include "Skeleton.h"
#include "LoaderDBA.h"
#include "CharacterManager.h"

#include <AzTest/AzTest.h>

namespace CommandTest
{
    const int g_bufferCount = Command::TargetBuffer + 3;
    const int32 g_testBoneCrc = 0xDEADBEEF;
    const int32 g_testChildBoneCrc = 0xFEEBDAED;

    /////////////////////////////////////////////////////////////////
    // Global Utility functions
    bool AreQuatsEqual(QuatT& lhs, QuatT& rhs)
    {
        if (!fcmp(lhs.q.v[0], rhs.q.v[0]) || !fcmp(lhs.q.v[1], rhs.q.v[1]) ||
            !fcmp(lhs.q.v[2], rhs.q.v[2]) || !fcmp(lhs.q.w, rhs.q.w))
        {
            return false;
        }

        return true;
    }

    bool AreQuatsEquivalent(QuatT& lhs, QuatT& rhs)
    {
        QuatT negRhs = rhs;
        negRhs.q *= -1.f;
        if (AreQuatsEqual(lhs, rhs) || AreQuatsEqual(lhs, negRhs))
        {
            return true;
        }

        return false;
    }

    /////////////////////////////////////////////////////////////////
    // Animation Set Stub for test
    // always returns a default ModelAnimationHeader with a global ID of 0
    class CAnimationSetStub
        : public CAnimationSet
    {
    public:
        CAnimationSetStub()
            : CAnimationSet(nullptr)

        {
            m_dummyHeader.m_nGlobalAnimId = 0;
            m_dummyHeader.m_nAssetType = CAF_File;
            m_arrAnimations.push_back(m_dummyHeader);
        }

        virtual ~CAnimationSetStub()
        {
        }

        virtual const ModelAnimationHeader* GetModelAnimationHeader(int i) const
        {
            return &m_dummyHeader;
        }

        void SetModelHeaderIsAim()
        {
            m_dummyHeader.m_nAssetType = AIM_File;
        }

    private:
        ModelAnimationHeader m_dummyHeader;
    };

    //////////////////////////////////////////////////////////////////
    // Controller stub for test
    // Always returns identity quat and zero position for result
    class CControllerStub
        : public IController
    {
    public:
        CControllerStub()
        {
            m_nControllerId = g_testBoneCrc;
        }
        virtual ~CControllerStub()
        {
        }

        virtual JointState GetOP(f32 key, Quat& quat, Vec3& pos) const
        {
            quat.SetIdentity();
            pos.zero();
            return (eJS_Position | eJS_Orientation);
        }
        virtual int32 GetO_numKey() const
        {
            return 1;
        }
        virtual int32 GetP_numKey() const
        {
            return 1;
        }

        virtual size_t GetRotationKeysNum() const
        {
            return 1;
        }
        virtual size_t GetPositionKeysNum() const
        {
            return 1;
        }

        // Minimal implementation (not used in tests)
        virtual JointState GetOPS(f32 key, Quat& quat, Vec3& pos, Diag33& scale) const
        {
            return emptyJointState;
        }
        virtual JointState GetO(f32 key, Quat& quat) const
        {
            return emptyJointState;
        }
        virtual JointState GetP(f32 key, Vec3& pos) const
        {
            return emptyJointState;
        }
        virtual JointState GetS(f32 key, Diag33& pos) const
        {
            return emptyJointState;
        }
        virtual size_t SizeOfController() const
        {
            return 0;
        }
        virtual size_t ApproximateSizeOfThis() const
        {
            return 0;
        }
        virtual void GetMemoryUsage(ICrySizer* pSizer) const
        {
        }

    protected:
        static const JointState emptyJointState = 0;
        bool m_isNegative;
    };

    class CDefaultSkeletonStub
        : public CDefaultSkeleton
    {
    public:
        CDefaultSkeletonStub()
            : CDefaultSkeleton(nullptr, 0)
        {
        }

        virtual ~CDefaultSkeletonStub()
        {
        }
    };

    //////////////////////////////////////////////////////////////////
    // Base class for tests of various Animation Commands
    class CommandTestBase
    {
    public:
        CommandTestBase()
        {
        }

        virtual ~CommandTestBase()
        {
        }
    protected:
        void SharedSetUp()
        {
            m_defaultBoneName = "test";
            for (auto i = 0; i < 2; i++)
            {
                m_boneMatrix[i].SetIdentity();
            }

            CreateDefaultSkeleton();
            CreateDefaultCommandState();
            CreateDefaultAnimationAsset();
        }

        void SharedTearDown()
        {
            ClearAnimationManager();
        }

        void ScaleBoneMatrixForBlend(float weight)
        {
            for (auto i = 0; i < 2; i++)
            {
                m_boneMatrix[i].q *= weight;
            }
        }

        void NegateBoneMatrix()
        {
            for (auto i = 0; i < 2; i++)
            {
                m_boneMatrix[i].q *= -1.f;
                m_posOrientWeights[i].x = 1.f;
                m_posOrientWeights[i].y = 1.f;
            }
        }

        void ClearAnimationManager()
        {
            g_AnimationManager.m_arrGlobalCAF.clear();
            g_AnimationManager.m_arrGlobalAIM.clear();
        }

        virtual void CreateDefaultAnimationAsset() = 0;

        void CreateDefaultSkeleton()
        {
            // Create default skeleton with a stub AnimationSet with a single bone
            m_defaultSkeleton.m_poseDefaultData.AllocateData(1);
            for (auto i = 0; i < 2; i++)
            {
                m_defaultSkeleton.m_poseDefaultData.SetJointAbsolute(i, m_boneMatrix[i]);
                m_defaultSkeleton.m_poseDefaultData.SetJointRelative(i, m_boneMatrix[i]);
            }

            m_defaultSkeleton.m_arrModelJoints.resize(2);
            m_defaultSkeleton.m_arrModelJoints[0].m_nJointCRC32 = g_testBoneCrc;
            m_defaultSkeleton.m_arrModelJoints[1].m_nJointCRC32 = g_testChildBoneCrc;
            m_defaultSkeleton.m_pAnimationSet.reset(new CAnimationSetStub()); // Deallocated via SmartPtr owned by Default Skeleton
        }

        void CreateDefaultCommandState()
        {
            // Set command values that will be accessed
            m_commandState.m_jointCount = 2;
            m_commandState.m_pDefaultSkeleton = &m_defaultSkeleton;


            // Setup Command buffers for data transfer
            m_commandBuffers[0] = static_cast<void*>(&m_boneMatrix);
            m_commandBuffers[1] = static_cast<void*>(&m_jointState);
            m_commandBuffers[2] = static_cast<void*>(&m_posOrientWeights);
        }

        CDefaultSkeletonStub m_defaultSkeleton;

        Command::CState m_commandState;
        void* m_commandBuffers[g_bufferCount];

        string m_defaultBoneName;

        // We need two of each of these, since some commands will not act on bone index 0
        QuatT m_boneMatrix[2];
        JointState m_jointState[2];
        Vec2 m_posOrientWeights[2];
    };

#if 0 // Disabled due to: SEH exception with code 0xc0000005 thrown in the test body.
      /////////////////////////////////////////////////////////////////
      // Framework for testing the Commands that require CAFs in Commands_Commands
    class CAFCommandTestFramework
        : public CommandTestBase
        , public ::testing::Test
    {
    public:
        CAFCommandTestFramework()
        {
        }

        virtual ~CAFCommandTestFramework()
        {
        }

    protected:
        virtual void SetUp()
        {
            SharedSetUp();
        }

        virtual void TearDown()
        {
            SharedTearDown();
        }

        // Build a valid default CAF with a duration of 0.033f at a sample rate of 30 FPS
        // It will have one bone whose transform is identity
        // It will be non-additive by default
        // Use SetTestCAFAdditive() and InvertCAFBoneMatrix to change these value for test
        virtual void CreateDefaultAnimationAsset()
        {
            // In case any animations were loaded to start with, clear the array
            ClearAnimationManager();

            // Register default CAF with Animation Manager
            g_AnimationManager.m_arrGlobalCAF.push_back();
            m_cafStub = &g_AnimationManager.m_arrGlobalCAF.back();

            // Set default values for timing/controller count
            m_cafStub->m_fStartSec = 0.f;
            m_cafStub->m_fEndSec = 0.033f;
            m_cafStub->m_nControllers = 2;
            m_cafStub->m_nControllers2 = 2;
            m_cafStub->OnAssetCreated();

            // Create test controller
            m_cafStub->m_arrController.push_back(new CControllerStub()); // Deallocation occurs via SmartPtr
            m_cafStub->m_arrControllerLookupVector.push_back(g_testBoneCrc);
            m_cafStub->m_arrController.push_back(new CControllerStub()); // Deallocation occurs via SmartPtr
            m_cafStub->m_arrControllerLookupVector.push_back(g_testChildBoneCrc);
        }

        void SetTestCAFAdditive()
        {
            m_cafStub->OnAssetAdditive();
        }

        GlobalAnimationHeaderCAF* m_cafStub;
    };

    // Ideally these will be switched to parameterized by value Bool() tests after rebuild of gmock

    TEST_F(CAFCommandTestFramework, SampleAddAnimFullExecute_AdditiveSameSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[0];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[0]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimFullExecute_AdditiveOppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[0];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);
        NegateBoneMatrix();


        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[0]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimFullExecute_SameSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;

        QuatT startingBoneMatrix = m_boneMatrix[0];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[0]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimFullExecute_OppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;

        QuatT startingBoneMatrix = m_boneMatrix[0];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);
        NegateBoneMatrix();


        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[0]));
    }

    TEST_F(CAFCommandTestFramework, SampleReplaceAnimPartExecute_AdditiveSameSignQuat_JointIsEquivalent)
    {
        Command::SampleReplaceAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[1];

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleReplaceAnimPartExecute_AdditiveOppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleReplaceAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[1];
        NegateBoneMatrix();

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleReplaceAnimPartExecute_SameSignQuat_JointIsEquivalent)
    {
        Command::SampleReplaceAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;

        QuatT startingBoneMatrix = m_boneMatrix[1];

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleReplaceAnimPartExecute_OppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleReplaceAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;

        QuatT startingBoneMatrix = m_boneMatrix[1];
        NegateBoneMatrix();

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimPartExecute_AdditiveSameSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[1];

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimPartExecute_AdditiveOppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;
        SetTestCAFAdditive();

        QuatT startingBoneMatrix = m_boneMatrix[1];
        NegateBoneMatrix();

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimPartExecute_SameSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;

        QuatT startingBoneMatrix = m_boneMatrix[1];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(CAFCommandTestFramework, SampleAddAnimPartExecute_OppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleAddAnimPart testCommand;
        testCommand.m_fAnimTime = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;
        testCommand.m_TargetBuffer = Command::TmpBuffer;

        QuatT startingBoneMatrix = m_boneMatrix[1];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);
        NegateBoneMatrix();

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }
#endif

#if 0 // Disabled: SEH exception with code 0xc0000005 thrown in the test fixture's constructor.
      /////////////////////////////////////////////////////////////////
      // Framework for testing the Commands that require CAFs in Commands_Commands
    class AIMCommandTestFramework
        : public CommandTestBase
        , public ::testing::Test
    {
    public:
        AIMCommandTestFramework()
        {
        }

        virtual ~AIMCommandTestFramework()
        {
        }

    protected:
        virtual void SetUp()
        {
            SharedSetUp();
        }

        virtual void TearDown()
        {
            SharedTearDown();
        }

        virtual void CreateDefaultAnimationAsset()
        {
            // Create Default AIM
            g_AnimationManager.m_arrGlobalAIM.push_back();
            m_aimStub = &g_AnimationManager.m_arrGlobalAIM.back();

            // Set default values for timing/controller count
            m_aimStub->m_fStartSec = 0.f;
            m_aimStub->m_fEndSec = 0.033f;
            m_aimStub->m_nControllers = 2;
            m_aimStub->OnAssetCreated();

            // Create test controller
            m_aimStub->m_arrController.push_back(new CControllerStub()); // Deallocation occurs via SmartPtr
            m_aimStub->m_arrController.push_back(new CControllerStub()); // Deallocation occurs via SmartPtr
        }


        GlobalAnimationHeaderAIM* m_aimStub;
    };

    TEST_F(AIMCommandTestFramework, SampleAddPoseFullExecute_SameSignQuat_JointIsEquivalent)
    {
        Command::SampleAddPoseFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;

        QuatT startingBoneMatrix = m_boneMatrix[1];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }

    TEST_F(AIMCommandTestFramework, SampleAddPoseFullExecute_OppositeSignQuat_JointIsEquivalent)
    {
        Command::SampleAddPoseFull testCommand;
        testCommand.m_fETimeNew = 0.f;
        testCommand.m_nEAnimID = 0;
        testCommand.m_fWeight = 0.5f;

        QuatT startingBoneMatrix = m_boneMatrix[1];

        ScaleBoneMatrixForBlend(1.f - testCommand.m_fWeight);
        NegateBoneMatrix();

        testCommand.Execute(m_commandState, m_commandBuffers);
        EXPECT_TRUE(AreQuatsEquivalent(startingBoneMatrix, m_boneMatrix[1]));
    }
#endif // Disabled: SEH exception with code 0xc0000005 thrown in the test fixture's constructor.
};
