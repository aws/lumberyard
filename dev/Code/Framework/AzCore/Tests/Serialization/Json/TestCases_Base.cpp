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

#include <AzCore/Serialization/SerializeContext.h>
#include <Tests/Serialization/Json/TestCases_Base.h>


namespace JsonSerializationTests
{
    // BaseClass

    bool BaseClass::Equals(const BaseClass& rhs, bool) const
    {
        return m_baseVar == rhs.m_baseVar;
    }

    void BaseClass::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
    {
        context->Class<BaseClass>()->Field("base_var", &BaseClass::m_baseVar);
    }


    // BaseClass2

    bool BaseClass2::Equals(const BaseClass2& rhs, bool) const
    {
        return m_base2Var == rhs.m_base2Var;
    }

    void BaseClass2::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
    {
        context->Class<BaseClass2>()->Field("base2_var", &BaseClass2::m_base2Var);
    }
} // namespace JsonSerializationTests
