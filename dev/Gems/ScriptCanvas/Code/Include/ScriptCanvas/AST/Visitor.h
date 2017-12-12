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

#pragma once

#include "AST.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class Visitor
        {
        public:
            virtual void Visit(const BinaryOperation& node) = 0;
            virtual void Visit(const Block& node) = 0;
            virtual void Visit(const Expression& node) = 0;
            virtual void Visit(const ExpressionList& node) = 0;
            virtual void Visit(const FunctionCall& node) = 0;
            virtual void Visit(const FunctionCallAsPrefixExpression& node) = 0;
            virtual void Visit(const FunctionCallAsStatement& node) = 0;
            virtual void Visit(const Name& node) = 0;
            virtual void Visit(const PrefixExpression& node) = 0;
            virtual void Visit(const ReturnStatement& node) = 0;
            virtual void Visit(const Statement& node) = 0;
            virtual void Visit(const UnaryOperation& node) = 0;
            virtual void Visit(const Variable& node) = 0;
        };
    }
} // namespace ScriptCanvas