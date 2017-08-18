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

#include <LyShine/Bus/UiDynamicScrollBoxBus.h>
#include <LyShine/Bus/UiScrollBoxBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This component dynamically sets up scrollbox content as a horizontal or vertical list of
//! elements that are cloned from a prototype element. Only the minimum number of elements are
//! created for efficient scrolling
class UiDynamicScrollBoxComponent
    : public AZ::Component
    , public UiDynamicScrollBoxBus::Handler
    , public UiScrollBoxNotificationBus::Handler
    , public UiInitializationBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiElementNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiDynamicScrollBoxComponent, LyShine::UiDynamicScrollBoxComponentUuid, AZ::Component);

    UiDynamicScrollBoxComponent();
    ~UiDynamicScrollBoxComponent() override;

    // UiDynamicScrollBoxInterface
    virtual void RefreshContent() override;
    virtual int GetLocationIndexOfChild(AZ::EntityId childElement) override;
    virtual AZ::EntityId GetChildElementAtLocationIndex(int index) override;
    // ~UiDynamicScrollBoxInterface

    // UiScrollBoxNotifications
    virtual void OnScrollOffsetChanging(AZ::Vector2 newScrollOffset) override;
    virtual void OnScrollOffsetChanged(AZ::Vector2 newScrollOffset) override;
    // ~UiScrollBoxNotifications

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotification

    // UiElementNotifications
    void OnUiElementBeingDestroyed() override;
    // ~UiElementNotifications

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiDynamicScrollBoxService", 0x11112f1a));
        provided.push_back(AZ_CRC("UiDynamicContentService", 0xc5af0b83));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiDynamicContentService", 0xc5af0b83));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
        required.push_back(AZ_CRC("UiScrollBoxService", 0xfdafc904));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: //types

    struct ElementEntry
    {
        AZ::EntityId m_element;
        int m_index;
    };

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiDynamicScrollBoxComponent);

    // Get the content entity of the scrollbox
    AZ::Entity* GetContentEntity();

    // Resize the content entity to fit the desired number of elements
    void ResizeContentToFitElements();

    // Mark all elements as not visible
    void ClearVisibleElements();

    // Update which elements are visible
    void UpdateElementVisibility();

    // Calculate the first and last visible element indexes
    void CalculateVisibleElementIndexes(int& firstVisibleElementIndex, int& lastVisibleElementIndex);

    // Return whether the element at the specified index is visible
    bool IsElementVisibleAtIndex(int index);

    // Get a recycled element or a new element for display
    AZ::EntityId GetElementForDisplay();

    // Set an element's position based on index
    void PositionElementAtIndex(AZ::EntityId element, int index);

    // Get the child of the content element that contains the specified descendant
    AZ::EntityId GetImmediateContentChildFromDescendant(AZ::EntityId childElement);

protected: // data

    //! The entity Id of the prototype element
    AZ::EntityId m_prototypeElement;

    //! Number of elements by default. Overwritten by UiDynamicListDataBus::GetNumElements
    int m_defaultNumElements;

    //! Stores the size of the prototype element before it is removed from the content element's
    //! child list. Used to calculate the content size
    AZ::Vector2 m_prototypeElementSize;

    //! A list of currently visible elements
    AZStd::list<ElementEntry> m_visibleElementEntries;

    //! A list of unused elements
    AZStd::list<AZ::EntityId> m_recycledElements;

    //! First and last visible element indexes
    int m_firstVisibleElementIndex;
    int m_lastVisibleElementIndex;

    //! The virtual number of elements
    int m_numElements;
};
