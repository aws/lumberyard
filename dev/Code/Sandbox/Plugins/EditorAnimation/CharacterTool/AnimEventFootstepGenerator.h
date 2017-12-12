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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "AnimEvent.h"

struct ICharacterInstance;

namespace CharacterTool
{
    struct FootstepGenerationParameters
    {
        bool generateFoleys;
        int foleyDelayFrames;
        int shuffleFoleyDelayFrames;
        float footHeightMM;
        float footShuffleUpperLimitMM;
        string leftFootJoint;
        string rightFootJoint;
        AnimEvent leftFootEvent;
        AnimEvent rightFootEvent;
        AnimEvent leftFoleyEvent;
        AnimEvent rightFoleyEvent;
        AnimEvent leftShuffleEvent;
        AnimEvent rightShuffleEvent;
        AnimEvent leftShuffleFoleyEvent;
        AnimEvent rightShuffleFoleyEvent;

        void Serialize(Serialization::IArchive& ar);

        FootstepGenerationParameters()
            : foleyDelayFrames(3)
            , shuffleFoleyDelayFrames(3)
            , footHeightMM(200.0f)
            , footShuffleUpperLimitMM(275.0f)
            , generateFoleys(true)
        {
            SetLeftFootJoint("Bip01 L Foot");
            SetRightFootJoint("Bip01 R Foot");

            leftFootEvent.startTime = -1.0f;
            leftFootEvent.endTime = -1.0f;
            leftFootEvent.type = "footstep";
            rightFootEvent.startTime = -1.0f;
            rightFootEvent.endTime = -1.0f;
            rightFootEvent.type = "footstep";

            leftFoleyEvent.startTime = -1.0f;
            leftFoleyEvent.endTime = -1.0f;
            leftFoleyEvent.type = "foley";
            rightFoleyEvent.startTime = -1.0f;
            rightFoleyEvent.endTime = -1.0f;
            rightFoleyEvent.type = "foley";

            leftShuffleEvent.startTime = -1.0f;
            leftShuffleEvent.endTime = -1.0f;
            leftShuffleEvent.type = "foley";
            rightShuffleEvent.startTime = -1.0f;
            rightShuffleEvent.endTime = -1.0f;
            rightShuffleEvent.type = "foley";

            leftShuffleFoleyEvent.startTime = -1.0f;
            leftShuffleFoleyEvent.endTime = -1.0f;
            leftShuffleFoleyEvent.type = "foley";
            rightShuffleFoleyEvent.startTime = -1.0f;
            rightShuffleFoleyEvent.endTime = -1.0f;
            rightShuffleFoleyEvent.type = "foley";
        }

        void SetLeftFootJoint(const char* jointName);
        void SetRightFootJoint(const char* jointName);
    };

    struct AnimationContent;
    bool GenerateFootsteps(AnimationContent* content, string* errorMessage, ICharacterInstance* character, const char* animationName, const FootstepGenerationParameters& params);
}
