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

#include "pch.h"
#include "AnimationTagList.h"
#include "EditorCompressionPresetTable.h"
#include "EditorDBATable.h"

namespace CharacterTool
{
    enum ETagGroup
    {
        TAG_GROUP_COMPRESSION_PRESETS,
        TAG_GROUP_DBAS,
        NUM_TAG_GROUPS
    };

    AnimationTagList::AnimationTagList(EditorCompressionPresetTable* presets, EditorDBATable* dbaTable)
        : m_presets(presets)
        , m_dbaTable(dbaTable)
    {
        m_groups.resize(NUM_TAG_GROUPS);
        m_groups[TAG_GROUP_COMPRESSION_PRESETS].name = "Compression Preset";
        m_groups[TAG_GROUP_DBAS].name = "DBA";

        connect(m_presets, SIGNAL(SignalChanged()), this, SLOT(OnCompressionPresetsChanged()));
        connect(m_dbaTable, SIGNAL(SignalChanged()), this, SLOT(OnDBATableChanged()));

        OnCompressionPresetsChanged();
        OnDBATableChanged();
    }

    void AnimationTagList::OnCompressionPresetsChanged()
    {
        std::vector<std::pair<string, string> > tags;
        m_presets->FindTags(&tags);

        SGroup& group = m_groups[TAG_GROUP_COMPRESSION_PRESETS];
        group.entries.resize(tags.size());
        for (size_t i = 0; i < tags.size(); ++i)
        {
            group.entries[i].tag = tags[i].first;
            group.entries[i].description = tags[i].second;
        }
    }

    void AnimationTagList::OnDBATableChanged()
    {
        std::vector<std::pair<string, string> > tags;
        m_dbaTable->FindTags(&tags);

        SGroup& group = m_groups[TAG_GROUP_DBAS];
        group.entries.resize(tags.size());
        for (size_t i = 0; i < tags.size(); ++i)
        {
            group.entries[i].tag = tags[i].first;
            group.entries[i].description = tags[i].second;
        }
    }

    unsigned int AnimationTagList::TagCount(unsigned int group) const
    {
        if (group >= m_groups.size())
        {
            return 0;
        }
        return m_groups[group].entries.size();
    }

    const char* AnimationTagList::TagValue(unsigned int group, unsigned int index) const
    {
        if (group >= m_groups.size())
        {
            return 0;
        }
        const SGroup& g = m_groups[group];
        if (index >= g.entries.size())
        {
            return "";
        }
        return g.entries[index].tag.c_str();
    }

    const char* AnimationTagList::TagDescription(unsigned int group, unsigned int index) const
    {
        if (group >= m_groups.size())
        {
            return 0;
        }
        const SGroup& g = m_groups[group];
        if (index >= g.entries.size())
        {
            return "";
        }
        return g.entries[index].description.c_str();
    }

    const char* AnimationTagList::GroupName(unsigned int group) const
    {
        if (group >= m_groups.size())
        {
            return "";
        }
        return m_groups[group].name.c_str();
    }

    unsigned int AnimationTagList::GroupCount() const
    {
        return m_groups.size();
    }
}

#include <CharacterTool/AnimationTagList.moc>
