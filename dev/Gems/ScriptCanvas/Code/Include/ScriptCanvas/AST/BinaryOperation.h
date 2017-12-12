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
        class BinaryOperation
            : public Expression
        {
        public:
            AZ_RTTI(BinaryOperation, "{2BDFF4EA-C258-45A6-A003-1C120FA983BB}", Expression);
            AZ_CLASS_ALLOCATOR(BinaryOperation, AZ::SystemAllocator, 0);

            BinaryOperation(const NodeCtrInfo& info, SmartPtrConst<Expression>&& lhs, SmartPtrConst<Expression>&& rhs);
        };
    }
} // namespace ScriptCanvas