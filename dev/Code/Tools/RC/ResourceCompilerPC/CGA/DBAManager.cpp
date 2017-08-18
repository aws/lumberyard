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
#include "DBAManager.h"

#include "../../../CryXML/IXMLSerializer.h"
#include "../../../CryXML/ICryXML.h"
#include "FileUtil.h"
#include "PathHelpers.h"
#include "StringHelpers.h"
#include "PakSystem.h"
#include "PakXmlFileBufferSource.h"


//////////////////////////////////////////////////////////////////////////
DBAManager::DBAManager(IPakSystem* pPakSystem, ICryXML* pXmlParser)
    : m_pPakSystem(pPakSystem)
    , m_pXmlParser(pXmlParser)
{
    assert(m_pPakSystem);
    assert(m_pXmlParser);
}


//////////////////////////////////////////////////////////////////////////
bool DBAManager::LoadDBATable(const string& filename)
{
    m_dbaTablePath = filename;

    m_animationToDbaIndex.clear();
    m_dbaEntries.clear();

    RCLog("Loading dba table '%s'.", filename.c_str());
    const bool bFileExists = FileUtil::FileExists(filename.c_str());
    if (!bFileExists)
    {
        RCLog("Failed to find '%s' on disk, searching in the paks...", filename.c_str());
        PakSystemFile* pFile = m_pPakSystem->Open(filename.c_str(), "rb");
        const bool bFileExistsInPak = (pFile != NULL);
        if (!bFileExistsInPak)
        {
            RCLogError("Can't load dba table: Failed to find '%s' on disk or in pak files.", filename.c_str());
            return false;
        }
        m_pPakSystem->Close(pFile);
    }

    XmlNodeRef pXmlRoot;
    {
        const int errorBufferSize = 4096;
        char errorBuffer[errorBufferSize];
        IXMLSerializer* pXmlSerializer = m_pXmlParser->GetXMLSerializer();
        PakXmlFileBufferSource xmlFileBufferSource(m_pPakSystem, filename.c_str());
        const bool bRemoveNonessentialSpacesFromContent = true;
        pXmlRoot = pXmlSerializer->Read(xmlFileBufferSource, bRemoveNonessentialSpacesFromContent, errorBufferSize, errorBuffer);
        if (!pXmlRoot)
        {
            RCLogError("Can't load dba table: Failed to read '%s'. '%s'\n", filename.c_str(), errorBuffer);
            return false;
        }
    }

    const int dbaEntriesCount = pXmlRoot->getChildCount();
    m_dbaEntries.reserve(dbaEntriesCount);
    for (int i = 0; i < dbaEntriesCount; ++i)
    {
        XmlNodeRef pXmlDBANode = pXmlRoot->getChild(i);
        const string dbaPath = pXmlDBANode->getAttr("Path");
        if (dbaPath.empty())
        {
            RCLogError("While parsing '%s' failed to find 'Path' attribute for dba entry element '%s' at line '%d'. Skipping this entry.", filename.c_str(), pXmlDBANode->getTag(), pXmlDBANode->getLine());
            continue;
        }
        const size_t dbaEntryIndex = m_dbaEntries.size();
        m_dbaEntries.push_back(DBAManagerEntry());
        DBAManagerEntry& dbaEntry = m_dbaEntries[dbaEntryIndex];

        dbaEntry.m_path = PathHelpers::ToUnixPath(StringHelpers::MakeLowerCase(dbaPath));

        const int animationEntriesCount = pXmlDBANode->getChildCount();
        dbaEntry.m_animations.reserve(animationEntriesCount);
        for (int j = 0; j < animationEntriesCount; ++j)
        {
            XmlNodeRef pXmlAnimationNode = pXmlDBANode->getChild(j);
            string animationPath = pXmlAnimationNode->getAttr("Path");
            if (animationPath.empty())
            {
                RCLogError("While parsing '%s' failed to find 'Path' attribute for animation entry element '%s' at line '%d'. Skipping this entry.", filename.c_str(), pXmlAnimationNode->getTag(), pXmlAnimationNode->getLine());
                continue;
            }
            animationPath = PathHelpers::ToUnixPath(StringHelpers::MakeLowerCase(animationPath));

            AnimationNameToDBAIndexMap::const_iterator cit = m_animationToDbaIndex.find(animationPath);
            const bool bAnimationPathDuplicated = (cit != m_animationToDbaIndex.end());
            if (bAnimationPathDuplicated)
            {
                const size_t otherDbaEntryIndex = cit->second;
                const DBAManagerEntry& otherDbaEntry = m_dbaEntries[otherDbaEntryIndex];
                const char* const otherDbaPath = otherDbaEntry.m_path.c_str();
                RCLogError("While parsing '%s' found duplicate animation entry element with path '%s' at line '%d'. Animation path is already assigned to dba '%s'. Skipping this entry.", filename.c_str(), pXmlAnimationNode->getTag(), pXmlAnimationNode->getLine(), otherDbaPath);
                continue;
            }

            dbaEntry.m_animations.push_back(animationPath);
            m_animationToDbaIndex[animationPath] = dbaEntryIndex;
        }
        std::sort(dbaEntry.m_animations.begin(), dbaEntry.m_animations.end());
    }

    RCLog("Finished loading dba table.");

    return true;
}


//////////////////////////////////////////////////////////////////////////
const char* DBAManager::FindDBAPath(const char* const animationName) const
{
    assert(animationName);
    const string animationPath = animationName;
    AnimationNameToDBAIndexMap::const_iterator cit = m_animationToDbaIndex.find(animationPath);
    const bool bFound = (cit != m_animationToDbaIndex.end());
    if (!bFound)
    {
        return NULL;
    }
    const size_t dbaEntryIndex = cit->second;
    const DBAManagerEntry& dbaEntry = m_dbaEntries[dbaEntryIndex];
    const string& dbaPath = dbaEntry.GetPath();
    return dbaPath.c_str();
}
