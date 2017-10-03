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
#include "PrefixExpression.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class FunctionCallAsPrefixExpression
            : public PrefixExpression
        {
        public:
            AZ_RTTI(FunctionCallAsPrefixExpression, "{624EE374-9147-42BA-BB7B-3816743226AD}", PrefixExpression);
            AZ_CLASS_ALLOCATOR(FunctionCallAsPrefixExpression, AZ::SystemAllocator, 0);
    
            FunctionCallAsPrefixExpression(const NodeCtrInfo& info, SmartPtrConst<PrefixExpression>&& function, SmartPtrConst<ExpressionList>&& arguments);

            void SetFunctionName(const char* functionName);

            void Visit(Visitor& visitor) const override;

        private:
            AZStd::string m_functionName;
        };
    }
} // namespace ScriptCanvas