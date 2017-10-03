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
        class Name
            : public Node
        {
        public:
            AZ_RTTI(Name, "{7CB58C7A-6792-4633-A5B1-3F53B15DAD60}", Node);
            AZ_CLASS_ALLOCATOR(Name, AZ::SystemAllocator, 0);

            Name(const NodeCtrInfo& info, const AZStd::string&& name);

            AZ_INLINE const AZStd::string& Get() const { return m_name; }

            void Visit(Visitor& visitor) const;

        private:
            const AZStd::string m_name;
        };
    }
} // namespace ScriptCanvas