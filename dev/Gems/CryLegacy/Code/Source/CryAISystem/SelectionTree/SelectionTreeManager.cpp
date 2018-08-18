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

#include "CryLegacy_precompiled.h"
#include "SelectionTreeManager.h"
#include "SelectionTreeDebugger.h"

#include <StringUtils.h>


void CSelectionTreeManager::DiscoverFolder(const char* folderName)
{
    string folder(folderName);
    folder += "/";

    string searchString(folder + "*.*");

    _finddata_t fd;
    intptr_t handle = 0;

    ICryPak* pPak = gEnv->pCryPak;
    handle = pPak->FindFirst(searchString.c_str(), &fd);

    if (handle > -1)
    {
        do
        {
            if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
            {
                continue;
            }

            if (fd.attrib & _A_SUBDIR)
            {
                DiscoverFolder(folder + fd.name);
            }
            else if (!_stricmp(PathUtil::GetExt(fd.name), "xml"))
            {
                Discover(folder + fd.name);
            }
        } while (pPak->FindNext(handle, &fd) >= 0);

        pPak->FindClose(handle);
    }
}

void CSelectionTreeManager::ScanFolder(const char* folderName)
{
    m_blocks.reset(new BlockyXmlBlocks());

    DiscoverFolder(folderName);

    stl::push_back_unique(m_folderNames, folderName);

    while (!m_fileNodes.empty())
    {
        FileNode back = m_fileNodes.back();
        m_fileNodes.pop_back();

        LoadFileNode(back.rootNode, back.fileName.c_str());
    }

    m_blocks.reset();
    FileNodes().swap(m_fileNodes);
}

void CSelectionTreeManager::Reload()
{
    Reset();

    TreeTemplates().swap(m_templates);

    for (uint32 i = 0; i < m_folderNames.size(); ++i)
    {
        ScanFolder(m_folderNames[i].c_str());
    }
}


void CSelectionTreeManager::Reset()
{
}

SelectionTreeTemplateID CSelectionTreeManager::GetTreeTemplateID(const char* treeName) const
{
    return CryStringUtils::CalculateHashLowerCase(treeName);
}

bool CSelectionTreeManager::HasTreeTemplate(const SelectionTreeTemplateID& templateID) const
{
    return m_templates.find(templateID) != m_templates.end();
}

const SelectionTreeTemplate& CSelectionTreeManager::GetTreeTemplate(const SelectionTreeTemplateID& templateID) const
{
    TreeTemplates::const_iterator it = m_templates.find(templateID);
    if (it != m_templates.end())
    {
        return it->second;
    }

    static SelectionTreeTemplate empty;

    return empty;
}

uint32 CSelectionTreeManager::GetSelectionTreeCount() const
{
    return m_templates.size();
}

uint32 CSelectionTreeManager::GetSelectionTreeCountOfType(const char* typeName) const
{
    return m_templateTypes.count(typeName);
}

const char* CSelectionTreeManager::GetSelectionTreeName(uint32 index) const
{
    assert(index < m_templates.size());

    TreeTemplates::const_iterator it = m_templates.begin();
    std::advance(it, (int)index);

    const SelectionTreeTemplate& treeTemplate = it->second;
    return treeTemplate.GetName();
}

const char* CSelectionTreeManager::GetSelectionTreeNameOfType(const char* typeName, uint32 index) const
{
    assert(index < m_templateTypes.count(typeName));

    TreeTemplateTypes::const_iterator it = m_templateTypes.find(typeName);
    std::advance(it, (int)index);

    TreeTemplates::const_iterator tit = m_templates.find(it->second);

    assert(tit != m_templates.end());
    const SelectionTreeTemplate& treeTemplate = tit->second;

    return treeTemplate.GetName();
}

ISelectionTreeDebugger* CSelectionTreeManager::GetDebugger() const
{
#ifndef RELEASE
    return gEnv->IsEditor() ? CSelectionTreeDebugger::GetInstance() : NULL;
#else
    return NULL;
#endif
}

bool CSelectionTreeManager::LoadBlocks(const XmlNodeRef& blocksNode, const char* scopeName, const char* fileName)
{
    uint32 blockCount = blocksNode->getChildCount();

    for (uint32 i = 0; i < blockCount; ++i)
    {
        XmlNodeRef childNode = blocksNode->getChild(i);
        if (!_stricmp(childNode->getTag(), "Block"))
        {
            if (!LoadBlock(childNode, scopeName, fileName))
            {
                return false;
            }
        }
        else
        {
            AIWarning("Invalid tag '%s' in file '%s' at line %d. Expected 'Block'.",
                childNode->getTag(), fileName, childNode->getLine());

            return false;
        }
    }

    return true;
}

