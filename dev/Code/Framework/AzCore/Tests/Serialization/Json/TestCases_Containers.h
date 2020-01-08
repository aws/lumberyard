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

#include <AzCore/std/containers/array.h>
// fixed_forward_list and fixed_list are not supported by the Serialize Context and the Json Serializer
// will be at best backwards compatible with the Serialize Context and over time remove types so support
// for fixed(_forward)_list will not be added.
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename Container>
    struct SimpleContainer
    {
        AZ_RTTI(SimpleContainer, "{5EE4A735-AB33-4D69-A59E-795A3FD8C7F7}");

        static const bool SupportsPartialDefaults = false;

        virtual ~SimpleContainer() = default;

        bool Equals(const SimpleContainer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleContainer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleContainer> GetInstanceWithoutDefaults();

        // Defaults for containers are always considered empty.
        Container m_container;
    };

    using SimpleContainer_Vector = SimpleContainer<AZStd::vector<int>>;
    using SimpleContainer_List = SimpleContainer<AZStd::list<int>>;
    using SimpleContainer_ForwardList = SimpleContainer<AZStd::forward_list<int>>;
    using SimpleContainer_FixedVector = SimpleContainer<AZStd::fixed_vector<int, 3>>;

    struct SimpleContainer_Array
    {
        AZ_RTTI(SimpleContainer_Array, "{4CD18CE5-4CEA-4F09-B4EE-AD612D9352BD}");

        static const bool SupportsPartialDefaults = false;

        SimpleContainer_Array();
        virtual ~SimpleContainer_Array() = default;

        bool Equals(const SimpleContainer_Array& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleContainer_Array> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleContainer_Array> GetInstanceWithoutDefaults();

        AZStd::array<int, 3> m_array;
    };

    template<typename Container>
    class ComplexContainer
    {
    public:
        AZ_RTTI(ComplexContainer, "{CBDCCA4B-271E-46C8-AB5E-64B92991CD28}");

        static const bool SupportsPartialDefaults = true;

        virtual ~ComplexContainer() = default;

        bool Equals(const ComplexContainer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<ComplexContainer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<ComplexContainer> GetInstanceWithoutDefaults();

    private:
        static SimpleNested PartialInit1();
        static SimpleNested PartialInit2();
        static SimpleNested PartialInit3();
        
        static SimpleNested FullInit1();
        static SimpleNested FullInit2();
        static SimpleNested FullInit3();

        // Defaults for containers are always considered empty.
        Container m_container;
    };

    using ComplexContainer_Vector = ComplexContainer<AZStd::vector<SimpleNested>>;
    using ComplexContainer_ForwardList = ComplexContainer<AZStd::forward_list<SimpleNested>>;
    using ComplexContainer_List = ComplexContainer<AZStd::list<SimpleNested>>;
    using ComplexContainer_FixedVector = ComplexContainer<AZStd::fixed_vector<SimpleNested, 3>>;
    using ComplexContainer_Array = ComplexContainer<AZStd::array<SimpleNested, 3>>;

} // namespace JsonSerializationTests

#include <Tests/Serialization/Json/TestCases_Containers.inl>
