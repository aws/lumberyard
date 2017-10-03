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

#include "SkeletonParameters.h"

namespace CharacterTool
{
    class ExplorerFileList;

    struct SkeletonContent
    {
        SkeletonParameters skeletonParameters;

        AnimationSetFilter includedAnimationSetFilter;
        bool m_filterInValidState;
        string m_errorListString;

        SkeletonContent():
            m_filterInValidState(true)
        {
        }

        void Reset()
        {
            *this = SkeletonContent();
        }

        void UpdateIncludedAnimationSet(ExplorerFileList* skeletonList);
        bool ComposeCompleteAnimationSetFilter(AnimationSetFilter* outFilter, ExplorerFileList* skeletonList) const;

        void GetDependencies(vector<string>* paths) const;
        void Serialize(Serialization::IArchive& ar);
        bool IsValid()
        {
            return m_filterInValidState;
        };
        const char* GetErrorString() const
        {
            return m_errorListString.c_str();
        }
    };
}
