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
        class Variable
            : public Node
        {
        public:
            AZ_RTTI(Variable, "{9E1EAF1C-C44B-4A8E-BFCD-2D7E228047F0}", Node);
            AZ_CLASS_ALLOCATOR(Variable, AZ::SystemAllocator, 0);

            Variable(const NodeCtrInfo& info, AST::SmartPtrConst<Name>&& name);
            // Variable(const NodeCtrInfo& info, AST::SmartPtrConst<PrefixExpression>&& prefixExpression);
            // Variable(const NodeCtrInfo& info, AST::SmartPtrConst<PrefixExpression>&& prefixExpression, AST::SmartPtrConst<Name>&& name);

            void Visit(Visitor& visitor) const;
        };
    }
} // namespace ScriptCanvas