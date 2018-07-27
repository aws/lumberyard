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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTRANSLATOR_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTRANSLATOR_H
#pragma once

/*
    A Leaf node name translator. Any leafs with a matching name will be translated.
    It supports disambiguation, if needed, with parent-qualified leaf names with : as separator.

    Example: Root:Combat:Shoot
*/

#include "BlockyXml.h"
#include "SelectionTree.h"


class SelectionTree;
class SelectionTranslator
{
public:
    bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const SelectionTree& selectionTree, const XmlNodeRef& rootNode,
        const char* scopeName, const char* fileName);

    const char* GetTranslation(const SelectionNodeID& nodeID) const;
private:
    struct QualifiedTranslation
    {
        std::vector<string> ancestors;
        string target;
    };

    typedef std::map<SelectionNodeID, string> Translations;
    Translations m_translations;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTRANSLATOR_H
