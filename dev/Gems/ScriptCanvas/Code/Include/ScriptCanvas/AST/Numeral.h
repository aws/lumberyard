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
#include "Expression.h"

namespace ScriptCanvas
{
    namespace AST
    {
        class Numeral
            : public Expression
        {
        public:
            AZ_RTTI(Numeral, "{A97D501D-79F8-417A-A76C-188948C794EC}", Expression);
            AZ_CLASS_ALLOCATOR(Numeral, AZ::SystemAllocator, 0);

            Numeral(const NodeCtrInfo& info, const float value);

            AZ_INLINE float GetValue() const { return m_value; }

            void Visit(Visitor& visitor) const;

        private:
            const float m_value;
        };
    }
} // namespace ScriptCanvas