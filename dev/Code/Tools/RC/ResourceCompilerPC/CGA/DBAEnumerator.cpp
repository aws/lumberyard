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

#include "stdafx.h"
#include "DBAEnumerator.h"
#include "DBAManager.h"
#include "SkeletonManager.h"
#include "AnimationCompiler.h"
#include "IPakSystem.h"
#include "../../../CryXML/ICryXML.h"
#include <StringHelpers.h>

void FindCAFsForDefinition(std::vector<CAFFileItem>* items, const string& sourceFolder, const SAnimationDefinition& definition, const CAnimationInfoLoader& loader);

DBAManagerEnumerator::DBAManagerEnumerator(DBAManager* dbaManager, const string& sourceRoot)
    : m_dbaManager(dbaManager)
    , m_sourceRoot(sourceRoot)
{
}

int DBAManagerEnumerator::GetDBACount() const
{
    return m_dbaManager->GetDBACount();
}

void DBAManagerEnumerator::GetDBA(EnumeratedDBA* dba, int index) const
{
    const DBAManagerEntry& entry = m_dbaManager->GetDBAEntry(index);
    dba->innerPath = entry.GetPath();
    dba->animationCount = entry.GetAnimationCount();
}

bool DBAManagerEnumerator::GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const
{
    const DBAManagerEntry& entry = m_dbaManager->GetDBAEntry(dbaIndex);
    caf->path = entry.GetAnimationPath(animationIndex);
    caf->skipDBA = false;
    return true;
}

// ---------------------------------------------------------------------------

CBAEnumerator::CBAEnumerator(CAnimationInfoLoader* cbaLoader, std::vector<CBAEntry>& cbaEntries, const string& cbaFolderPath)
    : m_cbaLoader(cbaLoader)
    , m_cbaEntries(cbaEntries)
    , m_itemsDBAIndex(-1)
    , m_cbaFolderPath(cbaFolderPath)
{
}

void CBAEnumerator::GetDBA(EnumeratedDBA* dba, int index) const
{
    const string& dbName = m_cbaLoader->m_ADefinitions[index].m_DBName;
    if (!dbName.empty())
    {
        dba->innerPath = string("animations/") + dbName;
    }
    else
    {
        dba->innerPath.clear();
    }

    CacheItems(index);
    dba->animationCount = m_items.size();
}

int CBAEnumerator::GetDBACount() const
{
    return m_cbaLoader->m_ADefinitions.size();
}

bool CBAEnumerator::GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const
{
    CacheItems(dbaIndex);

    const SAnimationDefinition& definition = m_cbaLoader->m_ADefinitions[dbaIndex];
    const string& filename = m_items[animationIndex].m_name;

    string defPath  = definition.GetAnimationPath();
    if (StringHelpers::StartsWith(defPath, ".."))
    {
        defPath = defPath.substr(3);
    }
    else
    {
        defPath = string("animations/") + defPath;
    }
    caf->path = StringHelpers::MakeLowerCase(PathHelpers::ToUnixPath(PathHelpers::Join(defPath, filename)));

    SAnimationDesc desc = m_cbaLoader->m_ADefinitions[dbaIndex].GetAnimationDesc(caf->path);
    caf->skipDBA = desc.m_bSkipSaveToDatabase;
    return true;
}

void CBAEnumerator::CacheItems(int dbaIndex) const
{
    if (m_itemsDBAIndex != dbaIndex)
    {
        const SAnimationDefinition& definition = m_cbaLoader->m_ADefinitions[dbaIndex];
        FindCAFsForDefinition(&m_items, m_cbaFolderPath, definition, *m_cbaLoader);

        m_itemsDBAIndex = dbaIndex;
    }
}

