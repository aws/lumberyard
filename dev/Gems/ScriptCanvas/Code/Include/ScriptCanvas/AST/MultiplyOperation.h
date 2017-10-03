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
        class MultiplyOperation
            : public BinaryOperation
        {
        public:
            AZ_RTTI(MultiplyOperation, "{90672387-00A8-4BB4-BB27-0AA8B66BDB8D}", BinaryOperation);
            AZ_CLASS_ALLOCATOR(MultiplyOperation, AZ::SystemAllocator, 0);

            MultiplyOperation(const NodeCtrInfo& info, SmartPtrConst<Expression>&& lhs, SmartPtrConst<Expression>&& rhs);

            void Visit(Visitor& visitor) const override;
        };
    }
} // namespace ScriptCanvas