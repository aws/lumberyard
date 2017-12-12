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
#include "EntryList.h"
#include "EntryListImpl.h"
#include <ICryPak.h>

namespace CharacterTool
{
    static bool operator!=(const MemoryOArchive& oa, const vector<char>& buffer)
    {
        if (oa.length() != buffer.size())
        {
            return true;
        }
        return memcmp(oa.buffer(), &buffer[0], buffer.size()) != 0;
    }

    void EntryBase::StoreSavedContent()
    {
        MemoryOArchive oa;
        Serialize(oa);
        savedContent.assign(oa.buffer(), oa.buffer() + oa.length());
    }

    // ---------------------------------------------------------------------------

    const char* GetFilename(const char* path);

    void EntryListBase::Clear()
    {
        m_entries.clear();
        m_entriesById.clear();
        m_entriesByPath.clear();
        m_entriesByName.clear();
    }

    void EntryListBase::AddEntry(EntryBase* entry)
    {
        entry->id = MakeNextId();
        if (entry->name.empty())
        {
            entry->name = GetFilename(entry->path);
        }

        m_entries.push_back(unique_ptr<EntryBase>(entry));
        m_entriesById[entry->id] = entry;
        m_entriesByPath[entry->path] = entry;
        m_entriesByName[entry->name] = entry;
    }

    EntryBase* EntryListBase::GetBaseByIndex(size_t index) const
    {
        if (index >= m_entries.size())
        {
            return 0;
        }
        return m_entries[index].get();
    }

    EntryBase* EntryListBase::GetBaseById(EntryId id) const
    {
        EntriesById::const_iterator it = m_entriesById.find(id);
        if (it == m_entriesById.end())
        {
            return 0;
        }
        return it->second;
    }

    EntryBase* EntryListBase::GetBaseByName(const char* name) const
    {
        EntriesByPath::const_iterator it = m_entriesByName.find(name);
        if (it == m_entriesByName.end())
        {
            return 0;
        }
        return it->second;
    }

    EntryBase* EntryListBase::GetBaseByPath(const char* path) const
    {
        EntriesByPath::const_iterator it = m_entriesByPath.find(path);
        if (it == m_entriesByPath.end())
        {
            return 0;
        }
        return it->second;
    }


    bool EntryListBase::EntryReverted(vector<char>* previousContent, EntryBase* entry) const
    {
        bool wasModified = entry->modified;
        if (entry->savedContent != entry->lastContent)
        {
            wasModified = true;
            if (previousContent)
            {
                previousContent->clear();
                previousContent->swap(entry->lastContent);
            }
            entry->lastContent = entry->savedContent;
        }
        entry->modified = false;
        return wasModified;
    }

    bool EntryListBase::EntryChanged(vector<char>* previousContent, EntryBase* entry) const
    {
        MemoryOArchive content;
        entry->Serialize(content);

        if (content != entry->lastContent)
        {
            previousContent->clear();
            previousContent->swap(entry->lastContent);
            entry->lastContent.assign(content.buffer(), content.buffer() + content.length());
            entry->modified = content != entry->savedContent;
            return true;
        }
        return false;
    }

    bool EntryListBase::EntrySaved(EntryBase* entry) const
    {
        entry->StoreSavedContent();
        entry->lastContent = entry->savedContent;
        bool wasModified = entry->modified;
        entry->modified = false;
        entry->pendingSave = false;
        return wasModified;
    }

    EntryId EntryListBase::MakeNextId()
    {
        if (m_idProvider)
        {
            return m_idProvider->MakeNextId();
        }
        else
        {
            return m_nextId++;
        }
    }

    bool EntryListBase::RemoveById(EntryId id)
    {
        EntryBase* entry = GetBaseById(id);
        if (!entry)
        {
            return false;
        }

        m_entriesById.erase(entry->id);
        m_entriesByPath.erase(entry->path);
        m_entriesByName.erase(entry->name);
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].get() == entry)
            {
                m_entries.erase(m_entries.begin() + i);
                --i;
            }
        }
        return true;
    }

    unsigned int EntryListBase::RemoveByPath(const char* path)
    {
        unsigned int id = 0;

        EntryBase* entry = GetBaseByPath(path);
        if (!entry)
        {
            return false;
        }

        m_entriesById.erase(entry->id);
        m_entriesByPath.erase(entry->path);
        m_entriesByName.erase(entry->name);
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].get() == entry)
            {
                id = entry->id;
                m_entries.erase(m_entries.begin() + i);
                --i;
            }
        }
        return id;
    }
}