bool CSelectionTreeManager::LoadBlock(const XmlNodeRef& blockNode, const char* scopeName, const char* fileName)
{
    if (blockNode->haveAttr("name"))
    {
        const char* blockName;
        blockNode->getAttr("name", &blockName);

        if (m_blocks->AddBlock(scopeName, blockName, blockNode, fileName))
        {
            return true;
        }
    }
    else
    {
        AIWarning("Missing block name attribute in file '%s' line %d.",
            fileName, blockNode->getLine());
    }

    return false;
}

bool CSelectionTreeManager::Discover(const char* fileName)
{
    XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(fileName);
    if (!rootNode)
    {
        AIWarning("Failed to load XML file '%s'...", fileName);

        return false;
    }

    if (!_stricmp(rootNode->getTag(), "Blocks"))
    {
        return LoadBlocks(rootNode, "Global", fileName);
    }
    else if (!_stricmp(rootNode->getTag(), "SelectionTrees"))
    {
        uint32 childCount = rootNode->getChildCount();
        for (uint32 i = 0; i < childCount; ++i)
        {
            XmlNodeRef childNode = rootNode->getChild(i);

            if (!_stricmp(childNode->getTag(), "SelectionTree"))
            {
                const char* scopeName = 0;
                if (childNode->haveAttr("name"))
                {
                    childNode->getAttr("name", &scopeName);
                    if (!DiscoverBlocks(childNode, scopeName, fileName))
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
        }
    }
    else if (!_stricmp(rootNode->getTag(), "SelectionTree"))
    {
        const char* scopeName = 0;
        if (rootNode->haveAttr("name"))
        {
            rootNode->getAttr("name", &scopeName);
            if (!DiscoverBlocks(rootNode, scopeName, fileName))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    m_fileNodes.push_back(FileNode(fileName, rootNode));

    return true;
}

bool CSelectionTreeManager::DiscoverBlocks(const XmlNodeRef& rootNode, const char* scope, const char* fileName)
{
    uint32 childCount = rootNode->getChildCount();
    for (uint32 i = 0; i < childCount; ++i)
    {
        XmlNodeRef childNode = rootNode->getChild(i);

        if (!_stricmp(childNode->getTag(), "Blocks"))
        {
            if (!LoadBlocks(childNode, scope, fileName))
            {
                return false;
            }
        }
    }

    return true;
}

bool CSelectionTreeManager::LoadFileNode(const XmlNodeRef& rootNode, const char* fileName)
{
    if (!_stricmp(rootNode->getTag(), "Blocks"))
    {
        return true;
    }
    else if (!_stricmp(rootNode->getTag(), "SelectionTree"))
    {
        return LoadTreeTemplate(rootNode, fileName);
    }
    else if (!_stricmp(rootNode->getTag(), "SelectionTrees"))
    {
        uint32 childCount = rootNode->getChildCount();
        for (uint32 i = 0; i < childCount; ++i)
        {
            XmlNodeRef childNode = rootNode->getChild(i);
            if (!_stricmp(childNode->getTag(), "SelectionTree"))
            {
                if (!LoadTreeTemplate(childNode, fileName))
                {
                    return false;
                }
            }
            else
            {
                AIWarning("Unexpected tag '%s' in file '%s' at line %d.", childNode->getTag(), fileName, childNode->getLine());

                return false;
            }
        }
    }
    else
    {
        AIWarning("Unexpected tag '%s' in file '%s' at line %d.", rootNode->getTag(), fileName, rootNode->getLine());

        return false;
    }

    return true;
}

bool CSelectionTreeManager::LoadTreeTemplate(const XmlNodeRef& rootNode, const char* fileName)
{
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "SelectionTreeManager::LoadTreeTemplate - %s", fileName);

    const char* treeName = 0;
    if (rootNode->haveAttr("name"))
    {
        rootNode->getAttr("name", &treeName);
    }
    else
    {
        AIWarning("Missing 'name' attribute for '%s' tag in file '%s' at line %d.",
            rootNode->getTag(), fileName, rootNode->getLine());

        return false;
    }

    const char* typeName = 0;
    if (rootNode->haveAttr("type"))
    {
        rootNode->getAttr("type", &typeName);
    }
    else
    {
        AIWarning("Missing 'type' attribute for '%s' tag in file '%s' at line %d.",
            rootNode->getTag(), fileName, rootNode->getLine());

        return false;
    }

    SelectionTreeTemplateID templateID = GetTreeTemplateID(treeName);

    std::pair<TreeTemplates::iterator, bool> iresult = m_templates.insert(
            TreeTemplates::value_type(templateID, SelectionTreeTemplate()));

    if (!iresult.second)
    {
        AIWarning("Duplicate SelectionTree definition '%s' in file '%s' at line %d.", treeName, fileName, rootNode->getLine());

        return false;
    }

    m_templateTypes.insert(TreeTemplateTypes::value_type(typeName, templateID));

    SelectionTreeTemplate& selectionTree = iresult.first->second;
    return selectionTree.LoadFromXML(templateID, m_blocks, rootNode, fileName);
}
