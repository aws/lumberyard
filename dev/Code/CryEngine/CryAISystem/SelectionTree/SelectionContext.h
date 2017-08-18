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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONCONTEXT_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONCONTEXT_H
#pragma once

/*
    This is the evaluation context for nodes.
*/

#include "SelectionVariables.h"
#include "SelectionTreeNode.h"


class SelectionContext
{
public:
    SelectionContext(SelectionTreeNodes& nodes, const SelectionNodeID& currentNodeID,
        const SelectionVariables& variables)
        : m_nodes(nodes)
        , m_currentNodeID(currentNodeID)
        , m_variables(variables)
    {
    }

    const SelectionVariables& GetVariables() const
    {
        return m_variables;
    }

    SelectionTreeNode& GetNode(const SelectionNodeID& nodeID) const
    {
        assert(nodeID > 0 && nodeID <= m_nodes.size());
        return m_nodes[nodeID - 1];
    }

    const SelectionNodeID& GetCurrentNodeID() const
    {
        return m_currentNodeID;
    }

private:
    SelectionTreeNodes& m_nodes;
    SelectionNodeID m_currentNodeID;

    const SelectionVariables& m_variables;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONCONTEXT_H
