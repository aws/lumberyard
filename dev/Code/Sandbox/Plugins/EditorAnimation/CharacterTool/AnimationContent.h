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

#include <Cry_Math.h>
#include "AnimEvent.h"
#include "Shared/AnimSettings.h"
#include "BlendSpace.h"


struct IDefaultSkeleton;
struct IAnimationSet;

namespace CharacterTool {
    struct System;
    struct AnimationContent
    {
        enum Type
        {
            ANIMATION,
            BLEND_SPACE,
            COMBINED_BLEND_SPACE,
            AIMPOSE,
            LOOKPOSE,
            ANM
        };

        enum EImportState
        {
            NOT_SET,
            NEW_ANIMATION,
            WAITING_FOR_CHRPARAMS_RELOAD,
            IMPORTED,
            COMPILED_BUT_NO_ANIMSETTINGS
        };

        Type type;

        EImportState importState;
        int size;
        bool loadedInEngine;
        bool loadedAsAdditive;
        bool delayApplyUntilStart;
        int animationId;
        SAnimSettings settings;
        BlendSpace blendSpace;
        CombinedBlendSpace combinedBlendSpace;
        string newAnimationSkeleton;
        std::vector<AnimEvent> events;
        System* system;

        AnimationContent();

        void ApplyToCharacter(bool* triggerPreview, ICharacterInstance* characterInstance, const char* animationPath, bool animationStarting);
        void ApplyAfterStart(ICharacterInstance* characterInstance, const char* animationPath);
        void UpdateBlendSpaceMotionParameters(IAnimationSet* animationSet, IDefaultSkeleton* defaultSkeleton);

        void Reset();
        void Serialize(Serialization::IArchive& ar);

        bool HasAudioEvents() const;
    };
}
