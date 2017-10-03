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
#include "Name.h"

#include "Visitor.h"

namespace ScriptCanvas
{
    namespace AST
    {
        Name::Name(const NodeCtrInfo& info, const AZStd::string&& name)
            : Node(NodeCtrInfo(info, Grammar::eRule::Name))
            , m_name(AZStd::move(name))
        {}

        void Name::Visit(Visitor& visitor) const
        { 
            visitor.Visit(*this); 
        }

    } // namespace AST
} // namespace ScriptCanvas