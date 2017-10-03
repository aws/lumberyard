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
#include "Expression.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class PrefixExpression
            : public Expression
        {
        public:
            AZ_RTTI(PrefixExpression, "{C9487A52-633C-4A57-A413-9F7563829A85}", Expression);
            AZ_CLASS_ALLOCATOR(PrefixExpression, AZ::SystemAllocator, 0);

            PrefixExpression(const NodeCtrInfo& info);

            PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<Variable>&& variable);
            PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<FunctionCallAsPrefixExpression>&& functionCall);
            PrefixExpression(const NodeCtrInfo& info, AST::SmartPtrConst<FunctionCallAsStatement>&& functionCall);

            void Visit(Visitor& visitor) const override;
        };
    }
} // namespace ScriptCanvas