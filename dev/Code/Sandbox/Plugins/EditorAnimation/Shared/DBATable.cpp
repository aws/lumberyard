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
#include <ICryPak.h>
#include <CryPath.h>
#include "DBATable.h"
#include "Serialization.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"
#include <IXml.h>

void SDBAEntry::Serialize(IArchive& ar)
{
    if (ar.IsEdit() && ar.IsOutput())
    {
        ar(path, "digest", "!^");
    }
    ar(OutputFilePath(path, "Animation Databases (.dba)|*.dba", "Animations"), "path", "<Path");
    filter.Serialize(ar);
}

void SDBATable::Serialize(IArchive& ar)
{
    ar(entries, "entries", "Entries");
}

bool SDBATable::ImportOldXML(const XmlNodeRef& xmlRoot)
{
    if (!xmlRoot)
    {
        return false;
    }

    XmlNodeRef dbas = xmlRoot;

    int childCount = dbas->getChildCount();
    for (int i = 0; i < childCount; ++i)
    {
        XmlNodeRef dbaNode = dbas->getChild(i);
        string path = dbaNode->getAttr("Path");
        if (path.empty())
        {
            continue;
        }

        int numAnimations = dbaNode->getChildCount();
        if (numAnimations == 0)
        {
            continue;
        }

        string commonPath = dbaNode->getChild(0)->getAttr("Path");
        string::size_type pos = commonPath.rfind('/');
        if (pos == string::npos)
        {
            pos = commonPath.rfind('\\');
        }
        if (pos != string::npos)
        {
            commonPath.erase(pos + 1, commonPath.size());
        }

        if (!commonPath.empty())
        {
            for (int j = 1; j < numAnimations; ++j)
            {
                XmlNodeRef animationNode = dbaNode->getChild(j);
                string animationPath = animationNode->getAttr("Path");
                if (animationPath.empty())
                {
                    continue;
                }

                while (!commonPath.empty() && strnicmp(animationPath.c_str(), commonPath.c_str(), commonPath.size()) != 0)
                {
                    if (commonPath[commonPath.size() - 1] == '/' || commonPath[commonPath.size() - 1] == '\\')
                    {
                        commonPath.erase(commonPath.size() - 1);
                    }
                    string::size_type pos = commonPath.rfind('/');
                    if (pos == string::npos)
                    {
                        pos = commonPath.rfind('\\');
                    }
                    if (pos != string::npos)
                    {
                        commonPath.erase(pos + 1, commonPath.size());
                    }
                    else
                    {
                        commonPath.clear();
                    }
                }
            }
        }

        SDBAEntry entry;
        entry.path = PathUtil::ToUnixPath(path);
        if (!commonPath.empty())
        {
            entry.filter.SetInFolderCondition(PathUtil::ToUnixPath(commonPath).c_str());
        }
        entries.push_back(entry);
    }
    return true;
}

bool SDBATable::Load(const char* fullPath)
{
    Serialization::JSONIArchive ia;
    if (!ia.load(fullPath))
    {
        return false;
    }

    ia(*this);

    return true;
}

bool SDBATable::Save(const char* fullPath)
{
    Serialization::JSONOArchive oa(120);
    oa(*this);

    oa.save(fullPath);
    return true;
}

int SDBATable::FindDBAForAnimation(const SAnimationFilterItem& animation) const
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (entries[i].filter.MatchesFilter(animation))
        {
            return int(i);
        }
    }

    return -1;
}
