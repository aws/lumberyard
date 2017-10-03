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
        class Expression
            : public Node
        {
        public:
            AZ_RTTI(Expression, "{285CEA96-97E2-4DDE-A077-6E764D00BDD0}", Node);
            AZ_CLASS_ALLOCATOR(Expression, AZ::SystemAllocator, 0);

            Expression(const NodeCtrInfo& info);
        };
    }
} // namespace ScriptCanvas