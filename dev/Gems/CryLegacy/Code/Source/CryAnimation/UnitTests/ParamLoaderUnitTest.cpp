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

#include "AnimationBase.h"
#include "CharacterManager.h"
#include "ParamLoader.h"
#include "TypeInfo_impl.h"
#include <AzTest/AzTest.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Summary: This is the unit test suite for serializing chrparams files using the IChrParamsParser interface.
//          Its base behavior is creating a malformed chrparams, serializing it out, then verifying
//          A) All valid data has been preserved
//          B) The ordering of the valid data has been preserved
//          C) All invalid data has been discarded
/////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace PL_UT_HELPER
{
    CharacterManager* g_dummyCharacterManager;

    string jointList[] =
    {
        "Joint1",
        "Joint2",
        "Joint3",
        "Joint4",
        "Joint5",
        "Joint6",
        "Joint7",
        "Joint8",
        "Joint9",
        "Joint10",
    };
    uint jointCount = ARRAY_COUNT(jointList);

    string test1Path = "Animations\\UnitTest\\test1.chrparams";
    string test2Path = "Animations\\UnitTest\\test2.chrparams";
    string subPath = "Animations\\UnitTest\\sub.chrparams";

    SAnimNode animListValid[] =
    {
        SAnimNode("$AnimEventDatabase", "Animation\\UnitTest\\path\\aed.animevents", "", e_chrParamsAnimationNodeAnimEventsDB),
        SAnimNode("$FaceLib", "Animation\\UnitTest\\path\\facelib.fxl", "", e_chrParamsAnimationNodeFaceLib),
        SAnimNode("$Include", subPath, "", e_chrParamsAnimationNodeInclude),
    };
    uint validRootCount = ARRAY_COUNT(animListValid);

    SAnimNode subAnimListValid[] =
    {
        SAnimNode("#filepath", "Animation\\UnitTest\\path\\animpath\\", "", e_chrParamsAnimationNodeFilepath),
        SAnimNode("anim1", "anim1.caf", "", e_chrParamsAnimationNodeSingleAsset),
        SAnimNode("*_wildcard", "subpath\\*.caf", "", e_chrParamsAnimationNodeWildcardAsset),
        SAnimNode("*", "subpath\\*.dba", "", e_chrParamsAnimationNodeWildcardDBA),
        SAnimNode("animgroup", "animgroup.dba", "persistent", e_chrParamsAnimationNodeSingleDBA),
    };
    uint validSubCount = ARRAY_COUNT(subAnimListValid);

    uint bboxExcludeList[] =
    {
        3,
        5,
        1,
        7,
    };
    uint bboxExcludeCount = ARRAY_COUNT(bboxExcludeList);

    uint bboxIncludeList[] =
    {
        2,
        4,
        0,
        6,
    };
    uint bboxIncludeCount = ARRAY_COUNT(bboxIncludeList);

    uint boneLodJointCount[] =
    {
        10,
        8,
        5,
    };
    uint boneLodCount = ARRAY_COUNT(boneLodJointCount);

    Vec3 bbMin(-1.f, -1.f, -1.f);
    Vec3 bbMax(1.f, 1.f, 1.f);

    struct SDummyLimbIK
    {
        string handle;
        int joints[5];
        string solver;
        float threshold;
        float stepSize;
        int iterations;
    };

    SDummyLimbIK dummyLimbIK[] =
    {
        { "Sample2BIK", { 0, 1, -1, -1, -1 }, "2BIK", 0.f, 0.f, 0 },
        { "Sample3BIK", { 2, 3, -1, -1, -1 }, "3BIK", 0.f, 0.f, 0 },
        { "SampleCCDX", { 4, 9, -1, -1, -1 }, "CCDX", 1.f, 1.f, 12 },
        { "SampleCCD5", { 4, 5, 6, 7, 8 }, "CCD5", 0.f, 0.f, 0 },
    };
    uint dummyLimbIKCount = ARRAY_COUNT(dummyLimbIK);

    struct SDummyADIK
    {
        string handle;
        int targetJoint;
        int weightJoint;
    };

    SDummyADIK dummyADIK[] =
    {
        { "ADIK1", 0, 1 },
        { "ADIK2", 2, 3 },
    };
    uint dummyADIKCount = ARRAY_COUNT(dummyADIK);

    string dummyFootLockIK[] =
    {
        "SampleHandle1",
        "SampleHandle2",
        "SampleHandle3",
        "SampleHandle4",
    };
    uint dummyFootLockIKCount = ARRAY_COUNT(dummyFootLockIK);

    struct SDummyRecoilImpactJoint
    {
        int arm;
        float delay;
        float weight;
        int joint;
    };

    struct SDummyRecoilIK
    {
        string rightIKHandle;
        string leftIKHandle;
        int rightWeaponJoint;
        int leftWeaponJoint;
        DynArray<SDummyRecoilImpactJoint> impactJoints;
        SDummyRecoilIK()
        {
            rightIKHandle = "SampleRightIKHandle";
            leftIKHandle = "SampleLeftIKHandle";
            rightWeaponJoint = 1;
            leftWeaponJoint = 6;

            SDummyRecoilImpactJoint impactJoint1 = {1, 0.2f, 0.5f, 3};
            impactJoints.push_back(impactJoint1);
            SDummyRecoilImpactJoint impactJoint2 = {2, 0.3f, 0.75f, 4};
            impactJoints.push_back(impactJoint2);
            SDummyRecoilImpactJoint impactJoint3 = {3, 0.5f, 1.f, 5};
            impactJoints.push_back(impactJoint3);
        }
    };

    SDummyRecoilIK dummyRecoilIK;

    struct SDummyDirectionalBlend
    {
        string animToken;
        int parameterJoint;
        int startJoint;
        int referenceJoint;
    };

    struct SDummyPosRot
    {
        SDummyPosRot(int jointVal, bool additiveVal, bool primaryVal)
            : joint(jointVal)
            , additive(additiveVal)
            , primary(primaryVal) {}
        int joint;
        bool additive;
        bool primary;
    };

    struct SDummyLookIK
    {
        int leftEyeAttachment;
        int rightEyeAttachment;
        float eyeLimitHalfYaw;
        float eyeLimitPitchUp;
        float eyeLimitPitchDown;
        DynArray<SDummyPosRot> posList;
        DynArray<SDummyPosRot> rotList;
        DynArray<SDummyDirectionalBlend> dirBlends;
        SDummyLookIK()
        {
            leftEyeAttachment = 5;
            rightEyeAttachment = 6;
            eyeLimitHalfYaw = 30.f;
            eyeLimitPitchUp = 25.f;
            eyeLimitPitchDown = 15.f;
            posList.push_back(SDummyPosRot(0, false, false));
            posList.push_back(SDummyPosRot(1, false, false));
            posList.push_back(SDummyPosRot(2, true, false));
            rotList.push_back(SDummyPosRot(0, false, true));
            rotList.push_back(SDummyPosRot(1, false, true));
            rotList.push_back(SDummyPosRot(2, true, false));

            SDummyDirectionalBlend blend1 = {"LookDirBlend1", 0, 1, 2};
            dirBlends.push_back(blend1);
            SDummyDirectionalBlend blend2 = {"LookDirBlend2", 3, 4, 5};
            dirBlends.push_back(blend2);
        }
    };

    SDummyLookIK dummyLookIK;

    struct SDummyAimIK
    {
        DynArray<SDummyPosRot> posList;
        DynArray<SDummyPosRot> rotList;
        DynArray<SDummyDirectionalBlend> dirBlends;
        DynArray<int> procAdjustments;
        SDummyAimIK()
        {
            posList.push_back(SDummyPosRot(3, false, false));
            posList.push_back(SDummyPosRot(4, false, false));
            posList.push_back(SDummyPosRot(5, true, false));
            rotList.push_back(SDummyPosRot(3, false, true));
            rotList.push_back(SDummyPosRot(4, true, false));
            rotList.push_back(SDummyPosRot(5, true, false));
            SDummyDirectionalBlend blend1 = {"AimDirBlend1", 3, 4, 5};
            dirBlends.push_back(blend1);
            SDummyDirectionalBlend blend2 = {"AimDirBlend2", 7, 8, 9};
            dirBlends.push_back(blend2);
            procAdjustments.push_back(1);
            procAdjustments.push_back(2);
        }
    };

    SDummyAimIK dummyAimIK;

    string GetJointName(uint jointIndex)
    {
        assert(jointIndex < jointCount);
        return jointList[jointIndex];
    }

    const IChrParams* GetChrParams(string chrParamsName)
    {
        return g_dummyCharacterManager->GetParamLoader().GetChrParams(chrParamsName.c_str());
    }

    IChrParams* GetEmptyChrParams(string chrParamsName)
    {
        return g_dummyCharacterManager->GetParamLoader().ClearChrParams(chrParamsName.c_str());
    }

    IChrParams* GetEditableChrParams(string chrParamsName)
    {
        return g_dummyCharacterManager->GetParamLoader().GetEditableChrParams(chrParamsName.c_str());
    }

    // LDS: Gets or creates two chrparams files and stores a basic structure within each of them for testing
    void BuildSampleChrParams(string chrParamsFile, string subChrParamsFile)
    {
        g_dummyCharacterManager = new CharacterManager;
        IChrParams* params = GetEmptyChrParams(chrParamsFile);
        IChrParams* subParams = GetEmptyChrParams(subChrParamsFile);

        CRY_ASSERT(params && subParams);

        DynArray<string> bboxExcludeJoints;
        DynArray<string> bboxIncludeJoints;
        DynArray<SAnimNode> testAnimList;
        DynArray<SAnimNode> subAnimList;

        for (uint i = 0; i < bboxExcludeCount; ++i)
        {
            bboxExcludeJoints.push_back(GetJointName(bboxExcludeList[i]));
        }

        for (uint i = 0; i < bboxIncludeCount; ++i)
        {
            bboxIncludeJoints.push_back(GetJointName(bboxIncludeList[i]));
        }

        for (uint i = 0; i < validRootCount; ++i)
        {
            testAnimList.push_back(animListValid[i]);
        }

        for (uint i = 0; i < validSubCount; ++i)
        {
            subAnimList.push_back(subAnimListValid[i]);
        }

        for (uint i = 0; i < boneLodCount; ++i)
        {
            DynArray<string> lodJointList;
            for (uint j = 0; j < boneLodJointCount[i]; ++j)
            {
                lodJointList.push_back(GetJointName(j));
            }
            params->AddBoneLod(i, lodJointList);
        }

        params->AddBBoxExcludeJoints(bboxExcludeJoints);
        params->AddBBoxIncludeJoints(bboxIncludeJoints);
        params->SetBBoxExpansion(bbMin, bbMax);
        params->AddAnimations(testAnimList);

        params->SetUsePhysProxyBBox(true);

        IChrParamsIKDefNode* ikDef = static_cast<IChrParamsIKDefNode*>(params->GetEditableCategoryNode(e_chrParamsNodeIKDefinition));

        DynArray<SLimbIKDef> newLimbIK;
        for (uint i = 0; i < dummyLimbIKCount; ++i)
        {
            SLimbIKDef limbIKDef;
            limbIKDef.m_handle = dummyLimbIK[i].handle;
            limbIKDef.m_solver = dummyLimbIK[i].solver;
            if (limbIKDef.m_solver == "CCD5")
            {
                for (uint j = 0; j < 5; ++j)
                {
                    limbIKDef.m_ccdFiveJoints.push_back(GetJointName(dummyLimbIK[i].joints[j]));
                }
            }
            else
            {
                limbIKDef.m_rootJoint = GetJointName(dummyLimbIK[i].joints[0]);
                limbIKDef.m_endEffector = GetJointName(dummyLimbIK[i].joints[1]);
            }
            limbIKDef.m_maxIterations = dummyLimbIK[i].iterations;
            limbIKDef.m_stepSize = dummyLimbIK[i].stepSize;
            limbIKDef.m_threshold = dummyLimbIK[i].threshold;

            newLimbIK.push_back(limbIKDef);
        }
        IChrParamsLimbIKDef* limbIKNode = static_cast<IChrParamsLimbIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeLimb));
        limbIKNode->SetLimbIKDefs(newLimbIK);

        DynArray<SAnimDrivenIKDef> newAnimDrivenIK;
        for (uint i = 0; i < dummyADIKCount; ++i)
        {
            SAnimDrivenIKDef adIKDef;
            adIKDef.m_handle = dummyADIK[i].handle;
            adIKDef.m_targetJoint = GetJointName(dummyADIK[i].targetJoint);
            adIKDef.m_weightJoint = GetJointName(dummyADIK[i].weightJoint);
            newAnimDrivenIK.push_back(adIKDef);
        }
        IChrParamsAnimDrivenIKDef* adIKNode = static_cast<IChrParamsAnimDrivenIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeAnimDriven));
        adIKNode->SetAnimDrivenIKDefs(newAnimDrivenIK);

        DynArray<string> newFootLockIK;
        for (uint i = 0; i < dummyFootLockIKCount; ++i)
        {
            newFootLockIK.push_back(dummyFootLockIK[i]);
        }
        IChrParamsFootLockIKDef* footLockIKNode = static_cast<IChrParamsFootLockIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeFootLock));
        footLockIKNode->SetFootLockIKDefs(newFootLockIK);

        IChrParamsRecoilIKDef* recoilIKNode = static_cast<IChrParamsRecoilIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeRecoil));
        recoilIKNode->SetLeftIKHandle(dummyRecoilIK.leftIKHandle);
        recoilIKNode->SetRightIKHandle(dummyRecoilIK.rightIKHandle);
        recoilIKNode->SetLeftWeaponJoint(GetJointName(dummyRecoilIK.leftWeaponJoint));
        recoilIKNode->SetRightWeaponJoint(GetJointName(dummyRecoilIK.rightWeaponJoint));
        uint impactJointCount = dummyRecoilIK.impactJoints.size();
        DynArray<SRecoilImpactJoint> newImpactJoints;
        for (uint i = 0; i < impactJointCount; ++i)
        {
            SRecoilImpactJoint impactJoint;
            impactJoint.m_arm = dummyRecoilIK.impactJoints[i].arm;
            impactJoint.m_delay = dummyRecoilIK.impactJoints[i].delay;
            impactJoint.m_weight = dummyRecoilIK.impactJoints[i].weight;
            impactJoint.m_jointName = GetJointName(dummyRecoilIK.impactJoints[i].joint);
            newImpactJoints.push_back(impactJoint);
        }
        recoilIKNode->SetImpactJoints(newImpactJoints);

        IChrParamsLookIKDef* lookIKNode = static_cast<IChrParamsLookIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeLook));
        lookIKNode->SetLeftEyeAttachment(GetJointName(dummyLookIK.leftEyeAttachment));
        lookIKNode->SetRightEyeAttachment(GetJointName(dummyLookIK.rightEyeAttachment));
        SEyeLimits newEyeLimits;
        newEyeLimits.m_halfYaw = dummyLookIK.eyeLimitHalfYaw;
        newEyeLimits.m_pitchUp = dummyLookIK.eyeLimitPitchUp;
        newEyeLimits.m_pitchDown = dummyLookIK.eyeLimitPitchDown;
        lookIKNode->SetEyeLimits(newEyeLimits);
        uint rotCount = dummyLookIK.rotList.size();
        DynArray<SAimLookPosRot> newRotList;
        for (uint i = 0; i < rotCount; ++i)
        {
            SAimLookPosRot rot;
            rot.m_jointName = GetJointName(dummyLookIK.rotList[i].joint);
            rot.m_additive = dummyLookIK.rotList[i].additive;
            rot.m_primary = dummyLookIK.rotList[i].primary;
            newRotList.push_back(rot);
        }
        lookIKNode->SetRotationList(newRotList);
        uint posCount = dummyLookIK.posList.size();
        DynArray<SAimLookPosRot> newPosList;
        for (uint i = 0; i < posCount; ++i)
        {
            SAimLookPosRot pos;
            pos.m_jointName = GetJointName(dummyLookIK.posList[i].joint);
            pos.m_additive = dummyLookIK.posList[i].additive;
            newPosList.push_back(pos);
        }
        lookIKNode->SetPositionList(newPosList);
        uint dirBlendCount = dummyLookIK.dirBlends.size();
        DynArray<SAimLookDirectionalBlend> newDirBlends;
        for (uint i = 0; i < dirBlendCount; ++i)
        {
            SAimLookDirectionalBlend dirBlend;
            dirBlend.m_animToken = dummyLookIK.dirBlends[i].animToken;
            dirBlend.m_parameterJoint = GetJointName(dummyLookIK.dirBlends[i].parameterJoint);
            dirBlend.m_startJoint = GetJointName(dummyLookIK.dirBlends[i].startJoint);
            dirBlend.m_referenceJoint = GetJointName(dummyLookIK.dirBlends[i].referenceJoint);
            newDirBlends.push_back(dirBlend);
        }
        lookIKNode->SetDirectionalBlends(newDirBlends);

        IChrParamsAimIKDef* aimIKNode = static_cast<IChrParamsAimIKDef*>(ikDef->GetEditableIKSubNode(e_chrParamsIKNodeAim));
        rotCount = dummyAimIK.rotList.size();
        newRotList.clear();
        for (uint i = 0; i < rotCount; ++i)
        {
            SAimLookPosRot rot;
            rot.m_jointName = GetJointName(dummyAimIK.rotList[i].joint);
            rot.m_additive = dummyAimIK.rotList[i].additive;
            rot.m_primary = dummyAimIK.rotList[i].primary;
            newRotList.push_back(rot);
        }
        aimIKNode->SetRotationList(newRotList);
        posCount = dummyAimIK.posList.size();
        newPosList.clear();
        for (uint i = 0; i < posCount; ++i)
        {
            SAimLookPosRot pos;
            pos.m_jointName = GetJointName(dummyAimIK.posList[i].joint);
            pos.m_additive = dummyAimIK.posList[i].additive;
            newPosList.push_back(pos);
        }
        aimIKNode->SetPositionList(newPosList);
        dirBlendCount = dummyAimIK.dirBlends.size();
        newDirBlends.clear();
        for (uint i = 0; i < dirBlendCount; ++i)
        {
            SAimLookDirectionalBlend dirBlend;
            dirBlend.m_animToken = dummyAimIK.dirBlends[i].animToken;
            dirBlend.m_parameterJoint = GetJointName(dummyAimIK.dirBlends[i].parameterJoint);
            dirBlend.m_startJoint = GetJointName(dummyAimIK.dirBlends[i].startJoint);
            dirBlend.m_referenceJoint = GetJointName(dummyAimIK.dirBlends[i].referenceJoint);
            newDirBlends.push_back(dirBlend);
        }
        aimIKNode->SetDirectionalBlends(newDirBlends);
        uint procAdjustCount = dummyAimIK.procAdjustments.size();
        DynArray<string> newProcAdjustments;
        for (uint i = 0; i < procAdjustCount; ++i)
        {
            newProcAdjustments.push_back(GetJointName(dummyAimIK.procAdjustments[i]));
        }
        aimIKNode->SetProcAdjustments(newProcAdjustments);

        subParams->AddAnimations(subAnimList);
    }
}

