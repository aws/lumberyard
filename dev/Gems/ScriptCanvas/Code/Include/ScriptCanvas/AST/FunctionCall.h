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
#include "Node.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class FunctionCall // consider making these all interfaces
            : public Node
        {
        public:
            AZ_RTTI(FunctionCall, "{5FE0FC52-9511-4586-A6A5-031E774D2DC2}", Node);
            AZ_CLASS_ALLOCATOR(FunctionCall, AZ::SystemAllocator, 0);

            FunctionCall(const NodeCtrInfo& info, SmartPtrConst<PrefixExpression>&& function, SmartPtrConst<ExpressionList>&& arguments);

            void SetFunctionName(const char* functionName);

            void Visit(Visitor& visitor) const override;
        
        private:
            AZStd::string m_functionName;
        };
    }
} // namespace ScriptCanvas