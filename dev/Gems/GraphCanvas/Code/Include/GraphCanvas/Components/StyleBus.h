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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>

#include <QVariant>

#include <GraphCanvas/Styling/Selector.h>

namespace GraphCanvas
{
    static const AZ::Crc32 StyledGraphicItemServiceCrc = AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4);

    //! StyledEntityRequests
    //! Provide details about an entity to support it being styled.
    class StyledEntityRequests : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! If this entity has a parent that is also styled, get its ID, otherwise AZ::EntityId()
        virtual AZ::EntityId GetStyleParent() const = 0;

        //! Get a set of styling selectors applicable for the entity
        virtual Styling::SelectorVector GetStyleSelectors() const = 0;

        virtual void AddSelectorState(const char* selector) = 0;
        virtual void RemoveSelectorState(const char* selector) = 0;

        //! Get the "style element" that the entity "is"; e.g. "node", "slot", "connection", etc.
        virtual AZStd::string GetElement() const = 0;
        //! Get the "style class" that the entity has. This should start with a '.' and contain [A-Za-z_-].
        virtual AZStd::string GetClass() const = 0;
        //! Get the "element ID" that the entity has. This should start with a '#' and contain [A-Za-z_-].
        virtual AZStd::string GetId() const = 0;
    };

    using StyledEntityRequestBus = AZ::EBus<StyledEntityRequests>;

    //! StyledSheetRequests
    //! Requests that can be made of a stylesheet.
    class StyleSheetRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Match the selectors an entity has against known styles and provide an aggregate meta - style for it.
        virtual AZ::EntityId ResolveStyles(const AZ::EntityId& object) const = 0;
    };

    using StyleSheetRequestBus = AZ::EBus<StyleSheetRequests>;

    //! StyleRequests
    //! Get the style for an entity (per its current state)
    class StyleRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get a textual description of the style, useful for debugging.
        virtual AZStd::string GetDescription() const = 0;

        //! Check whether the style has a given attribute.
        virtual bool HasAttribute(AZ::u32 attribute) const = 0;
        //! Get an attribute from a style. If the style lacks the attribute, QVariant() will be returned.
        virtual QVariant GetAttribute(AZ::u32 attribute) const = 0;
    };

    using StyleRequestBus = AZ::EBus<StyleRequests>;

    //! StyleNtofications
    //! Notifications about changes to the style
    class StyleNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! The style changed.
        virtual void OnStyleChanged() = 0;
    };

    using StyleNotificationBus = AZ::EBus<StyleNotifications>;

}