// Only compile this under debug or profile
TEST(CryAnimationParamLoaderTest, TestCreate)
{
    PL_UT_HELPER::BuildSampleChrParams(PL_UT_HELPER::test1Path, PL_UT_HELPER::subPath);

    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* sub1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::subPath);

    // Were all of the requested ChrParams structures successfully created and registered with the IChrParamsParser?
    EXPECT_TRUE(test1 && sub1);
}

TEST(CryAnimationParamLoaderTest, TestBBoxExcludeAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsBBoxExcludeNode* bboxExcludeNode = static_cast<const IChrParamsBBoxExcludeNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxExcludeList));
    uint excludeCount = bboxExcludeNode->GetJointCount();

    EXPECT_TRUE(excludeCount == PL_UT_HELPER::bboxExcludeCount);
    for (uint i = 0; i < excludeCount; ++i)
    {
        EXPECT_TRUE(bboxExcludeNode->GetJointName(i) == PL_UT_HELPER::GetJointName(PL_UT_HELPER::bboxExcludeList[i]));
    }
}

TEST(CryAnimationParamLoaderTest, TestBBoxIncludeAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsBBoxIncludeNode* bboxIncludeNode = static_cast<const IChrParamsBBoxIncludeNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxIncludeList));
    uint includeCount = bboxIncludeNode->GetJointCount();

    EXPECT_TRUE(includeCount == PL_UT_HELPER::bboxIncludeCount);
    for (uint i = 0; i < includeCount; ++i)
    {
        EXPECT_TRUE(bboxIncludeNode->GetJointName(i) == PL_UT_HELPER::GetJointName(PL_UT_HELPER::bboxIncludeList[i]));
    }
}

