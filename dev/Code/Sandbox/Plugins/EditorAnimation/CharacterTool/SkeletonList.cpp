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
#include "SkeletonList.h"
#include "Serialization.h"
#include <IXml.h>
#include <ISystem.h>
#include <IEditor.h>
#include <IEditorFileMonitor.h>
#include <CryFile.h> // for PathUtil
#include "../Shared/AnimSettings.h"

#include <Util/PathUtil.h> // for getting the game folder

namespace CharacterTool
{
    static const char* ANIMATIONS_SKELETON_LIST_PATH = "Animations/SkeletonList.xml";

    SkeletonList::SkeletonList()
    {
        OutputDebugString("CSkeletonList created\n");
    }

    SkeletonList::~SkeletonList()
    {
        OutputDebugString("CSkeletonList destroyed\n");
    }

    void SkeletonList::OnFileChange(const char* filename, EChangeType eType)
    {
        Load();
    }

    void SkeletonList::Reset()
    {
        m_skeletonList.clear();
    }

    bool SkeletonList::Load()
    {
        std::set<string> loadedAliases;

        XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromFile(Path::GamePathToFullPath(ANIMATIONS_SKELETON_LIST_PATH).toUtf8().data());
        if (!xmlRoot)
        {
            return false;
        }

        m_skeletonList.clear();
        m_skeletonNames.clear();
        m_skeletonNames.push_back(string()); // service name that we use for UI

        const int childCount = xmlRoot->getChildCount();
        for (int i = 0; i < childCount; ++i)
        {
            XmlNodeRef xmlEntry = xmlRoot->getChild(i);

            const char* name = xmlEntry->getAttr("name");
            ;
            const char* file = xmlEntry->getAttr("file");
            if (file[0] != '\0' && name[0] != '\0')
            {
                const bool foundSkeletonName = loadedAliases.find(name) != loadedAliases.end();
                if (foundSkeletonName)
                {
                    CryLog("Found duplicate skeleton name '%s' in file '%s' while parsing line '%d'. Skipping this entry.", name, ANIMATIONS_SKELETON_LIST_PATH, xmlEntry->getLine());
                }
                else
                {
                    SEntry entry;
                    entry.alias = name;
                    entry.path = file;
                    m_skeletonList.push_back(entry);
                    m_skeletonNames.push_back(name);
                }
            }
            else
            {
                CryLog("Missing 'name' or 'file' attribute in skeleton entry in file '%s' while parsing line '%d'. Skipping this entry.", ANIMATIONS_SKELETON_LIST_PATH, xmlEntry->getLine());
            }
        }

        std::sort(m_skeletonNames.begin(), m_skeletonNames.end());
        SignalSkeletonListModified();
        return true;
    }

    bool SkeletonList::Save()
    {
        XmlNodeRef xmlRoot = GetISystem()->CreateXmlNode("SkeletonList");

        for (size_t i = 0; i < m_skeletonList.size(); ++i)
        {
            const SEntry& entry = m_skeletonList[i];
            if (!entry.alias.empty() && !entry.path.empty())
            {
                XmlNodeRef xmlEntry = xmlRoot->createNode("Skeleton");
                xmlRoot->addChild(xmlEntry);

                xmlEntry->setAttr("name", entry.alias.c_str());
                xmlEntry->setAttr("file", entry.path.c_str());
            }
        }

        const bool saveSuccess = xmlRoot->saveToFile(Path::GamePathToFullPath(ANIMATIONS_SKELETON_LIST_PATH).toUtf8().data());
        return saveSuccess;
    }

    bool SkeletonList::HasSkeletonName(const char* skeletonName) const
    {
        for (size_t i = 0; i < m_skeletonList.size(); ++i)
        {
            if (m_skeletonList[i].alias == skeletonName)
            {
                return true;
            }
        }
        return false;
    }

    string SkeletonList::FindSkeletonNameByPath(const char* path) const
    {
        for (size_t i = 0; i < m_skeletonList.size(); ++i)
        {
            const SEntry& entry = m_skeletonList[i];
            if (stricmp(entry.path.c_str(), path) == 0)
            {
                return entry.alias;
            }
        }

        return string();
    }

    string SkeletonList::FindSkeletonPathByName(const char* name) const
    {
        for (size_t i = 0; i < m_skeletonList.size(); ++i)
        {
            const SEntry& entry = m_skeletonList[i];
            if (stricmp(entry.alias.c_str(), name) == 0)
            {
                return entry.path;
            }
        }

        return string();
    }


    void SkeletonList::Serialize(Serialization::IArchive& ar)
    {
        ar(m_skeletonList, "aliases", "Aliases");

        if (ar.IsInput())
        {
            m_skeletonNames.clear();
            m_skeletonNames.push_back(string());
            for (size_t i = 0; i < m_skeletonList.size(); ++i)
            {
                m_skeletonNames.push_back(m_skeletonList[i].alias);
            }
            std::sort(m_skeletonNames.begin() + 1, m_skeletonNames.end());
        }
    }

    void SkeletonList::SEntry::Serialize(Serialization::IArchive& ar)
    {
        ar(alias, "alias", "^");
        ar(ResourceFilePath(path, "Skeleton"), "path", "^");
        if (ar.IsEdit() && ar.IsInput() && alias.empty() && !path.empty())
        {
            alias = PathUtil::GetFileName(path);
        }
    }
}

// ---------------------------------------------------------------------------

bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label)
{
    using CharacterTool::SkeletonList;
    if (ar.IsEdit())
    {
        SkeletonList* skeletonList = ar.FindContext<SkeletonList>();
        if (skeletonList)
        {
            const Serialization::StringList& skeletons = skeletonList->GetSkeletonNames();
            int index = skeletons.find(value.alias.c_str());
            if (index == Serialization::StringList::npos)
            {
                return false;
            }

            Serialization::StringListValue stringListValue(skeletons, index, &value.alias, Serialization::TypeID::get<string>());
            if (!ar(stringListValue, name, label))
            {
                return false;
            }

            if (ar.IsInput())
            {
                value.alias = stringListValue.c_str();
            }
            return true;
        }
        else
        {
            return ar(value.alias, name, label);
        }
    }
    else
    {
        return ar(value.alias, name, label);
    }
}

#include <CharacterTool/SkeletonList.moc>
