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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREE_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREE_H
#pragma once

/*
    This file implements the runtime portion of the selection tree.
    It is also used as the tree template, and new trees are just assignments of this one.
*/

#include "BlockyXml.h"
#include "SelectionTreeNode.h"
#include "SelectionVariables.h"


typedef uint32 SelectionTreeTemplateID;


class SelectionTreeTemplate;
class SelectionTree
{
    typedef SelectionNodeID NodeID;
public:
    SelectionTree();
    SelectionTree(const SelectionTreeTemplateID& templateID);
    SelectionTree(const SelectionTree& selectionTree);

    void Clear();
    void Reset();

    const SelectionNodeID& Evaluate(const SelectionVariables& variables);

    SelectionNodeID AddNode(const SelectionTreeNode& node);
    SelectionTreeNode& GetNode(const NodeID& nodeID);
    const SelectionTreeNode& GetNode(const NodeID& nodeID) const;
    const SelectionNodeID& GetCurrentNodeID() const;

    const SelectionTreeTemplateID& GetTemplateID() const;
    const SelectionTreeTemplate& GetTemplate() const;

    uint32 GetNodeCount() const;
    const SelectionTreeNode& GetNodeAt(uint32 index) const;

    void ReserveNodes(uint32 nodeCount);
    bool Empty() const;
    void Swap(SelectionTree& other);
    void Validate();

    void Serialize(TSerialize ser);

    void DebugDraw() const;

private:
    SelectionTreeTemplateID m_templateID;
    SelectionTreeNodes m_nodes;
    NodeID m_currentNodeID;
};


#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREE_H