TEST(CryAnimationParamLoaderTest, TestBBoxExtensionSet)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsBBoxExtensionNode* bboxExtensionNode = static_cast<const IChrParamsBBoxExtensionNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxExtensionList));
    Vec3 bbMin, bbMax;
    bboxExtensionNode->GetBBoxMaxMin(bbMin, bbMax);

    EXPECT_TRUE(bbMin == PL_UT_HELPER::bbMin);
    EXPECT_TRUE(bbMax == PL_UT_HELPER::bbMax);
}

TEST(CryAnimationParamLoaderTest, TestBoneLodAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsBoneLodNode* boneLodNode = static_cast<const IChrParamsBoneLodNode*>(test1->GetCategoryNode(e_chrParamsNodeBoneLod));
    uint boneLodCount = boneLodNode->GetLodCount();

    // Same number of bone LODs?
    EXPECT_TRUE(boneLodCount == PL_UT_HELPER::boneLodCount);
    for (uint i = 0; i < boneLodCount; ++i)
    {
        // Same number of bones in this LOD?
        uint jointCount = boneLodNode->GetJointCount(i);
        EXPECT_TRUE(jointCount == PL_UT_HELPER::boneLodJointCount[i]);
        for (uint j = 0; j < jointCount; j++)
        {
            // Same bone names in this LOD?
            EXPECT_TRUE(boneLodNode->GetJointName(i, j) == PL_UT_HELPER::GetJointName(j));
        }
    }
}

