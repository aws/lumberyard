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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>

#include <QVariant>

#include <Styling/definitions.h>
#include <Styling/Selector.h>
#include <Components/StyleBus.h>

namespace GraphCanvas
{
    namespace Styling
    {

        class Style
        {
        public:
            AZ_RTTI(Style, "{61BD6B15-8AD6-4FC6-9287-9957FD076073}");
            AZ_CLASS_ALLOCATOR(Style, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            Style() = default;
            Style(const Style& other) = default;
            Style(const SelectorVector& selectors);
            Style(std::initializer_list<Selector> selectors);
            virtual ~Style() = default;

            SelectorVector GetSelectors() const { return m_selectors; }

            int Matches(const AZ::EntityId& object) const;

            virtual bool HasAttribute(Attribute attribute) const;

            QVariant GetAttribute(Attribute attribute) const;

            void SetAttribute(Attribute attribute, const QVariant& value);

            bool IsEmpty() { return m_values.empty(); }

            void Dump() const;
            AZStd::string GetSelectorsAsString() const { return m_selectorsAsString; }

        protected:
            SelectorVector Selectors() { return m_selectors; }

        private:
            void MakeSelectorsDefault();


            using ValueMap = AZStd::unordered_map<Attribute, QVariant>;

            SelectorVector m_selectors;
            AZStd::string m_selectorsAsString;
            ValueMap m_values;

            friend class StyleSheet;
        };

        using StyleVector = AZStd::vector<Style*>;


        class ComputedStyle
            : public AZ::Component
            , protected StyleRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ComputedStyle, "{695DEBB5-45A1-4CD5-B6B7-D9F7EF801194}");

            static void Reflect(AZ::ReflectContext* context);

            ComputedStyle() = default;
            ComputedStyle(const SelectorVector& objectSelectors, StyleVector&& styles);
            ComputedStyle(ComputedStyle&& other);
            ComputedStyle(const ComputedStyle& other);
            virtual ~ComputedStyle() = default;

            const SelectorVector& ObjectSelectors() { return m_objectSelectors; }

        private:
            // StyleRequestBus::Handler
            AZStd::string GetDescription() const override;

            bool HasAttribute(AZ::u32 attribute) const override;
            QVariant GetAttribute(AZ::u32 attribute) const override;
            ////

            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            ////

            SelectorVector m_objectSelectors;
            AZStd::string m_objectSelectorsAsString;
            StyleVector m_styles;
        };


        class StyleSheet
            : public AZ::Component
            , protected StyleSheetRequestBus::Handler
        {
        public:
            AZ_COMPONENT(StyleSheet, "{34B81206-2C69-4886-945B-4A9ECC0FDAEE}");

            static void Reflect(AZ::ReflectContext* context);

            StyleSheet();
            virtual ~StyleSheet();
            StyleSheet& operator=(const StyleSheet& other);
            StyleSheet& operator=(StyleSheet&& other);

            static AZ::EntityId Create();

            // AZ::Component
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("GraphCanvas_StyleService", 0x1a69884f));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                (void)dependent;
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                (void)required;
            }
            ////


        private:
            void MakeStylesDefault();

            // StyleSheetRequestBus::Handler
            AZ::EntityId ResolveStyles(const AZ::EntityId& object) const override;
            ////

            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            ////

            StyleVector m_styles;


            friend class Parser;
        };

    } // namespace Styling
} // namespace GraphCanvas