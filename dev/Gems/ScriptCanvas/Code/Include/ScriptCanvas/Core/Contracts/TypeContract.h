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

#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class TypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(TypeContract, AZ::SystemAllocator, 0);
        AZ_RTTI(TypeContract, "{92343025-F306-4457-B646-1E0989521D2C}", Contract);

        enum Flags : int
        {
            Inclusive, //> Contract will be satisfied by any of the type Uuids stored in the contract.
            Exclusive, //> Contract may satisfy any endpoint except to those types in the contract.
        };

        TypeContract(Flags flags = Inclusive) : m_flags(flags) {}
        TypeContract(std::initializer_list<Data::Type> types, Flags flags = Inclusive)
            : TypeContract(types.begin(), types.end(), flags)
        {}
        template<typename Container>
        TypeContract(const Container& types, Flags flags = Inclusive)
            : TypeContract(types.begin(), types.end(), flags)
        {}
        template<typename InputIterator>
        TypeContract(InputIterator first, InputIterator last, Flags flags = Inclusive)
            : m_types(first, last)
            , m_flags(flags)
        {}
        ~TypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

        void AddType(Data::Type&& type) { m_types.emplace_back(AZStd::move(type)); }
    protected:
        Flags m_flags;
        AZStd::vector<Data::Type> m_types;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
