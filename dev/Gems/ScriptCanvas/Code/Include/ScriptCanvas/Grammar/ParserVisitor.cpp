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
#include "ParserVisitor.h"

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Core/Core.h>
#include <ScriptCanvas/Libraries/Logic/Logic.h>
#include <ScriptCanvas/Libraries/Math/Math.h>

#include "Parser.h"

namespace ScriptCanvas 
{
    namespace Grammar
    {
        ParserVisitor::ParserVisitor(Parser& parser)
            : m_parser(parser)
        {}

        void ParserVisitor::Visit(const Nodes::Math::Divide& node)
        {
            AZ_Assert(false, "redo me!"); // m_result = m_parser.ParseBinaryOperation(node, eRule::DivideOperation);
        }

        void ParserVisitor::Visit(const Nodes::Math::Multiply& node)
        {
            AZ_Assert(false, "redo me!"); // m_result = m_parser.ParseBinaryOperation(node, eRule::MultiplyOperation);
        }

        void ParserVisitor::Visit(const Nodes::Math::Number& node)
        {
            AZ_Assert(false, "redo me!"); // m_result = m_parser.ParseNumeral(node);
        }

        void ParserVisitor::Visit(const Nodes::Math::Subtract& node)
        {
            AZ_Assert(false, "redo me!"); // m_result = m_parser.ParseBinaryOperation(node, eRule::SubtractOperation); 
        }

        void ParserVisitor::Visit(const Nodes::Math::Sum& node)
        {
            AZ_Assert(false, "redo me!"); // m_result = m_parser.ParseBinaryOperation(node, eRule::AddOperation);
        }

        // this could be genericized into function call as statement and added to the parser

        // the visitor would just have a thing that takes the expected rule from the parser and returns true if the current token is part of that rule

    } // namespace Grammar
} // namespace ScriptCanvas