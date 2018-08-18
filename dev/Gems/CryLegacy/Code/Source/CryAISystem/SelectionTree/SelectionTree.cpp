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
#include "SelectionTree.h"
#include "SelectionContext.h"
#include "SelectionTreeManager.h"


SelectionTree::SelectionTree()
    : m_currentNodeID(0) // @Marcio: What if the first node is selected. Then the selected node and the current will not differ and no behavior will be selected? /Jonas 2010-08-25
    , m_templateID(0)
{
}

SelectionTree::SelectionTree(const SelectionTreeTemplateID& templateID)
    : m_currentNodeID(0)
    , m_templateID(templateID)
{
}

SelectionTree::SelectionTree(const SelectionTree& selectionTree)
    : m_currentNodeID(selectionTree.m_currentNodeID)
    , m_templateID(selectionTree.m_templateID)
    , m_nodes(selectionTree.m_nodes)
{
}

void SelectionTree::Clear()
{
    m_currentNodeID = 0;
    SelectionTreeNodes().swap(m_nodes);
}

void SelectionTree::Reset()
{
    m_currentNodeID = 0;
}

const SelectionNodeID& SelectionTree::Evaluate(const SelectionVariables& variables)
{
    SelectionContext context(m_nodes, m_currentNodeID, variables);
    if (!m_nodes.front().Evaluate(context, m_currentNodeID))
    {
        m_currentNodeID = 0;
    }

    return m_currentNodeID;
}

SelectionNodeID SelectionTree::AddNode(const SelectionTreeNode& node)
{
    m_nodes.push_back(node);
    m_nodes.back().SetNodeID(m_nodes.size());
    return m_nodes.size();
}

SelectionTreeNode& SelectionTree::GetNode(const NodeID& nodeID)
{
    return m_nodes[nodeID - 1];
}

const SelectionTreeNode& SelectionTree::GetNode(const NodeID& nodeID) const
{
    return m_nodes[nodeID - 1];
}

const SelectionNodeID& SelectionTree::GetCurrentNodeID() const
{
    return m_currentNodeID;
}

const SelectionTreeTemplateID& SelectionTree::GetTemplateID() const
{
    return m_templateID;
}

const SelectionTreeTemplate& SelectionTree::GetTemplate() const
{
    return gAIEnv.pSelectionTreeManager->GetTreeTemplate(m_templateID);
}

uint32 SelectionTree::GetNodeCount() const
{
    return m_nodes.size();
}

const SelectionTreeNode& SelectionTree::GetNodeAt(uint32 index) const
{
    assert(index < m_nodes.size());

    return m_nodes[index];
}

void SelectionTree::ReserveNodes(uint32 nodeCount)
{
    m_nodes.reserve(nodeCount);
}

void SelectionTree::Swap(SelectionTree& other)
{
    std::swap(m_templateID, other.m_templateID);
    std::swap(m_currentNodeID, other.m_currentNodeID);
    m_nodes.swap(other.m_nodes);
}

bool SelectionTree::Empty() const
{
    return m_nodes.empty();
}

void SelectionTree::Validate()
{
    SelectionTreeNodes(m_nodes).swap(m_nodes);
}

void SelectionTree::Serialize(TSerialize ser)
{
    ser.Value("m_currentNodeID", m_currentNodeID);
}

void SelectionTree::DebugDraw() const
{
}
