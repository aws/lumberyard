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

#include "precompiled.h"
#include "Block.h"

#include "Statement.h"
#include "ReturnStatement.h"
#include "Visitor.h"

namespace ScriptCanvas
{
    namespace AST
    {
        Block::Block(const NodeCtrInfo& info)
            : Node(NodeCtrInfo(info, Grammar::eRule::Block))
        {}

//         Block::Block(const NodeCtrInfo& info, const AZStd::vector<SmartPtrConst<Statement>>& statements, const SmartPtrConst<ReturnStatement>& returnStatement)
//             : Node(NodeCtrInfo(info, Grammar::eRule::Block))
//             , m_numStatements(azlossy_caster(statements.size()))
//         {}
        
        int Block::GetNumStatements() const
        {
            return m_numStatements;
        }

        SmartPtrConst<Statement> Block::GetStatement(int index) const
        {
            return index < m_numStatements ? GetChildAs<Statement>(index) : nullptr;
        }

        SmartPtrConst<ReturnStatement> Block::GetReturnStatement() const
        {
           return GetChildAs<ReturnStatement>(m_numStatements + 1);
        }

        void Block::Visit(Visitor& visitor) const
        {
            visitor.Visit(*this);
        }

    } // namespace AST

} // namespace ScriptCanvas