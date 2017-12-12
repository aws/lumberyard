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
        class Visitor;

        class Block 
            : public Node
        {
        public:
            AZ_RTTI(Block, "{37D0852C-C57F-41DE-9A51-4B433B9740BC}", Node);
            AZ_CLASS_ALLOCATOR(Block, AZ::SystemAllocator, 0);

            Block(const NodeCtrInfo& info); //, const AZStd::vector<SmartPtrConst<Statement>>& statements, const SmartPtrConst<ReturnStatement>& returnStatement);

            int GetNumStatements() const;
            SmartPtrConst<Statement> GetStatement(int index) const;
            SmartPtrConst<ReturnStatement> GetReturnStatement() const;

            void Visit(Visitor& visitor) const override;

        private:
            int m_numStatements = 0;
        };
    }
} // namespace ScriptCanvas