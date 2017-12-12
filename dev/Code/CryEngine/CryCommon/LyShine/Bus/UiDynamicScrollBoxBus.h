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

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that a dynamic scrollbox component needs to implement. A dynamic scrollbox
//! component sets up scrollbox content as a horizontal or vertical list of elements that are
//! cloned from a prototype element. Only the minimum number of elements are created for efficient
//! scrolling
class UiDynamicScrollBoxInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxInterface() {}

    // Refresh the content. Should be called when list size or element content has changed
    virtual void RefreshContent() = 0;

    // Get the index of the specified child element. Returns -1 if not found
    virtual int GetLocationIndexOfChild(AZ::EntityId childElement) = 0;

    // Get the child element at the specified location index
    virtual AZ::EntityId GetChildElementAtLocationIndex(int index) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDynamicScrollBoxInterface> UiDynamicScrollBoxBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that provides data needed to display a list of elements
class UiDynamicScrollBoxDataInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxDataInterface() {}

    //! Returns the number of elements in the list
    virtual int GetNumElements() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDynamicScrollBoxDataInterface> UiDynamicScrollBoxDataBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to receive notifications of element state
//! changes, such as when an element is about to scroll into view
class UiDynamicScrollBoxElementNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxElementNotifications(){}

    //! Called when an element is about to become visible. Used to populate the element with data
    //! for display
    virtual void OnElementBecomingVisible(AZ::EntityId entityId, int index) = 0;
};

typedef AZ::EBus<UiDynamicScrollBoxElementNotifications> UiDynamicScrollBoxElementNotificationBus;