TEST(CryAnimationParamLoaderTest, TestLimbIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsLimbIKDef* limbIKNode = static_cast<const IChrParamsLimbIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeLimb));

    // Same number of limbIKDefs?
    uint limbIKDefCount = limbIKNode->GetLimbIKDefCount();
    EXPECT_TRUE(limbIKDefCount == PL_UT_HELPER::dummyLimbIKCount);
    for (uint i = 0; i < limbIKDefCount; ++i)
    {
        const SLimbIKDef* limbIKDef = limbIKNode->GetLimbIKDef(i);
        // Same handle and solver?
        EXPECT_TRUE(limbIKDef->m_handle == PL_UT_HELPER::dummyLimbIK[i].handle);
        EXPECT_TRUE(limbIKDef->m_solver == PL_UT_HELPER::dummyLimbIK[i].solver);
        // CCD5 validation
        if (limbIKDef->m_solver == "CCD5")
        {
            EXPECT_TRUE(limbIKDef->m_ccdFiveJoints.size() == 5);
            for (uint j = 0; j < 5; j++)
            {
                EXPECT_TRUE(limbIKDef->m_ccdFiveJoints[j] == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyLimbIK[i].joints[j]));
            }
        }
        // Validation for all other solvers
        else
        {
            EXPECT_TRUE(limbIKDef->m_rootJoint == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyLimbIK[i].joints[0]));
            EXPECT_TRUE(limbIKDef->m_endEffector == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyLimbIK[i].joints[1]));
            EXPECT_TRUE(limbIKDef->m_stepSize == PL_UT_HELPER::dummyLimbIK[i].stepSize);
            EXPECT_TRUE(limbIKDef->m_threshold == PL_UT_HELPER::dummyLimbIK[i].threshold);
            EXPECT_TRUE(limbIKDef->m_maxIterations == PL_UT_HELPER::dummyLimbIK[i].iterations);
        }
    }
}

TEST(CryAnimationParamLoaderTest, TestAnimDrivenIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsAnimDrivenIKDef* adIKNode = static_cast<const IChrParamsAnimDrivenIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeAnimDriven));

    // Same number of AnimDrivenIKDefs?
    uint adIKCount = adIKNode->GetAnimDrivenIKDefCount();
    EXPECT_TRUE(adIKCount == PL_UT_HELPER::dummyADIKCount);
    for (uint i = 0; i < adIKCount; ++i)
    {
        // Same Data and ordering?
        const SAnimDrivenIKDef* adIKDef = adIKNode->GetAnimDrivenIKDef(i);
        EXPECT_TRUE(adIKDef->m_handle == PL_UT_HELPER::dummyADIK[i].handle);
        EXPECT_TRUE(adIKDef->m_targetJoint == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyADIK[i].targetJoint));
        EXPECT_TRUE(adIKDef->m_weightJoint == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyADIK[i].weightJoint));
    }
}

TEST(CryAnimationParamLoaderTest, TestFootLockIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsFootLockIKDef* footLockIKNode = static_cast<const IChrParamsFootLockIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeFootLock));

    // Same number of foot lock defs?
    uint footLockIKCount = footLockIKNode->GetFootLockIKDefCount();
    EXPECT_TRUE(footLockIKCount == PL_UT_HELPER::dummyFootLockIKCount);
    for (uint i = 0; i < footLockIKCount; ++i)
    {
        //Same data and ordering?
        EXPECT_TRUE(footLockIKNode->GetFootLockIKDef(i) == PL_UT_HELPER::dummyFootLockIK[i]);
    }
}

TEST(CryAnimationParamLoaderTest, TestRecoilIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsRecoilIKDef* recoilIKNode = static_cast<const IChrParamsRecoilIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeRecoil));

    // Same handles and bones?
    EXPECT_TRUE(recoilIKNode->GetRightIKHandle() == PL_UT_HELPER::dummyRecoilIK.rightIKHandle);
    EXPECT_TRUE(recoilIKNode->GetLeftIKHandle() == PL_UT_HELPER::dummyRecoilIK.leftIKHandle);
    EXPECT_TRUE(recoilIKNode->GetRightWeaponJoint() == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyRecoilIK.rightWeaponJoint));
    EXPECT_TRUE(recoilIKNode->GetLeftWeaponJoint() == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyRecoilIK.leftWeaponJoint));

    // Same number of impact joints?
    uint impactJointCount = recoilIKNode->GetImpactJointCount();
    EXPECT_TRUE(impactJointCount == PL_UT_HELPER::dummyRecoilIK.impactJoints.size());
    for (uint i = 0; i < impactJointCount; ++i)
    {
        // Same data and ordering?
        const SRecoilImpactJoint* impactJoint = recoilIKNode->GetImpactJoint(i);
        const PL_UT_HELPER::SDummyRecoilImpactJoint* dummyImpactJoint = &PL_UT_HELPER::dummyRecoilIK.impactJoints[i];
        EXPECT_TRUE(impactJoint->m_arm == dummyImpactJoint->arm);
        EXPECT_TRUE(impactJoint->m_delay == dummyImpactJoint->delay);
        EXPECT_TRUE(impactJoint->m_weight == dummyImpactJoint->weight);
        EXPECT_TRUE(impactJoint->m_jointName == PL_UT_HELPER::GetJointName(dummyImpactJoint->joint));
    }
}

