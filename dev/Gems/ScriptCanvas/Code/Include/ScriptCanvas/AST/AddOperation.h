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
#include "BinaryOperation.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class AddOperation
            : public BinaryOperation
        {
        public:
            AZ_RTTI(AddOperation, "{87E685D2-830F-4700-9E61-86C752345524}", BinaryOperation);
            AZ_CLASS_ALLOCATOR(AddOperation, AZ::SystemAllocator, 0);

            AddOperation(const NodeCtrInfo& info, SmartPtrConst<Expression>&& lhs, SmartPtrConst<Expression>&& rhs);

            void Visit(Visitor& visitor) const override;
        };
    }
} // namespace ScriptCanvas