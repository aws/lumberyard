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
#include <IEditor.h>
#include <IEditorFileMonitor.h>
#include "FilterAnimationList.h"
#include "../Shared/AnimSettings.h"
#include "Util/PathUtil.h"

namespace CharacterTool
{
    FilterAnimationList::FilterAnimationList()
        : m_revision(0)
    {
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "animsettings");
    }

    FilterAnimationList::~FilterAnimationList()
    {
        GetIEditor()->GetFileMonitor()->UnregisterListener(this);
    }

    void FilterAnimationList::Populate()
    {
        m_items.clear();

        std::vector< string > filenames;

        SDirectoryEnumeratorHelper dirHelper;
        dirHelper.ScanDirectoryRecursive(Path::GetEditingGameDataFolder().c_str(), "", "*.animsettings", filenames);

        for (size_t i = 0; i < filenames.size(); ++i)
        {
            const char* filename = filenames[i];
            SAnimSettings settings;

            string animSettingsFilename = SAnimSettings::GetAnimSettingsFilename(filename);
            if (settings.Load(animSettingsFilename.c_str(), std::vector<string>(), 0, 0))
            {
                SAnimationFilterItem filterItem;
                filterItem.tags = settings.build.tags;
                filterItem.skeletonAlias = settings.build.skeletonAlias;
                filterItem.path = PathUtil::ReplaceExtension(PathUtil::ToUnixPath(filename), "caf");
                m_items.push_back(filterItem);
            }
        }
    }

    void FilterAnimationList::UpdateItem(const char* filename)
    {
        string animationPath = PathUtil::ReplaceExtension(PathUtil::ToUnixPath(filename), "caf");
        int numItems = m_items.size();
        bool updated = false;
        for (int i = 0; i < numItems; ++i)
        {
            if (azstricmp(m_items[i].path.c_str(), animationPath) == 0)
            {
                SAnimationFilterItem& filterItem = m_items[i];
                SAnimSettings settings;
                string animSettingsFilename = SAnimSettings::GetAnimSettingsFilename(filename);
                if (settings.Load(animSettingsFilename.c_str(), std::vector<string>(), 0, 0))
                {
                    filterItem.tags = settings.build.tags;
                }
                else
                {
                    i -= RemoveItem(filename);
                }
                updated = true;
            }
        }

        if (!updated)
        {
            SAnimSettings settings;
            string animSettingsFilename = SAnimSettings::GetAnimSettingsFilename(filename);
            if (settings.Load(animSettingsFilename.c_str(), std::vector<string>(), 0, 0))
            {
                SAnimationFilterItem filterItem;
                filterItem.tags = settings.build.tags;
                filterItem.path = animationPath;
                m_items.push_back(filterItem);
            }
        }
        ++m_revision;
    }

    int FilterAnimationList::RemoveItem(const char* _filename)
    {
        string filename = PathUtil::ReplaceExtension(PathUtil::ToUnixPath(_filename), "caf");

        int itemsRemoved = 0;

        int numItems = m_items.size();
        for (int i = 0; i < numItems; ++i)
        {
            if (azstricmp(m_items[i].path.c_str(), filename) == 0)
            {
                m_items.erase(m_items.begin() + i);
                --numItems;
                ++itemsRemoved;
                --i;
            }
        }
        ++m_revision;
        return itemsRemoved;
    }

    void FilterAnimationList::OnFileChange(const char* filename, EChangeType eType)
    {
        const string originalExt = PathUtil::GetExt(filename);
        if (originalExt == "animsettings")
        {
            if (gEnv->pCryPak->IsFileExist(filename, ICryPak::eFileLocation_OnDisk))
            {
                UpdateItem(filename);
            }
            else
            {
                RemoveItem(filename);
            }
        }
    }
}