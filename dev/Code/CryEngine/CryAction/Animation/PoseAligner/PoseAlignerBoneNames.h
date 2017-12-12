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
#ifndef CRYINCLUDE_CRYANIMATION_POSEALIGNER_POSEALIGNERBONENAMES_H
#define CRYINCLUDE_CRYANIMATION_POSEALIGNER_POSEALIGNERBONENAMES_H
#pragma once


namespace PoseAlignerBones
{
    namespace Biped
    {
        extern const char* Pelvis;

        extern const char* LeftHeel;
        extern const char* LeftPlaneTarget;
        extern const char* LeftPlaneWeight;

        extern const char* RightHeel;
        extern const char* RightPlaneTarget;
        extern const char* RightPlaneWeight;
    }

    namespace PlaneAligner
    {
        extern const char* Left;
        extern const char* LeftLockWeight;
        extern const char* Right;
        extern const char* RightLockWeight;
        extern const char* Center;
        extern const char* CenterLockWeight;
        extern const char* BackLeft;
        extern const char* BackLeftLockWeight;
        extern const char* BackRight;
        extern const char* BackRightLockWeight;
        extern const char* FrontLeft;
        extern const char* FrontLeftLockWeight;
        extern const char* FrontRight;
        extern const char* FrontRightLockWeight;
    }

    namespace Deer
    {
        extern const char* Root;

        extern const char* LeftToeEnd;
        extern const char* LeftWristEnd;

        extern const char* RightToeEnd;
        extern const char* RightWristEnd;
    }
}

#endif // CRYINCLUDE_CRYANIMATION_POSEALIGNER_POSEALIGNERBONENAMES_H