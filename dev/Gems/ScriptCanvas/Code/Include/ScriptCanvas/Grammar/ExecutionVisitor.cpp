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
#include "ExecutionVisitor.h"

#include <ScriptCanvas/Core/Node.h>

#include "ExecutionIterator.h"

namespace ScriptCanvas 
{
    namespace Grammar
    {
        ExecutionVisitor::ExecutionVisitor(ExecutionIterator& iterator)
            : m_iterator(iterator)
        {}

        void ExecutionVisitor::Visit(const Nodes::Math::Divide& node)
        {
            m_iterator.PushExpression();
        }

        void ExecutionVisitor::Visit(const Nodes::Math::Multiply& node)
        {
            m_iterator.PushExpression();
        }

        void ExecutionVisitor::Visit(const Nodes::Math::Number& node)
        {
            m_iterator.PushExpression();
        }

        void ExecutionVisitor::Visit(const Nodes::Math::Subtract& node)
        {
            m_iterator.PushExpression();
        }

        void ExecutionVisitor::Visit(const Nodes::Math::Sum& node)
        {
            m_iterator.PushExpression();
        }

    } // namespace Grammar

} // namespace ScriptCanvas