TEST(CryAnimationParamLoaderTest, TestLookIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsLookIKDef* lookIKNode = static_cast<const IChrParamsLookIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeLook));

    // Same eye data?
    EXPECT_TRUE(lookIKNode->GetLeftEyeAttachment() == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyLookIK.leftEyeAttachment));
    EXPECT_TRUE(lookIKNode->GetRightEyeAttachment() == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyLookIK.rightEyeAttachment));
    const SEyeLimits* eyeLimits = lookIKNode->GetEyeLimits();
    EXPECT_TRUE(eyeLimits->m_halfYaw == PL_UT_HELPER::dummyLookIK.eyeLimitHalfYaw);
    EXPECT_TRUE(eyeLimits->m_pitchUp == PL_UT_HELPER::dummyLookIK.eyeLimitPitchUp);
    EXPECT_TRUE(eyeLimits->m_pitchDown == PL_UT_HELPER::dummyLookIK.eyeLimitPitchDown);

    // Same number of rotations specified?
    uint rotCount = lookIKNode->GetRotationCount();
    EXPECT_TRUE(rotCount == PL_UT_HELPER::dummyLookIK.rotList.size());
    for (uint i = 0; i < rotCount; ++i)
    {
        // Same data and ordering?
        const SAimLookPosRot* rot = lookIKNode->GetRotation(i);
        const PL_UT_HELPER::SDummyPosRot* dummyRot = &PL_UT_HELPER::dummyLookIK.rotList[i];
        EXPECT_TRUE(rot->m_jointName == PL_UT_HELPER::GetJointName(dummyRot->joint));
        EXPECT_TRUE(rot->m_additive == dummyRot->additive);
        EXPECT_TRUE(rot->m_primary == dummyRot->primary);
    }

    // Same number of rotations specified?
    uint posCount = lookIKNode->GetPositionCount();
    EXPECT_TRUE(posCount == PL_UT_HELPER::dummyLookIK.posList.size());
    for (uint i = 0; i < posCount; ++i)
    {
        // Same data and ordering?
        const SAimLookPosRot* pos = lookIKNode->GetPosition(i);
        const PL_UT_HELPER::SDummyPosRot* dummyPos = &PL_UT_HELPER::dummyLookIK.posList[i];
        EXPECT_TRUE(pos->m_jointName == PL_UT_HELPER::GetJointName(dummyPos->joint));
        EXPECT_TRUE(pos->m_additive == dummyPos->additive);
        EXPECT_TRUE(pos->m_primary == dummyPos->primary);
    }

    // Same number of directional blends?
    uint dirBlendCount = lookIKNode->GetDirectionalBlendCount();
    EXPECT_TRUE(dirBlendCount == PL_UT_HELPER::dummyLookIK.dirBlends.size());
    for (uint i = 0; i < dirBlendCount; ++i)
    {
        // Same data and ordering?
        const SAimLookDirectionalBlend* dirBlend = lookIKNode->GetDirectionalBlend(i);
        const PL_UT_HELPER::SDummyDirectionalBlend* dummyDirBlend = &PL_UT_HELPER::dummyLookIK.dirBlends[i];
        EXPECT_TRUE(dirBlend->m_animToken == dummyDirBlend->animToken);
        EXPECT_TRUE(dirBlend->m_parameterJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->parameterJoint));
        EXPECT_TRUE(dirBlend->m_startJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->startJoint));
        EXPECT_TRUE(dirBlend->m_referenceJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->referenceJoint));
    }
}

TEST(CryAnimationParamLoaderTest, TestAimIKAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParamsIKDefNode* ikDefNode = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsAimIKDef* aimIKNode = static_cast<const IChrParamsAimIKDef*>(ikDefNode->GetIKSubNode(e_chrParamsIKNodeAim));

    // Same number of rotations specified?
    uint rotCount = aimIKNode->GetRotationCount();
    EXPECT_TRUE(rotCount == PL_UT_HELPER::dummyAimIK.rotList.size());
    for (uint i = 0; i < rotCount; ++i)
    {
        // Same data and ordering?
        const SAimLookPosRot* rot = aimIKNode->GetRotation(i);
        const PL_UT_HELPER::SDummyPosRot* dummyRot = &PL_UT_HELPER::dummyAimIK.rotList[i];
        EXPECT_TRUE(rot->m_jointName == PL_UT_HELPER::GetJointName(dummyRot->joint));
        EXPECT_TRUE(rot->m_additive == dummyRot->additive);
        EXPECT_TRUE(rot->m_primary == dummyRot->primary);
    }

    // Same number of rotations specified?
    uint posCount = aimIKNode->GetPositionCount();
    EXPECT_TRUE(posCount == PL_UT_HELPER::dummyAimIK.posList.size());
    for (uint i = 0; i < posCount; ++i)
    {
        // Same data and ordering?
        const SAimLookPosRot* pos = aimIKNode->GetPosition(i);
        const PL_UT_HELPER::SDummyPosRot* dummyPos = &PL_UT_HELPER::dummyAimIK.posList[i];
        EXPECT_TRUE(pos->m_jointName == PL_UT_HELPER::GetJointName(dummyPos->joint));
        EXPECT_TRUE(pos->m_additive == dummyPos->additive);
        EXPECT_TRUE(pos->m_primary == dummyPos->primary);
    }

    // Same number of directional blends?
    uint dirBlendCount = aimIKNode->GetDirectionalBlendCount();
    EXPECT_TRUE(dirBlendCount == PL_UT_HELPER::dummyAimIK.dirBlends.size());
    for (uint i = 0; i < dirBlendCount; ++i)
    {
        // Same data and ordering?
        const SAimLookDirectionalBlend* dirBlend = aimIKNode->GetDirectionalBlend(i);
        const PL_UT_HELPER::SDummyDirectionalBlend* dummyDirBlend = &PL_UT_HELPER::dummyAimIK.dirBlends[i];
        EXPECT_TRUE(dirBlend->m_animToken == dummyDirBlend->animToken);
        EXPECT_TRUE(dirBlend->m_parameterJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->parameterJoint));
        EXPECT_TRUE(dirBlend->m_startJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->startJoint));
        EXPECT_TRUE(dirBlend->m_referenceJoint == PL_UT_HELPER::GetJointName(dummyDirBlend->referenceJoint));
    }

    // Same number of procadjustments?
    uint procAdjustCount = aimIKNode->GetProcAdjustmentCount();
    EXPECT_TRUE(procAdjustCount == PL_UT_HELPER::dummyAimIK.procAdjustments.size());
    for (uint i = 0; i < procAdjustCount; ++i)
    {
        // Same data and ordering?
        EXPECT_TRUE(aimIKNode->GetProcAdjustmentJoint(i) == PL_UT_HELPER::GetJointName(PL_UT_HELPER::dummyAimIK.procAdjustments[i]));
    }
}

