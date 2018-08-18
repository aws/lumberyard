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
#include "PoseAlignerBoneNames.h"

namespace PoseAlignerBones
{
    namespace Biped
    {
        const char* Pelvis = "Bip01 Pelvis";

        const char* LeftHeel = "Bip01 L Heel";
        const char* LeftPlaneTarget = "Bip01 planeTargetLeft";
        const char* LeftPlaneWeight = "Bip01 planeWeightLeft";

        const char* RightHeel = "Bip01 R Heel";
        const char* RightPlaneTarget = "Bip01 planeTargetRight";
        const char* RightPlaneWeight = "Bip01 planeWeightRight";
    }

    namespace PlaneAligner
    {
        const char* Left = "planeAlignerLeft";
        const char* LeftLockWeight = "planeAlignerLeftLockWeight";
        const char* Right = "planeAlignerRight";
        const char* RightLockWeight = "planeAlignerRightLockWeight";
        const char* Center = "planeAlignerCenter";
        const char* CenterLockWeight = "planeAlignerCenterLockWeight";
        const char* BackLeft = "l_bwd_leg_end_joint";
        const char* BackLeftLockWeight = "planeLockWeightBackLeft";
        const char* BackRight = "r_bwd_leg_end_joint";
        const char* BackRightLockWeight = "planeLockWeightBackRight";
        const char* FrontLeft = "l_fwd_leg_end_joint";
        const char* FrontLeftLockWeight = "planeLockWeightFrontLeft";
        const char* FrontRight = "r_fwd_leg_end_joint";
        const char* FrontRightLockWeight = "planeLockWeightFrontRight";
    }

    namespace Deer
    {
        const char* Root = "Def_COG_jnt";

        const char* LeftToeEnd = "Def_L_ToeEnd_jnt";
        const char* LeftWristEnd = "Def_L_WristEnd_jnt";

        const char* RightToeEnd = "Def_R_ToeEnd_jnt";
        const char* RightWristEnd = "Def_R_WristEnd_jnt";
    }
}