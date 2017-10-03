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

#include "precompiled.h"
#include "ExecutionIterator.h"

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>

#include <ScriptCanvas/Grammar/Parser.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        ExecutionIterator::ExecutionIterator()
            : m_visitor(*this)
        {}

        ExecutionIterator::ExecutionIterator(const Graph& graph)
            : m_visitor(*this)
        {
            Begin(graph);
        }

        ExecutionIterator::ExecutionIterator(const Node& block)
            : m_visitor(*this)
        {
            Begin(block);
        }

        bool ExecutionIterator::Begin()
        {
            m_current = m_queue.begin();
            return bool(*this);
        }

        bool ExecutionIterator::Begin(const Graph& graph)
        {
            return BuildQueue(graph);
        }

        bool ExecutionIterator::Begin(const Node& block)
        {
            // find a block from the node (which should be a while/repeat/scope, etc)
            return BuildQueue(block);
        }

        void ExecutionIterator::BuildNextStatement()
        {
            if (m_currentSourceStatementNode = Grammar::FindNextStatement(*m_currentSourceStatementNode))
            {
                m_currentSourceNode = m_currentSourceStatementNode;
                m_currentSourceNode->Visit(m_visitor);
            }

            // refactor the build process into a seperate class and just return the queue
            m_currentSourceStatementNode = m_currentSourceStatementRoot = m_currentSourceNode = nullptr;
            m_pushingExpressions = false;
            m_currentSourceInitialStatements.clear();
        }

        bool ExecutionIterator::BuildQueue()
        {
            if (m_currentSourceInitialStatements.empty())
            {
                AZ_Warning("Script Canvas", false, "This script has no executable statements. This script does nothing.");
                return false;
            }
            
            for (const auto initialSourceStatement : m_currentSourceInitialStatements)
            {
                m_currentSourceNode = m_currentSourceStatementNode = m_currentSourceStatementRoot = initialSourceStatement;
                m_currentSourceNode->Visit(m_visitor);
            }

            return Begin();
        }

        bool ExecutionIterator::BuildQueue(const Graph& graph)
        {
            m_source = &graph;
            m_sourceNodes = graph.GetNodesConst();
            m_currentSourceInitialStatements = Grammar::FindInitialStatements(m_sourceNodes);
            return BuildQueue();
        }

        bool ExecutionIterator::BuildQueue(const Node& node)
        {
            return Begin();
        }

        const Node* ExecutionIterator::FindNextStatement(const Node& current)
        {
            return nullptr;
        }

        const Node* ExecutionIterator::operator->() const
        {
            AZ_Assert(*this, "invalid iterator");
            return *m_current;
        }

        ExecutionIterator::operator bool() const
        {
            return m_current != m_queue.end();
        }

        bool ExecutionIterator::operator!() const
        {
            return m_current == m_queue.end();
        }

        ExecutionIterator& ExecutionIterator::operator++()
        {
            ++m_current;
            return *this;
        }

        void ExecutionIterator::PrettyPrint() const
        {
            int count(-1);

            for (auto node : m_queue)
            {
                AZ_TracePrintf("AST", "Execution Queue %3d: %s", ++count, node->RTTI_GetTypeName());
            }
        }

        void ExecutionIterator::PushExpression()
        {
            const auto input = m_currentSourceNode->GetConnectedNodesByType(SlotType::DataIn);

            for (const auto inputNode : input)
            {
                m_currentSourceNode = inputNode;
                m_queue.push_back(m_currentSourceNode);
                inputNode->Visit(m_visitor);
            }
        }
        
        void ExecutionIterator::PushFunctionCall()
        {
            if (m_pushingExpressions)
            {
                PushExpression();
            }
            else
            {
                PushStatement();
            }
        }

        void ExecutionIterator::PushStatement()
        {
            m_queue.push_back(m_currentSourceNode);
            m_iteratedStatements.insert(m_currentSourceNode);
            m_pushingExpressions = true;
            PushExpression();
            m_pushingExpressions = false;
            BuildNextStatement();
        }


    } // namespace Grammar

} // namespace ScriptCanvas