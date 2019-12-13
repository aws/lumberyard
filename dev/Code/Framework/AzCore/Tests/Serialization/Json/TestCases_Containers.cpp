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
#include <Tests/Serialization/Json/TestCases_Containers.h>

namespace JsonSerializationTests
{
    // SimpleContainer_Array

    SimpleContainer_Array::SimpleContainer_Array()
    {
        // Initialized to zero because POD's are not initialize and therefore contain
        // garbage, just as with normal arrays.
        m_array[0] = 0;
        m_array[1] = 0;
        m_array[2] = 0;
    }

    bool SimpleContainer_Array::Equals(const SimpleContainer_Array& rhs, bool fullReflection) const
    {
        return !fullReflection || m_array == rhs.m_array;
    }

    void SimpleContainer_Array::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<SimpleContainer_Array>()
                ->Field("array", &SimpleContainer_Array::m_array);
        }
    }

    InstanceWithSomeDefaults<SimpleContainer_Array> SimpleContainer_Array::GetInstanceWithSomeDefaults()
    {
        // Because there's always a fixed number of elements (in this case all initialized to 0)
        // the full array is written out.
        const char* keptDefaults = R"(
            {
                "array": [ 0, 0, 0 ]
            })";
        return MakeInstanceWithSomeDefaults(AZStd::make_unique<SimpleContainer_Array>(), "{}", keptDefaults);
    }

    InstanceWithoutDefaults<SimpleContainer_Array> SimpleContainer_Array::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleContainer_Array>();
        instance->m_array = {{ 181, 124, 188 }};

        const char* json = R"(
            {
                "array": [ 181, 124, 188 ]
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }
} // namespace JsonSerializationTests
