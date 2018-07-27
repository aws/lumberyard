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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREETEMPLATE_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREETEMPLATE_H
#pragma once

/*
    This file implements a simple container for all the concepts required to run a selection tree.
*/


#include "SelectionTree.h"
#include "SelectionTranslator.h"
#include "SelectionSignalVariables.h"


class SelectionTreeTemplate
{
public:
    bool LoadFromXML(const SelectionTreeTemplateID& templateID, const BlockyXmlBlocks::Ptr& blocks, const XmlNodeRef& rootNode,
        const char* fileName);

    const SelectionTree& GetSelectionTree() const;
    const SelectionVariableDeclarations& GetVariableDeclarations() const;
    const SelectionTranslator& GetTranslator() const;
    const SelectionSignalVariables& GetSignalVariables() const;

    const char* GetName() const;
    bool Valid() const;

private:
    SelectionTree m_selectionTree;
    SelectionVariableDeclarations m_variableDecls;
    SelectionTranslator m_translator;
    SelectionSignalVariables m_signalVariables;

    string m_name;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREETEMPLATE_H
