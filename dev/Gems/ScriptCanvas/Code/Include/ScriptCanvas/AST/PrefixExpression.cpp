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
#include "PrefixExpression.h"

#include "FunctionCallAsPrefixExpression.h"
#include "FunctionCallAsStatement.h"
#include "Variable.h"
#include "Visitor.h"

namespace ScriptCanvas
{
    namespace AST
    {
        PrefixExpression::PrefixExpression(const NodeCtrInfo& info)
        : Expression(info)
        {}

        PrefixExpression::PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<Variable>&& variable)
        : Expression(NodeCtrInfo(info, Grammar::eRule::PrefixExpression))
        {
            AddChild(variable);
        }

        PrefixExpression::PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<FunctionCallAsPrefixExpression>&& functionCall)
        : Expression(NodeCtrInfo(info, Grammar::eRule::PrefixExpression))
        {
            AddChild(functionCall);
        }

        PrefixExpression::PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<FunctionCallAsStatement>&& functionCall)
        : Expression(NodeCtrInfo(info, Grammar::eRule::PrefixExpression))
        {
            AddChild(functionCall);
        }

        void PrefixExpression::Visit(Visitor& visitor) const
        {
            visitor.Visit(*this);
        }
    }
} // namespace ScriptCanvas