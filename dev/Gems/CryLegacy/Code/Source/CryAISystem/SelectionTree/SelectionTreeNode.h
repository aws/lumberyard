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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREENODE_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREENODE_H
#pragma once

/*
    This file implements a node in the selection tree.
*/


#include "BlockyXml.h"
#include "SelectionCondition.h"


typedef uint16 SelectionNodeID;


class SelectionContext;
class SelectionTreeNodes;
class SelectionTreeNode
{
    typedef SelectionNodeID NodeID;
public:
    enum Type
    {
        Leaf = 0,
        Priority,
        StateMachine,
        Sequence,
        Invalid,
    };

    SelectionTreeNode(NodeID parentID = 0);

    bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, SelectionTree& tree, const char* treeName,
        SelectionVariableDeclarations& variableDecls, const XmlNodeRef& rootNode,
        const char* fileName);

    inline void SetNodeID(const SelectionNodeID& nodeID)
    {
        m_nodeID = nodeID;
    }

    inline NodeID GetNodeID() const
    {
        return m_nodeID;
    }

    inline NodeID GetParentID() const
    {
        return m_parentID;
    }

    inline Type GetType() const
    {
        return m_type;
        ;
    }

    inline const char* GetName() const
    {
        return m_name.c_str();
    }

    bool IsDescendant(SelectionContext& context, const NodeID& nodeID) const;
    bool IsAncestor(SelectionContext& context, const NodeID& nodeID) const;
    bool Evaluate(SelectionContext& context, SelectionNodeID& nodeID);

private:
    string m_name;
    NodeID m_nodeID;
    NodeID m_parentID;
    uint8 m_currentID;
    Type m_type;

public:
    struct Child
    {
        Child(NodeID _childID, const SelectionCondition& _condition)
            : childID(_childID)
            , condition(_condition)
        {
        }

        SelectionCondition condition;
        NodeID childID;
    };
    typedef std::vector<Child> Children;

    inline const Children& GetChildren() const { return m_children; }

private:
    Children m_children;

    struct State
    {
        State()
            : childID(0)
        {
        }

        struct Transition
        {
            Transition()
                : targetStateID(-1)
            {
            }

            SelectionCondition condition;
            uint8 targetStateID;
        };

        NodeID childID;

        typedef std::vector<Transition> Transitions;
        Transitions transitions;
    };

    typedef std::vector<State> States;
    States m_states;
};


class SelectionTreeNodes
    : public std::vector<SelectionTreeNode>
{
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONTREENODE_H

