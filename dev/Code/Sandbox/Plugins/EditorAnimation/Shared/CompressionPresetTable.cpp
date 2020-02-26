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

#include "StdAfx.h"
#include <platform.h>
#include "CompressionPresetTable.h"
#include "AnimationFilter.h"
#include "DBATable.h"
#include "Serialization.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"

void SCompressionPresetEntry::Serialize(Serialization::IArchive& ar)
{
    if (ar.IsEdit() && ar.IsOutput())
    {
        ar(name, "digest", "!^");
    }
    ar(name, "name", "Name");
    filter.Serialize(ar);
    if (!filter.condition)
    {
        ar.Warning(filter.condition, "Specify filter to apply preset to animations.");
    }
    ar(settings, "settings", "+Settings");
}


void SCompressionPresetTable::Serialize(Serialization::IArchive& ar)
{
    ar(entries, "entries", "Entries");
}

bool SCompressionPresetTable::Load(const char* fullPath)
{
    Serialization::JSONIArchive ia;
    if (!ia.load(fullPath))
    {
        return false;
    }

    ia(*this);
    return true;
}

bool SCompressionPresetTable::Save(const char* fullPath)
{
    Serialization::JSONOArchive oa(120);
    oa(*this);

    return oa.save(fullPath);
}

const SCompressionPresetEntry* SCompressionPresetTable::FindPresetForAnimation(const char* animationPath, const vector<string>& tags, const char* skeletonAlias) const
{
    SAnimationFilterItem item;
    item.path = animationPath;
    item.tags = tags;
    item.skeletonAlias = skeletonAlias;

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const SCompressionPresetEntry& entry = entries[i];
        if (entry.filter.MatchesFilter(item))
        {
            return &entry;
        }
    }

    return 0;
}
