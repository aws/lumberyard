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
#include "MultiplyOperation.h"

#include "Visitor.h"

namespace ScriptCanvas
{
    namespace AST
    {
        MultiplyOperation::MultiplyOperation(const NodeCtrInfo& info, SmartPtrConst<Expression>&& lhs, SmartPtrConst<Expression>&& rhs)
            : BinaryOperation(NodeCtrInfo(info, Grammar::eRule::MultiplyOperation), std::move(lhs), std::move(rhs))
        {}

        void MultiplyOperation::Visit(Visitor& visitor) const 
        { 
            visitor.Visit(*this);
        }
    }
} // namespace ScriptCanvas