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

#include "AnimationFilter.h"
#include "AnimSettings.h"

struct SCompressionPresetEntry
{
    string name;
    SAnimationFilter filter;
    SCompressionSettings settings;

    void Serialize(Serialization::IArchive& ar);
};


struct SCompressionPresetTable
{
    std::vector<SCompressionPresetEntry> entries;

    const SCompressionPresetEntry* FindPresetForAnimation(const char* animationPath, const vector<string>& tags, const char* skeletonAlias) const;

    bool Load(const char* tablePath);
    bool Save(const char* tablePath);
    void Serialize(Serialization::IArchive& ar);
};