TEST(CryAnimationParamLoaderTest, TestAnimationListAdd)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* sub1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::subPath);

    // Root ChrParams
    const IChrParamsAnimationListNode* animListNode = static_cast<const IChrParamsAnimationListNode*>(test1->GetCategoryNode(e_chrParamsNodeAnimationList));
    uint animCount = animListNode->GetAnimationCount();
    EXPECT_TRUE(animCount == PL_UT_HELPER::validRootCount);
    for (uint i = 0; i < animCount; ++i)
    {
        const SAnimNode* animNode = animListNode->GetAnimation(i);
        EXPECT_TRUE(animNode->m_name == PL_UT_HELPER::animListValid[i].m_name);
        EXPECT_TRUE(animNode->m_path == PL_UT_HELPER::animListValid[i].m_path);
        EXPECT_TRUE(animNode->m_flags == PL_UT_HELPER::animListValid[i].m_flags);
        EXPECT_TRUE(animNode->m_type == PL_UT_HELPER::animListValid[i].m_type);
    }

    // Sub ChrParams
    animListNode = static_cast<const IChrParamsAnimationListNode*>(sub1->GetCategoryNode(e_chrParamsNodeAnimationList));
    animCount = animListNode->GetAnimationCount();
    EXPECT_TRUE(animCount == PL_UT_HELPER::validSubCount);
    for (uint i = 0; i < animCount; ++i)
    {
        const SAnimNode* animNode = animListNode->GetAnimation(i);
        EXPECT_TRUE(animNode->m_name == PL_UT_HELPER::subAnimListValid[i].m_name);
        EXPECT_TRUE(animNode->m_path == PL_UT_HELPER::subAnimListValid[i].m_path);
        EXPECT_TRUE(animNode->m_flags == PL_UT_HELPER::subAnimListValid[i].m_flags);
        EXPECT_TRUE(animNode->m_type == PL_UT_HELPER::subAnimListValid[i].m_type);
    }
}

#if 0 // Disabled: SEH exception with code 0xc0000005 thrown in the test body.
TEST(CryAnimationParamLoaderFeatureTest, TestWrite)
{
    EXPECT_TRUE(PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().WriteXML(PL_UT_HELPER::test1Path));
}

TEST(CryAnimationParamLoaderFeatureTest, TestReload)
{
    PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().DeleteParsedChrParams(PL_UT_HELPER::test1Path);
    PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().DeleteParsedChrParams(PL_UT_HELPER::subPath);
    EXPECT_TRUE(PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().ParseXML(PL_UT_HELPER::test1Path) != NULL);
}

TEST(CryAnimationParamLoaderFeatureTest, TestBoneLodConsistency)
{
    PL_UT_HELPER::BuildSampleChrParams(PL_UT_HELPER::test2Path, PL_UT_HELPER::subPath);

    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsBoneLodNode* boneLod1 = static_cast<const IChrParamsBoneLodNode*>(test1->GetCategoryNode(e_chrParamsNodeBoneLod));
    const IChrParamsBoneLodNode* boneLod2 = static_cast<const IChrParamsBoneLodNode*>(test2->GetCategoryNode(e_chrParamsNodeBoneLod));

    EXPECT_TRUE(boneLod1->GetLodCount() == boneLod2->GetLodCount());
    uint lodCount = boneLod1->GetLodCount();
    for (uint i = 0; i < lodCount; ++i)
    {
        EXPECT_TRUE(boneLod1->GetJointCount(i) == boneLod2->GetJointCount(i));
        uint jointCount = boneLod1->GetJointCount(i);
        for (uint j = 0; j < jointCount; ++j)
        {
            EXPECT_TRUE(boneLod1->GetJointName(i, j) == boneLod2->GetJointName(i, j));
        }
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestBBoxExcludeConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsBBoxExcludeNode* bboxExclude1 = static_cast<const IChrParamsBBoxExcludeNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxExcludeList));
    const IChrParamsBBoxExcludeNode* bboxExclude2 = static_cast<const IChrParamsBBoxExcludeNode*>(test2->GetCategoryNode(e_chrParamsNodeBBoxExcludeList));

    EXPECT_TRUE(bboxExclude1->GetJointCount() == bboxExclude2->GetJointCount());
    uint jointCount = bboxExclude1->GetJointCount();
    for (uint i = 0; i < jointCount; ++i)
    {
        EXPECT_TRUE(bboxExclude1->GetJointName(i) == bboxExclude2->GetJointName(i));
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestBBoxIncludeConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsBBoxIncludeNode* bboxInclude1 = static_cast<const IChrParamsBBoxIncludeNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxIncludeList));
    const IChrParamsBBoxIncludeNode* bboxInclude2 = static_cast<const IChrParamsBBoxIncludeNode*>(test2->GetCategoryNode(e_chrParamsNodeBBoxIncludeList));

    EXPECT_TRUE(bboxInclude1->GetJointCount() == bboxInclude2->GetJointCount());
    uint jointCount = bboxInclude1->GetJointCount();
    for (uint i = 0; i < jointCount; ++i)
    {
        EXPECT_TRUE(bboxInclude1->GetJointName(i) == bboxInclude2->GetJointName(i));
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestBBoxExtensionConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsBBoxExtensionNode* bboxExtension1 = static_cast<const IChrParamsBBoxExtensionNode*>(test1->GetCategoryNode(e_chrParamsNodeBBoxExtensionList));
    const IChrParamsBBoxExtensionNode* bboxExtension2 = static_cast<const IChrParamsBBoxExtensionNode*>(test2->GetCategoryNode(e_chrParamsNodeBBoxExtensionList));

    Vec3 bbMin1, bbMax1, bbMin2, bbMax2;
    bboxExtension1->GetBBoxMaxMin(bbMin1, bbMax1);
    bboxExtension2->GetBBoxMaxMin(bbMin2, bbMax2);

    EXPECT_TRUE(bbMin1 == bbMin2);
    EXPECT_TRUE(bbMax1 == bbMax2);
}

