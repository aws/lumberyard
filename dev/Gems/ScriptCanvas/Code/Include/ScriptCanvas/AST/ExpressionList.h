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
        class ExpressionList
            : public Node
        {
        public:
            AZ_RTTI(ExpressionList, "{E01414FE-C1F8-421A-9691-D0FED9723964}", Node);
            AZ_CLASS_ALLOCATOR(ExpressionList, AZ::SystemAllocator, 0);

            ExpressionList(const NodeCtrInfo& info, const AZStd::vector<SmartPtrConst<Expression>>& expressions);

            void Visit(Visitor& visitor) const override;
        };
    }
} // namespace ScriptCanvas