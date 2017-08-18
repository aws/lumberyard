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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREEMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREEMANAGER_H
#pragma once

/*
    This file implements a container and loader for selection trees.
*/

#include <ISelectionTreeManager.h>

#include "BlockyXml.h"
#include "SelectionTreeTemplate.h"


class CSelectionTreeManager
    : public ISelectionTreeManager
{
public:
    virtual ~CSelectionTreeManager(){}
    virtual void Reload();
    void ScanFolder(const char* folderName);

    void Reset();

    SelectionTreeTemplateID GetTreeTemplateID(const char* treeName) const;
    bool HasTreeTemplate(const SelectionTreeTemplateID& templateID) const;
    const SelectionTreeTemplate& GetTreeTemplate(const SelectionTreeTemplateID& templateID) const;

    // ISelectionTreeManager
    virtual uint32 GetSelectionTreeCount() const;
    virtual uint32 GetSelectionTreeCountOfType(const char* typeName) const;

    virtual const char* GetSelectionTreeName(uint32 index) const;
    virtual const char* GetSelectionTreeNameOfType(const char* typeName, uint32 index) const;

    virtual ISelectionTreeDebugger* GetDebugger() const;
    //~ISelectionTreeManager

private:
    void ResetBehaviorSelectionTreeOfAIActorOfType(unsigned short int nType);

    void DiscoverFolder(const char* folderName);
    bool Discover(const char* fileName);
    bool LoadBlocks(const XmlNodeRef& blocksNode, const char* scope, const char* fileName);
    bool LoadBlock(const XmlNodeRef& blockNode, const char* scope, const char* fileName);
    bool DiscoverBlocks(const XmlNodeRef& rootNode, const char* scope, const char* fileName);

    bool LoadFileNode(const XmlNodeRef& rootNode, const char* fileName);
    bool LoadTreeTemplate(const XmlNodeRef& rootNode, const char* fileName);

    typedef std::map<SelectionTreeTemplateID, SelectionTreeTemplate> TreeTemplates;
    TreeTemplates m_templates;

    typedef std::multimap<string, SelectionTreeTemplateID> TreeTemplateTypes;
    TreeTemplateTypes m_templateTypes;

    BlockyXmlBlocks::Ptr m_blocks;

    struct FileNode
    {
        FileNode(const char* file, const XmlNodeRef& node)
            : fileName(file)
            , rootNode(node)
        {
        }

        string fileName;
        XmlNodeRef rootNode;
    };

    typedef std::vector<FileNode> FileNodes;
    FileNodes m_fileNodes;

    typedef std::vector<string> FolderNames;
    FolderNames m_folderNames;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREEMANAGER_H
