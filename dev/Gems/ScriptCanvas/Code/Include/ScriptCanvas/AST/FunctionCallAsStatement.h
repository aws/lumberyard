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
#include "Statement.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class FunctionCallAsStatement
            : public Statement
        {
        public:
            AZ_RTTI(FunctionCallAsStatement, "{6742065A-2F19-446C-BFAD-A0FA89FDAB9A}", Statement);
            AZ_CLASS_ALLOCATOR(FunctionCallAsStatement, AZ::SystemAllocator, 0);

            FunctionCallAsStatement(const NodeCtrInfo& info, SmartPtrConst<PrefixExpression>&& function, SmartPtrConst<ExpressionList>&& arguments);

            void SetFunctionName(const char* functionName);

            void Visit(Visitor& visitor) const override;

        private:
            AZStd::string m_functionName;
        };
    }
} // namespace ScriptCanvas