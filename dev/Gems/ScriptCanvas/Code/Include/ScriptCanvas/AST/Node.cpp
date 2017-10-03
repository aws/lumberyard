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
#include "Node.h"

namespace ScriptCanvas
{
namespace AST
{
    Node::Node(const NodeCtrInfo& info)
        : m_info(info)
    {
        AZ_Assert(info.m_rule != Grammar::eRule::Count, "invalid node construction");
    }

    void Node::PrettyPrint() const
    {
        AZStd::string output("AST:\n");
        PrettyPrint(0, output);
    }

    void Node::PrettyPrint(int depth, AZStd::string& ) const
    {
        AZStd::string output;

        for (int i = 1; i < depth; ++i)
        {
            output.append("|..");
        }
        
        output.append(">");
        output.append(ToString());
        output.append("\n");
        AZ_TracePrintf("AST", "AST!\n%s", output.c_str());
        output.clear();

        ++depth;
        
        for (auto& child : m_children)
        {
            child->PrettyPrint(depth, output);
        }
    }

    const char* Node::GetName() const
    {
        return RTTI_GetTypeName();
    }

    const char* Node::ToString() const
    {
        return GetName();
    }

} // namespace AST
} // namespace ScriptCanvas