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
#include <ICryPak.h>
#include <ICryAnimation.h>
#include "EditorDBATable.h"
#include "Serialization.h"
#include <Util/PathUtil.h> // to get game folder
#include "../Shared/AnimSettings.h"
#include "EditorCompressionPresetTable.h"
#include "FilterAnimationList.h"

namespace CharacterTool
{
    void EditorCompressionPreset::Serialize(Serialization::IArchive& ar)
    {
        entry.Serialize(ar);

        if (ar.IsEdit() && ar.IsOutput())
        {
            ar(matchingAnimations, "matchingAnimations", "!Matching Local Animations");
        }
    }

    bool EditorCompressionPresetTable::Save()
    {
        AZStd::string dbaTablePath = Path::GetEditingGameDataFolder() + "\\Animations\\CompressionPresets.json";

        SCompressionPresetTable table;
        table.entries.resize(m_entries.size());
        for (size_t i = 0; i < table.entries.size(); ++i)
        {
            table.entries[i] = m_entries[i].entry;
        }

        return table.Save(dbaTablePath.c_str());
    }

    bool EditorCompressionPresetTable::Load()
    {
        AZStd::string dbaTablePath = Path::GetEditingGameDataFolder() + "\\Animations\\CompressionPresets.json";

        SCompressionPresetTable table;
        if (!table.Load(dbaTablePath.c_str()))
        {
            return false;
        }

        m_entries.clear();
        m_entries.resize(table.entries.size());
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].entry = table.entries[i];
        }

        UpdateMatchingAnimations();

        SignalChanged();
        return true;
    }

    EditorCompressionPreset* EditorCompressionPresetTable::FindPresetForAnimation(const SAnimationFilterItem& item)
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].entry.filter.MatchesFilter(item))
            {
                return &m_entries[i];
            }
        }

        return 0;
    }

    void EditorCompressionPresetTable::UpdateMatchingAnimations()
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].matchingAnimations.clear();
        }
        if (!m_filterAnimationList)
        {
            return;
        }

        const SAnimationFilterItems& filterItems = m_filterAnimationList->Animations();
        for (size_t i = 0; i < filterItems.size(); ++i)
        {
            EditorCompressionPreset* preset = FindPresetForAnimation(filterItems[i]);
            if (!preset)
            {
                continue;
            }
            preset->matchingAnimations.push_back(filterItems[i].path);
        }
    }

    void EditorCompressionPresetTable::Serialize(IArchive& ar)
    {
        ar(m_entries, "entries", "Entries");

        if (ar.IsEdit() && ar.IsInput())
        {
            UpdateMatchingAnimations();
            SignalChanged();
        }
    }

    void EditorCompressionPresetTable::FindTags(std::vector<std::pair<string, string> >* tags) const
    {
        typedef std::vector<string> Presets;
        typedef std::map<string, Presets> TagToPresets;
        TagToPresets tagMap;

        std::vector<string> entryTags;
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].entry.filter.FindTags(&entryTags);
            if (entryTags.empty())
            {
                continue;
            }
            for (size_t j = 0; j < entryTags.size(); ++j)
            {
                Presets& presets = tagMap[entryTags[j]];
                stl::push_back_unique(presets, m_entries[i].entry.name);
            }
        }

        tags->clear();
        string description;
        for (TagToPresets::iterator it = tagMap.begin(); it != tagMap.end(); ++it)
        {
            description.clear();
            const Presets& presets = it->second;
            size_t presetsToShow = presets.size();
            if (presetsToShow > 3)
            {
                presetsToShow = 3;
            }
            for (size_t i = 0; i < presetsToShow; ++i)
            {
                if (i)
                {
                    description += ", ";
                }
                description += presets[i];
            }
            if (presets.size() > 3)
            {
                description += ", ...";
            }

            tags->push_back(std::make_pair(it->first, description));
        }
    }

    void EditorCompressionPresetTable::Reset()
    {
        m_entries.clear();
    }
}

#include <CharacterTool/EditorCompressionPresetTable.moc>