TEST(CryAnimationParamLoaderFeatureTest, TestLimbIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsLimbIKDef* limbIK1 = static_cast<const IChrParamsLimbIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeLimb));
    const IChrParamsLimbIKDef* limbIK2 = static_cast<const IChrParamsLimbIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeLimb));

    EXPECT_TRUE(limbIK1->GetLimbIKDefCount() == limbIK2->GetLimbIKDefCount());
    uint limbIKDefCount = limbIK1->GetLimbIKDefCount();
    for (uint i = 0; i < limbIKDefCount; ++i)
    {
        const SLimbIKDef* limbDef1 = limbIK1->GetLimbIKDef(i);
        const SLimbIKDef* limbDef2 = limbIK2->GetLimbIKDef(i);
        EXPECT_TRUE(limbDef1->m_handle == limbDef2->m_handle);
        EXPECT_TRUE(limbDef1->m_solver == limbDef2->m_solver);
        EXPECT_TRUE(limbDef1->m_rootJoint == limbDef2->m_rootJoint);
        EXPECT_TRUE(limbDef1->m_endEffector == limbDef2->m_endEffector);
        EXPECT_NEAR(limbDef1->m_stepSize, limbDef2->m_stepSize, 0.01f);
        EXPECT_NEAR(limbDef1->m_threshold, limbDef2->m_threshold, 0.01f);
        EXPECT_TRUE(limbDef1->m_maxIterations == limbDef2->m_maxIterations);
        EXPECT_TRUE(limbDef1->m_ccdFiveJoints.size() == limbDef2->m_ccdFiveJoints.size());
        uint ccdFiveCount = limbDef1->m_ccdFiveJoints.size();
        for (uint j = 0; j < ccdFiveCount; ++j)
        {
            EXPECT_TRUE(limbDef1->m_ccdFiveJoints[j] == limbDef2->m_ccdFiveJoints[j]);
        }
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestAnimDrivenIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsAnimDrivenIKDef* adIK1 = static_cast<const IChrParamsAnimDrivenIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeAnimDriven));
    const IChrParamsAnimDrivenIKDef* adIK2 = static_cast<const IChrParamsAnimDrivenIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeAnimDriven));

    EXPECT_TRUE(adIK1->GetAnimDrivenIKDefCount() == adIK2->GetAnimDrivenIKDefCount());
    uint adIKDefCount = adIK1->GetAnimDrivenIKDefCount();
    for (uint i = 0; i < adIKDefCount; ++i)
    {
        const SAnimDrivenIKDef* adIKDef1 = adIK1->GetAnimDrivenIKDef(i);
        const SAnimDrivenIKDef* adIKDef2 = adIK2->GetAnimDrivenIKDef(i);
        EXPECT_TRUE(adIKDef1->m_handle == adIKDef2->m_handle);
        EXPECT_TRUE(adIKDef1->m_targetJoint == adIKDef2->m_targetJoint);
        EXPECT_TRUE(adIKDef1->m_weightJoint == adIKDef2->m_weightJoint);
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestFootLockIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsFootLockIKDef* footLockIK1 = static_cast<const IChrParamsFootLockIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeFootLock));
    const IChrParamsFootLockIKDef* footLockIK2 = static_cast<const IChrParamsFootLockIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeFootLock));

    EXPECT_TRUE(footLockIK1->GetFootLockIKDefCount() == footLockIK2->GetFootLockIKDefCount());
    uint footLockIKDefCount = footLockIK1->GetFootLockIKDefCount();
    for (uint i = 0; i < footLockIKDefCount; ++i)
    {
        EXPECT_TRUE(footLockIK1->GetFootLockIKDef(i) == footLockIK2->GetFootLockIKDef(i));
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestRecoilIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsRecoilIKDef* recoilIK1 = static_cast<const IChrParamsRecoilIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeRecoil));
    const IChrParamsRecoilIKDef* recoilIK2 = static_cast<const IChrParamsRecoilIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeRecoil));

    EXPECT_TRUE(recoilIK1->GetLeftIKHandle() == recoilIK2->GetLeftIKHandle());
    EXPECT_TRUE(recoilIK1->GetRightIKHandle() == recoilIK2->GetRightIKHandle());
    EXPECT_TRUE(recoilIK1->GetLeftWeaponJoint() == recoilIK2->GetLeftWeaponJoint());
    EXPECT_TRUE(recoilIK1->GetRightWeaponJoint() == recoilIK2->GetRightWeaponJoint());

    //EXPECT_TRUE(recoilIK1->GetImpactJointCount() == recoilIK2->GetImpactJointCount());
    uint impactJointCount = recoilIK1->GetImpactJointCount();
    for (uint i = 0; i < impactJointCount; ++i)
    {
        const SRecoilImpactJoint* impactJoint1 = recoilIK1->GetImpactJoint(i);
        const SRecoilImpactJoint* impactJoint2 = recoilIK2->GetImpactJoint(i);

        EXPECT_TRUE(impactJoint1->m_arm == impactJoint2->m_arm);
        EXPECT_NEAR(impactJoint1->m_delay, impactJoint2->m_delay, 0.01f);
        EXPECT_NEAR(impactJoint1->m_weight, impactJoint2->m_weight, 0.01f);
        EXPECT_TRUE(impactJoint1->m_jointName == impactJoint2->m_jointName);
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestLookIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsLookIKDef* lookIK1 = static_cast<const IChrParamsLookIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeLook));
    const IChrParamsLookIKDef* lookIK2 = static_cast<const IChrParamsLookIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeLook));

    EXPECT_TRUE(lookIK1->GetLeftEyeAttachment() == lookIK2->GetLeftEyeAttachment());
    EXPECT_TRUE(lookIK1->GetRightEyeAttachment() == lookIK2->GetRightEyeAttachment());

    const SEyeLimits* eyeLimits1 = lookIK1->GetEyeLimits();
    const SEyeLimits* eyeLimits2 = lookIK2->GetEyeLimits();
    EXPECT_NEAR(eyeLimits1->m_halfYaw, eyeLimits2->m_halfYaw, 0.01f);
    EXPECT_NEAR(eyeLimits1->m_pitchUp, eyeLimits2->m_pitchUp, 0.01f);
    EXPECT_NEAR(eyeLimits1->m_pitchDown, eyeLimits2->m_pitchDown, 0.01f);

    EXPECT_TRUE(lookIK1->GetRotationCount() == lookIK2->GetRotationCount());
    uint rotCount = lookIK1->GetRotationCount();
    for (uint i = 0; i < rotCount; ++i)
    {
        const SAimLookPosRot* rot1 = lookIK1->GetRotation(i);
        const SAimLookPosRot* rot2 = lookIK2->GetRotation(i);
        EXPECT_TRUE(rot1->m_jointName == rot2->m_jointName);
        EXPECT_TRUE(rot1->m_additive == rot2->m_additive);
        EXPECT_TRUE(rot1->m_primary == rot2->m_primary);
    }

    EXPECT_TRUE(lookIK1->GetPositionCount() == lookIK2->GetPositionCount());
    uint posCount = lookIK1->GetPositionCount();
    for (uint i = 0; i < posCount; ++i)
    {
        const SAimLookPosRot* pos1 = lookIK1->GetPosition(i);
        const SAimLookPosRot* pos2 = lookIK2->GetPosition(i);
        EXPECT_TRUE(pos1->m_jointName == pos2->m_jointName);
        EXPECT_TRUE(pos1->m_additive == pos2->m_additive);
    }

    EXPECT_TRUE(lookIK1->GetDirectionalBlendCount() == lookIK2->GetDirectionalBlendCount());
    uint dirBlendCount = lookIK1->GetDirectionalBlendCount();
    for (uint i = 0; i < dirBlendCount; ++i)
    {
        const SAimLookDirectionalBlend* dirBlend1 = lookIK1->GetDirectionalBlend(i);
        const SAimLookDirectionalBlend* dirBlend2 = lookIK2->GetDirectionalBlend(i);
        EXPECT_TRUE(dirBlend1->m_animToken == dirBlend2->m_animToken);
        EXPECT_TRUE(dirBlend1->m_parameterJoint == dirBlend2->m_parameterJoint);
        EXPECT_TRUE(dirBlend1->m_referenceJoint == dirBlend2->m_referenceJoint);
        EXPECT_TRUE(dirBlend1->m_startJoint == dirBlend2->m_startJoint);
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestAimIKConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsIKDefNode* ikDef1 = static_cast<const IChrParamsIKDefNode*>(test1->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsIKDefNode* ikDef2 = static_cast<const IChrParamsIKDefNode*>(test2->GetCategoryNode(e_chrParamsNodeIKDefinition));
    const IChrParamsAimIKDef* aimIK1 = static_cast<const IChrParamsAimIKDef*>(ikDef1->GetIKSubNode(e_chrParamsIKNodeAim));
    const IChrParamsAimIKDef* aimIK2 = static_cast<const IChrParamsAimIKDef*>(ikDef2->GetIKSubNode(e_chrParamsIKNodeAim));

    EXPECT_TRUE(aimIK1->GetRotationCount() == aimIK2->GetRotationCount());
    uint rotCount = aimIK1->GetRotationCount();
    for (uint i = 0; i < rotCount; ++i)
    {
        const SAimLookPosRot* rot1 = aimIK1->GetRotation(i);
        const SAimLookPosRot* rot2 = aimIK2->GetRotation(i);
        EXPECT_TRUE(rot1->m_jointName == rot2->m_jointName);
        EXPECT_TRUE(rot1->m_additive == rot2->m_additive);
        EXPECT_TRUE(rot1->m_primary == rot2->m_primary);
    }

    EXPECT_TRUE(aimIK1->GetPositionCount() == aimIK2->GetPositionCount());
    uint posCount = aimIK1->GetPositionCount();
    for (uint i = 0; i < posCount; ++i)
    {
        const SAimLookPosRot* pos1 = aimIK1->GetPosition(i);
        const SAimLookPosRot* pos2 = aimIK2->GetPosition(i);
        EXPECT_TRUE(pos1->m_jointName == pos2->m_jointName);
        EXPECT_TRUE(pos1->m_additive == pos2->m_additive);
    }

    EXPECT_TRUE(aimIK1->GetDirectionalBlendCount() == aimIK2->GetDirectionalBlendCount());
    uint dirBlendCount = aimIK1->GetDirectionalBlendCount();
    for (uint i = 0; i < dirBlendCount; ++i)
    {
        const SAimLookDirectionalBlend* dirBlend1 = aimIK1->GetDirectionalBlend(i);
        const SAimLookDirectionalBlend* dirBlend2 = aimIK2->GetDirectionalBlend(i);
        EXPECT_TRUE(dirBlend1->m_animToken == dirBlend2->m_animToken);
        EXPECT_TRUE(dirBlend1->m_parameterJoint == dirBlend2->m_parameterJoint);
        EXPECT_TRUE(dirBlend1->m_referenceJoint == dirBlend2->m_referenceJoint);
        EXPECT_TRUE(dirBlend1->m_startJoint == dirBlend2->m_startJoint);
    }

    EXPECT_TRUE(aimIK1->GetProcAdjustmentCount() == aimIK2->GetProcAdjustmentCount());
    uint procAdjustCount = aimIK1->GetProcAdjustmentCount();
    for (uint i = 0; i < procAdjustCount; ++i)
    {
        EXPECT_TRUE(aimIK1->GetProcAdjustmentJoint(i) == aimIK2->GetProcAdjustmentJoint(i));
    }
}

TEST(CryAnimationParamLoaderFeatureTest, TestAnimationListConsistency)
{
    const IChrParams* test1 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test1Path);
    const IChrParams* test2 = PL_UT_HELPER::GetChrParams(PL_UT_HELPER::test2Path);
    const IChrParamsAnimationListNode* animList1 = static_cast<const IChrParamsAnimationListNode*>(test1->GetCategoryNode(e_chrParamsNodeAnimationList));
    const IChrParamsAnimationListNode* animList2 = static_cast<const IChrParamsAnimationListNode*>(test2->GetCategoryNode(e_chrParamsNodeAnimationList));

    EXPECT_TRUE(animList1->GetAnimationCount() == animList2->GetAnimationCount());
    uint animCount = animList1->GetAnimationCount();
    for (uint i = 0; i < animCount; ++i)
    {
        EXPECT_TRUE(animList1->GetAnimation(i)->m_name == animList2->GetAnimation(i)->m_name);
        EXPECT_TRUE(animList1->GetAnimation(i)->m_path == animList2->GetAnimation(i)->m_path);
        EXPECT_TRUE(animList1->GetAnimation(i)->m_flags == animList2->GetAnimation(i)->m_flags);
        EXPECT_TRUE(animList1->GetAnimation(i)->m_type == animList2->GetAnimation(i)->m_type);
    }
}
#endif // Disabled due to: SEH exception with code 0xc0000005 thrown in the test body.

TEST(CryAnimationParamLoaderFeatureTest, TestChrParamsDelete)
{
    PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().DeleteParsedChrParams(PL_UT_HELPER::subPath);
    bool deleteFailed = PL_UT_HELPER::g_dummyCharacterManager->GetParamLoader().IsChrParamsParsed(PL_UT_HELPER::subPath);
    EXPECT_TRUE(!deleteFailed);
}
