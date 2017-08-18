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
#include "EditorDBATable.h"
#include <ICryPak.h>
#include "Serialization.h"
#include "Serialization/BinArchive.h"
#include "../Shared/AnimSettings.h"
#include "FilterAnimationList.h"
#include <Util/PathUtil.h> // for game folder

namespace CharacterTool
{
    void SEditorDBAEntry::Serialize(IArchive& ar)
    {
        entry.Serialize(ar);

        if (ar.IsEdit() && ar.IsOutput())
        {
            ar(matchingAnimations, "matchingAnimations", "!Matching Local Animations");
        }
    }


    bool EditorDBATable::Load()
    {
        string dbaTablePath = (Path::GetEditingGameDataFolder() + "\\Animations\\DBATable.json").c_str();

        SDBATable table;
        if (!table.Load(dbaTablePath.c_str()))
        {
            const char* oldTablePath = "Animations\\DBATable.xml";
            if (gEnv->pCryPak->IsFileExist(oldTablePath))
            {
                if (!table.ImportOldXML(GetISystem()->LoadXmlFromFile(oldTablePath)))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        m_entries.clear();
        m_entries.resize(table.entries.size());
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].entry = table.entries[i];
        }

        UpdateMatchingAnimations();

        return true;
    }

    void EditorDBATable::UpdateMatchingAnimations()
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
            int dbaIndex = FindDBAForAnimation(filterItems[i]);
            if (dbaIndex == -1)
            {
                continue;
            }
            m_entries[dbaIndex].matchingAnimations.push_back(filterItems[i].path);
        }
    }

    bool EditorDBATable::Save()
    {
        string dbaTablePath = (Path::GetEditingGameDataFolder() + "\\Animations\\DBATable.json").c_str();

        SDBATable table;
        table.entries.resize(m_entries.size());
        for (size_t i = 0; i < table.entries.size(); ++i)
        {
            table.entries[i] = m_entries[i].entry;
        }

        return table.Save(dbaTablePath.c_str());
    }

    void EditorDBATable::Serialize(IArchive& ar)
    {
        ar(m_entries, "entries", "Entries");

        if (ar.IsInput())
        {
            UpdateMatchingAnimations();
            SignalChanged();
        }
    }

    int EditorDBATable::FindDBAForAnimation(const SAnimationFilterItem& animation) const
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            const SEditorDBAEntry& entry = m_entries[i];
            if (entry.entry.filter.MatchesFilter(animation))
            {
                return int(i);
            }
        }

        return -1;
    }

    const SEditorDBAEntry* EditorDBATable::GetEntryByIndex(int index) const
    {
        if (size_t(index) >= m_entries.size())
        {
            return 0;
        }
        return &m_entries[index];
    }

    void EditorDBATable::FindTags(std::vector<std::pair<string, string> >* tags) const
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
                stl::push_back_unique(presets, PathUtil::GetFile(m_entries[i].entry.path.c_str()));
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

    void EditorDBATable::Reset()
    {
        m_entries.clear();
    }
}

#include <CharacterTool/EditorDBATable.moc>
