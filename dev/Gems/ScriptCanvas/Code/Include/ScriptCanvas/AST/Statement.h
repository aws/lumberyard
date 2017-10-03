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
        class Statement
            : public Node
        {
        public:
            AZ_RTTI(Statement, "{A99525EE-0844-4945-8ACB-D02B945E7453}", Node);
            AZ_CLASS_ALLOCATOR(Statement, AZ::SystemAllocator, 0);
            
            Statement(const NodeCtrInfo& info);
        };
    }
} // namespace ScriptCanvas