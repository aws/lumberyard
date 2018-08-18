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
#include "SelectionTreeNode.h"
#include "SelectionContext.h"
#include "SelectionTree.h"
#include "SelectionTreeDebugger.h"

SelectionTreeNode::SelectionTreeNode(NodeID parentID)
    : m_nodeID(0)
    , m_parentID(parentID)
    , m_currentID(0)
    , m_type(Invalid)
{
}

bool SelectionTreeNode::LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, SelectionTree& tree, const char* treeName,
    SelectionVariableDeclarations& variableDecls, const XmlNodeRef& rootNode,
    const char* fileName)
{
    assert(treeName);
    assert(m_nodeID);

    const char* nodeName = 0;
    if (rootNode->haveAttr("name"))
    {
        rootNode->getAttr("name", &nodeName);
    }
    else
    {
        AIWarning("Missing 'name' attribute for tag '%s' in file '%s' at line %d.",
            rootNode->getTag(), fileName, rootNode->getLine());

        return false;
    }

    m_name = nodeName;

    const char* nodeType = rootNode->getTag();

    if (!_stricmp(nodeType, "Priority") || !_stricmp(nodeType, "Sequence"))
    {
        m_type = !_stricmp(nodeType, "Priority") ? Priority : Sequence;

        BlockyXmlNodeRef blockyNode(blocks, treeName, rootNode, fileName);

        while (XmlNodeRef childNode = blockyNode.next())
        {
            const char* condition = 0;
            if (childNode->haveAttr("condition"))
            {
                childNode->getAttr("condition", &condition);
            }
            else
            {
                condition = "true";
            }

            NodeID childID = tree.AddNode(SelectionTreeNode(m_nodeID));
            SelectionTreeNode& child = tree.GetNode(childID);

            if (!child.LoadFromXML(blocks, tree, treeName, variableDecls, childNode, fileName))
            {
                return false;
            }

            m_children.push_back(Child(childID, SelectionCondition(condition, variableDecls)));

            if (!m_children.back().condition.Valid())
            {
                AIWarning("Failed to compile condition '%s' in file '%s' at line %d.",
                    condition, fileName, childNode->getLine());

                return false;
            }
        }
    }
    else if (!_stricmp(nodeType, "StateMachine"))
    {
        m_type = StateMachine;

        std::vector<string> stateNames;

        BlockyXmlNodeRef blockyNode(blocks, treeName, rootNode, fileName);

        while (XmlNodeRef childNode = blockyNode.next())
        {
            if (_stricmp(childNode->getTag(), "State"))
            {
                AIWarning("Unexpected tag '%s' in file '%s' at line %d. 'State' expected.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            const char* stateName = 0;
            if (childNode->haveAttr("name"))
            {
                childNode->getAttr("name", &stateName);
            }
            else
            {
                AIWarning("Missing 'name' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            stateNames.push_back(stateName);
        }

        blockyNode.first();
        while (XmlNodeRef childNode = blockyNode.next())
        {
            const char* stateName = stateNames[m_states.size()].c_str();

            m_states.push_back(State());
            State& state = m_states.back();

            BlockyXmlNodeRef blockStateNode(blocks, treeName, childNode, fileName);
            while (XmlNodeRef stateChildNode = blockStateNode.next())
            {
                if (!_stricmp(stateChildNode->getTag(), "Transition"))
                {
                    state.transitions.push_back(State::Transition());
                    State::Transition& transition = state.transitions.back();

                    const char* targetStateName = 0;
                    if (stateChildNode->haveAttr("state"))
                    {
                        stateChildNode->getAttr("state", &targetStateName);
                    }
                    else
                    {
                        AIWarning("Missing 'state' attribute for state '%s' transition in file '%s' at line %d.",
                            stateName, fileName, stateChildNode->getLine());

                        return false;
                    }

                    const char* condition = 0;
                    if (stateChildNode->haveAttr("condition"))
                    {
                        stateChildNode->getAttr("condition", &condition);
                    }
                    else
                    {
                        AIWarning("Missing 'condition' attribute for state '%s' transition in file '%s' at line %d.",
                            stateName, fileName, stateChildNode->getLine());

                        return false;
                    }

                    // find state by name
                    uint8 targetState = 0xff;
                    for (uint i = 0; i < stateNames.size(); ++i)
                    {
                        if (!_stricmp(stateNames[i], targetStateName))
                        {
                            targetState = i;
                            break;
                        }
                    }

                    if (targetState < stateNames.size())
                    {
                        transition.targetStateID = targetState;
                    }
                    else
                    {
                        AIWarning("Unknown target state '%s' for state '%s' transition in file '%s' at line %d.",
                            targetStateName, stateName, fileName, stateChildNode->getLine());

                        return false;
                    }

                    transition.condition = SelectionCondition(condition, variableDecls);
                    if (!transition.condition.Valid())
                    {
                        AIWarning("Failed to compile condition '%s' for state '%s' transition in file '%s' at line %d.",
                            condition, stateName, fileName, stateChildNode->getLine());

                        return false;
                    }
                }
                else if (state.childID == 0)
                {
                    state.childID = tree.AddNode(SelectionTreeNode(m_nodeID));
                    SelectionTreeNode& child = tree.GetNode(state.childID);

                    if (!child.LoadFromXML(blocks, tree, treeName, variableDecls, stateChildNode, fileName))
                    {
                        return false;
                    }
                }
                else
                {
                    AIWarning("Multiple children definition for State '%s' in file '%s' at line %d.",
                        stateName, fileName, stateChildNode->getLine());

                    return false;
                }
            }
        }
    }
    else if (!_stricmp(nodeType, "Leaf"))
    {
        m_type = Leaf;

        if (rootNode->getChildCount())
        {
            AIWarning("Leaf '%s' contains children in file '%s' at line %d.",
                m_name.c_str(), fileName, rootNode->getLine());

            return false;
        }
    }
    else
    {
        AIWarning("Unknown node type '%s' in file '%s' at line %d.",
            nodeType, fileName, rootNode->getLine());

        return false;
    }

    return true;
}

bool SelectionTreeNode::IsDescendant(SelectionContext& context, const NodeID& nodeID) const
{
    NodeID currentNodeID = nodeID;
    while (NodeID parentID = context.GetNode(currentNodeID).GetParentID())
    {
        const SelectionTreeNode& parentNode = context.GetNode(parentID);
        if (parentNode.GetNodeID() == GetNodeID())
        {
            return true;
        }

        currentNodeID = parentNode.GetNodeID();
    }

    return false;
}

bool SelectionTreeNode::IsAncestor(SelectionContext& context, const NodeID& nodeID) const
{
    if (m_parentID)
    {
        if (m_parentID != nodeID)
        {
            const SelectionTreeNode& parentNode = context.GetNode(m_parentID);
            return parentNode.IsAncestor(context, nodeID);
        }
        return true;
    }
    return false;
}

bool SelectionTreeNode::Evaluate(SelectionContext& context, SelectionNodeID& resultNodeID)
{
    BST_DEBUG_EVAL_NODE(*this);
    switch (m_type)
    {
    case Leaf:
    {
        resultNodeID = GetNodeID();
        return true;
    }
    case Priority:
    {
        Children::iterator it = m_children.begin();
        Children::iterator end = m_children.end();

        for (; it != end; ++it)
        {
            Child& child = *it;
            const bool conditionResult = child.condition.Evaluate(context.GetVariables());
            BST_DEBUG_EVAL_NODECONDITON(context.GetNode(child.childID), conditionResult);
            if (conditionResult)
            {
                SelectionTreeNode& childNode = context.GetNode(child.childID);
                if (childNode.Evaluate(context, resultNodeID))
                {
                    return true;
                }
            }
        }

        return false;
    }
    case Sequence:
    {
        if (!context.GetCurrentNodeID() || !IsDescendant(context, context.GetCurrentNodeID()))
        {
            m_currentID = 0;
        }
    }
    case StateMachine:
    {
        uint8 nextID = m_currentID;

        if (!context.GetCurrentNodeID() || !IsDescendant(context, context.GetCurrentNodeID()))
        {
            nextID = 0;
        }

        assert(nextID < 32);

        uint32 visitedIDs = 1 << m_currentID;

        while (true)
        {
            State& state = m_states[nextID];
            State::Transitions::iterator tit = state.transitions.begin();
            State::Transitions::iterator tend = state.transitions.end();

            for (; tit != tend; ++tit)
            {
                State::Transition& transition = *tit;

                const bool conditionResult = transition.condition.Evaluate(context.GetVariables());
                BST_DEBUG_EVAL_STATECONDITON(context.GetNode(state.childID), conditionResult);
                if (conditionResult)
                {
                    nextID = transition.targetStateID;
                    break;
                }
            }

            if (tit == tend)        // no transition
            {
                break;
            }

            if (visitedIDs & (1 << nextID))
            {
                // loop
                break;
            }

            visitedIDs |= (1 << nextID);
        }

        m_currentID = nextID;

        State& state = m_states[m_currentID];
        return context.GetNode(state.childID).Evaluate(context, resultNodeID);
    }
    default:
    {
        assert(0);
        return false;
    }
    }
}