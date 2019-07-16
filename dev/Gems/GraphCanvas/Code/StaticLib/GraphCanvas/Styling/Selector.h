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

#include <AzCore/std/string/string.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace GraphCanvas
{
    namespace Styling
    {
        class SelectorImplementation
        {
        public:
            AZ_RTTI(SelectorImplementation, "{A37E7DCF-4F24-4969-A030-D72750B1213D}");
            AZ_CLASS_ALLOCATOR(SelectorImplementation, AZ::SystemAllocator, 0);

            virtual ~SelectorImplementation() = default;

            static void Reflect(AZ::ReflectContext* context);

            virtual int GetComplexity() const { return{}; }
            virtual bool Matches(const AZ::EntityId& object) const { return{}; }
            virtual AZStd::string ToString() const { return{}; }

            virtual bool operator==(const SelectorImplementation& other) const { return{}; }

            virtual SelectorImplementation* Clone() const { return{}; }
        };

        class Selector
        {
        public:
            AZ_TYPE_INFO(Selector, "{A0BF8631-31C3-4BC8-9D7A-09DF2AD611DB}");
            AZ_CLASS_ALLOCATOR(Selector, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            static const Selector Get(const AZStd::string& selector);

            Selector() = default;
            Selector(SelectorImplementation* actual);
            Selector(const Selector& other);
            virtual ~Selector() = default;

            Selector& operator=(const Selector& other);
            Selector& operator=(Selector&& other);
            Selector& operator=(SelectorImplementation* implementation)
            {
                m_actual.reset(implementation);
                return *this;
            }

            virtual int GetComplexity() const { return m_actual->GetComplexity(); }
            virtual bool Matches(const AZ::EntityId& object) const { return m_actual->Matches(object); }
            virtual AZStd::string ToString() const { return m_actual->ToString(); }

            virtual bool operator==(const Selector& other) const
            {
                return *m_actual == *other.m_actual;
            }
            virtual bool operator==(const SelectorImplementation& other) const
            {
                return *m_actual == other;
            }

            bool IsValid() { return m_actual != nullptr; }

            void MakeDefault();

        private:

            void MakeNull();

            AZStd::unique_ptr<SelectorImplementation> m_actual;
        };

        using SelectorVector = AZStd::vector<Selector>;


        AZStd::string SelectorToStringAccumulator(const AZStd::string& soFar, const Selector& selector);

        AZStd::string SelectorsToString(const AZStd::vector<Selector>& selectors);
    }
}