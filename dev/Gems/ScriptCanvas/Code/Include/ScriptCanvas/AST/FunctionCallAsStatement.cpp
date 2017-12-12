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
#include "FunctionCallAsStatement.h"

#include <ScriptCanvas/Core/Node.h>

#include "ExpressionList.h"
#include "PrefixExpression.h"
#include "Visitor.h"

namespace ScriptCanvas
{
namespace AST 
{
    FunctionCallAsStatement::FunctionCallAsStatement(const NodeCtrInfo& info, SmartPtrConst<PrefixExpression>&& function, SmartPtrConst<ExpressionList>&& arguments)
        : Statement(NodeCtrInfo(info, Grammar::eRule::FunctionCall))
    {
        AddChild(function);
        AddChild(arguments);
        SetFunctionName(info.m_node->RTTI_GetTypeName());
    }

    void FunctionCallAsStatement::SetFunctionName(const char* functionName)
    {
        m_functionName = functionName;
    }

    void FunctionCallAsStatement::Visit(Visitor& visitor) const
    {
        visitor.Visit(*this);
    }
}
} // namespace ScriptCanvas