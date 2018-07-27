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
#include "LyShine_precompiled.h"
#include "UiDynamicScrollBoxComponent.h"

#include "UiElementComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiCanvasBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDynamicScrollBoxDataBus Behavior context handler class
class BehaviorUiDynamicScrollBoxDataBusHandler
    : public UiDynamicScrollBoxDataBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiDynamicScrollBoxDataBusHandler, "{74FA95AB-D4C2-40B8-8568-1B174BF577C0}", AZ::SystemAllocator,
        GetNumElements);

    int GetNumElements() override
    {
        int numElements = 0;
        CallResult(numElements, FN_GetNumElements);
        return numElements;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDynamicScrollBoxElementNotificationBus Behavior context handler class
class BehaviorUiDynamicScrollBoxElementNotificationBusHandler
    : public UiDynamicScrollBoxElementNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiDynamicScrollBoxElementNotificationBusHandler, "{4D166273-4D12-45A4-BC42-A7FF59A2092E}", AZ::SystemAllocator,
        OnElementBecomingVisible);

    void OnElementBecomingVisible(AZ::EntityId entityId, int index) override
    {
        Call(FN_OnElementBecomingVisible, entityId, index);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::UiDynamicScrollBoxComponent()
    : m_defaultNumElements(0)
    , m_firstVisibleElementIndex(-1)
    , m_lastVisibleElementIndex(-1)
    , m_numElements(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::~UiDynamicScrollBoxComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::RefreshContent()
{
    ResizeContentToFitElements();

    ClearVisibleElements();

    UpdateElementVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::GetLocationIndexOfChild(AZ::EntityId childElement)
{
    AZ::EntityId immediateChild = GetImmediateContentChildFromDescendant(childElement);

    for (auto e : m_visibleElementEntries)
    {
        if (e.m_element == immediateChild)
        {
            return e.m_index;
        }
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetChildElementAtLocationIndex(int index)
{
    AZ::EntityId elementId;

    for (auto e : m_visibleElementEntries)
    {
        if (e.m_index == index)
        {
            elementId = e.m_element;
            break;
        }
    }

    return elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnScrollOffsetChanging(AZ::Vector2 newScrollOffset)
{
    UpdateElementVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnScrollOffsetChanged(AZ::Vector2 newScrollOffset)
{
    UpdateElementVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::InGamePostActivate()
{
    // Find the content element
    AZ::EntityId contentEntityId;
    EBUS_EVENT_ID_RESULT(contentEntityId, GetEntityId(), UiScrollBoxBus, GetContentEntity);

    // Find the prototype element as the first child of the content element
    int numChildren = 0;
    EBUS_EVENT_ID_RESULT(numChildren, contentEntityId, UiElementBus, GetNumChildElements);

    if (numChildren > 0)
    {
        AZ::Entity* prototypeEntity = nullptr;
        EBUS_EVENT_ID_RESULT(prototypeEntity, contentEntityId, UiElementBus, GetChildElement, 0);

        if (prototypeEntity)
        {
            // Store the prototype element for future cloning
            m_prototypeElement = prototypeEntity->GetId();

            // Store the size of the prototype element for future content element size calculations
            EBUS_EVENT_ID_RESULT(m_prototypeElementSize, m_prototypeElement, UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

            AZ::Entity* contentEntity = GetContentEntity();
            if (contentEntity)
            {
                // Get the content entity's element component
                UiElementComponent* elementComponent = contentEntity->FindComponent<UiElementComponent>();
                AZ_Assert(elementComponent, "entity has no UiElementComponent");

                // Remove any extra elements
                if (numChildren > 1)
                {
                    for (int i = numChildren - 1; i > 0; i--)
                    {
                        // Remove the child element
                        AZ::Entity* element = nullptr;
                        EBUS_EVENT_ID_RESULT(element, contentEntityId, UiElementBus, GetChildElement, i);
                        if (element)
                        {
                            elementComponent->RemoveChild(element);
                            EBUS_EVENT_ID(element->GetId(), UiElementBus, DestroyElement);
                        }
                    }
                }

                // Remove the prototype element
                elementComponent->RemoveChild(prototypeEntity);

                // Refresh the content
                RefreshContent();

                // Listen for canvas space rect changes of the content's parent
                AZ::Entity* contentParentEntity = nullptr;
                EBUS_EVENT_ID_RESULT(contentParentEntity, contentEntityId, UiElementBus, GetParent);
                if (contentParentEntity)
                {
                    UiTransformChangeNotificationBus::Handler::BusConnect(contentParentEntity->GetId());
                }

                // Listen to scrollbox scrolling events
                UiScrollBoxNotificationBus::Handler::BusConnect(GetEntityId());
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        UpdateElementVisibility();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnUiElementBeingDestroyed()
{
    if (m_prototypeElement.IsValid())
    {
        EBUS_EVENT_ID(m_prototypeElement, UiElementBus, DestroyElement);
        m_prototypeElement.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiDynamicScrollBoxComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDynamicScrollBoxComponent, AZ::Component>()
            ->Version(1)
            ->Field("DefaultNumElements", &UiDynamicScrollBoxComponent::m_defaultNumElements);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDynamicScrollBoxComponent>("DynamicScrollBox",
                    "A component that dynamically sets up scroll box content as a horizontal or vertical list of elements that\n"
                    "are cloned from a prototype element. Only the minimum number of elements are created for efficient scrolling.\n"
                    "The scroll box's content element's first child acts as the prototype element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDynamicScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDynamicScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiDynamicScrollBoxComponent::m_defaultNumElements, "Default Num Elements",
                "The default number of elements in the list.")
                ->Attribute(AZ::Edit::Attributes::Min, 0);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiDynamicScrollBoxBus>("UiDynamicScrollBoxBus")
            ->Event("RefreshContent", &UiDynamicScrollBoxBus::Events::RefreshContent)
            ->Event("GetLocationIndexOfChild", &UiDynamicScrollBoxBus::Events::GetLocationIndexOfChild)
            ->Event("GetChildElementAtLocationIndex", &UiDynamicScrollBoxBus::Events::GetChildElementAtLocationIndex);

        behaviorContext->EBus<UiDynamicScrollBoxDataBus>("UiDynamicScrollBoxDataBus")
            ->Handler<BehaviorUiDynamicScrollBoxDataBusHandler>();

        behaviorContext->EBus<UiDynamicScrollBoxElementNotificationBus>("UiDynamicScrollBoxElementNotificationBus")
            ->Handler<BehaviorUiDynamicScrollBoxElementNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::Activate()
{
    UiDynamicScrollBoxBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
    UiElementNotificationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::Deactivate()
{
    UiDynamicScrollBoxBus::Handler::BusDisconnect();
    UiInitializationBus::Handler::BusDisconnect();
    if (UiTransformChangeNotificationBus::Handler::BusIsConnected())
    {
        UiTransformChangeNotificationBus::Handler::BusDisconnect();
    }
    if (UiScrollBoxNotificationBus::Handler::BusIsConnected())
    {
        UiScrollBoxNotificationBus::Handler::BusDisconnect();
    }
    UiElementNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiDynamicScrollBoxComponent::GetContentEntity()
{
    AZ::Entity* contentEntity = nullptr;

    // Find the content element
    AZ::EntityId contentEntityId;
    EBUS_EVENT_ID_RESULT(contentEntityId, GetEntityId(), UiScrollBoxBus, GetContentEntity);

    if (contentEntityId.IsValid())
    {
        EBUS_EVENT_RESULT(contentEntity, AZ::ComponentApplicationBus, FindEntity, contentEntityId);
    }

    return contentEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ResizeContentToFitElements()
{
    // Find the content element
    AZ::EntityId contentEntityId;
    EBUS_EVENT_ID_RESULT(contentEntityId, GetEntityId(), UiScrollBoxBus, GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return;
    }

    if (!m_prototypeElement.IsValid())
    {
        return;
    }

    // Get the number of elements in the list
    m_numElements = m_defaultNumElements;
    EBUS_EVENT_ID_RESULT(m_numElements, GetEntityId(), UiDynamicScrollBoxDataBus, GetNumElements);

    // Get current content size
    AZ::Vector2 curSize(0.0f, 0.0f);
    EBUS_EVENT_ID_RESULT(curSize, contentEntityId, UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

    // Calculate new content size
    AZ::Vector2 newSize = curSize;

    bool isVertical = true;
    EBUS_EVENT_ID_RESULT(isVertical, GetEntityId(), UiScrollBoxBus, GetIsVerticalScrollingEnabled);

    if (isVertical)
    {
        newSize.SetY(m_numElements * m_prototypeElementSize.GetY());
    }
    else
    {
        newSize.SetX(m_numElements * m_prototypeElementSize.GetX());
    }

    // Resize content element
    if (curSize != newSize)
    {
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, contentEntityId, UiTransform2dBus, GetOffsets);

        AZ::Vector2 pivot;
        EBUS_EVENT_ID_RESULT(pivot, contentEntityId, UiTransformBus, GetPivot);

        AZ::Vector2 sizeDiff = newSize - curSize;

        bool offsetsChanged = false;
        if (sizeDiff.GetX() != 0.0f)
        {
            offsets.m_left -= sizeDiff.GetX() * pivot.GetX();
            offsets.m_right += sizeDiff.GetX() * (1.0f - pivot.GetX());
            offsetsChanged = true;
        }
        if (sizeDiff.GetY() != 0.0f)
        {
            offsets.m_top -= sizeDiff.GetY() * pivot.GetY();
            offsets.m_bottom += sizeDiff.GetY() * (1.0f - pivot.GetY());
            offsetsChanged = true;
        }

        if (offsetsChanged)
        {
            EBUS_EVENT_ID(contentEntityId, UiTransform2dBus, SetOffsets, offsets);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ClearVisibleElements()
{
    for (auto e : m_visibleElementEntries)
    {
        m_recycledElements.push_front(e.m_element);

        // Disable element
        EBUS_EVENT_ID(e.m_element, UiElementBus, SetIsEnabled, false);
    }

    m_visibleElementEntries.clear();

    m_firstVisibleElementIndex = -1;
    m_lastVisibleElementIndex = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::UpdateElementVisibility()
{
    int firstVisibleElementIndex = -1;
    int lastVisibleElementIndex = -1;
    CalculateVisibleElementIndexes(firstVisibleElementIndex, lastVisibleElementIndex);

    // Remove the elements that are no longer visible
    m_visibleElementEntries.remove_if(
        [this, firstVisibleElementIndex, lastVisibleElementIndex](const ElementEntry& e)
        {
            if ((firstVisibleElementIndex < 0) || (e.m_index < firstVisibleElementIndex) || (e.m_index > lastVisibleElementIndex))
            {
                // This element is no longer visible, move it to the recycled elements list
                m_recycledElements.push_front(e.m_element);

                // Disable element
                EBUS_EVENT_ID(e.m_element, UiElementBus, SetIsEnabled, false);

                // Remove element from the visible element list
                return true;
            }
            else
            {
                return false;
            }
        }
        );

    // Add the newly visible elements
    if (firstVisibleElementIndex >= 0)
    {
        for (int i = firstVisibleElementIndex; i <= lastVisibleElementIndex; i++)
        {
            if (!IsElementVisibleAtIndex(i))
            {
                AZ::EntityId element = GetElementForDisplay();
                ElementEntry elementEntry;
                elementEntry.m_element = element;
                elementEntry.m_index = i;

                m_visibleElementEntries.push_front(elementEntry);

                PositionElementAtIndex(element, i);

                // Notify listeners that this element is about to become visible
                EBUS_EVENT_ID(GetEntityId(), UiDynamicScrollBoxElementNotificationBus, OnElementBecomingVisible, element, i);
            }
        }
    }

    m_firstVisibleElementIndex = firstVisibleElementIndex;
    m_lastVisibleElementIndex = lastVisibleElementIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::CalculateVisibleElementIndexes(int& firstVisibleElementIndex, int& lastVisibleElementIndex)
{
    firstVisibleElementIndex = -1;
    lastVisibleElementIndex = -1;

    // Find the content element
    AZ::EntityId contentEntityId;
    EBUS_EVENT_ID_RESULT(contentEntityId, GetEntityId(), UiScrollBoxBus, GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return;
    }

    if (!m_prototypeElement.IsValid())
    {
        return;
    }

    if (m_numElements == 0)
    {
        return;
    }

    // Get content's parent
    AZ::Entity* contentParentEntity = nullptr;
    EBUS_EVENT_ID_RESULT(contentParentEntity, contentEntityId, UiElementBus, GetParent);
    if (!contentParentEntity)
    {
        return;
    }

    // Get content's rect in canvas space
    UiTransformInterface::Rect contentRect;
    EBUS_EVENT_ID(contentEntityId, UiTransformBus, GetCanvasSpaceRectNoScaleRotate, contentRect);

    // Get content parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    EBUS_EVENT_ID(contentParentEntity->GetId(), UiTransformBus, GetCanvasSpaceRectNoScaleRotate, parentRect);

    // Check if any items are visible
    AZ::Vector2 minA(contentRect.left, contentRect.top);
    AZ::Vector2 maxA(contentRect.right, contentRect.bottom);
    AZ::Vector2 minB(parentRect.left, parentRect.top);
    AZ::Vector2 maxB(parentRect.right, parentRect.bottom);

    bool boxesIntersect = true;

    if (maxA.GetX() < minB.GetX() || // a is left of b
        minA.GetX() > maxB.GetX() || // a is right of b
        maxA.GetY() < minB.GetY() || // a is above b
        minA.GetY() > maxB.GetY())   // a is below b
    {
        boxesIntersect = false;   // no overlap
    }

    if (boxesIntersect)
    {
        // Some items are visible

        bool isVertical = true;
        EBUS_EVENT_ID_RESULT(isVertical, GetEntityId(), UiScrollBoxBus, GetIsVerticalScrollingEnabled);

        if (isVertical)
        {
            if (m_prototypeElementSize.GetY() > 0.0f)
            {
                // Calculate first visible element index
                float topOffset = parentRect.top - contentRect.top;
                firstVisibleElementIndex = max(static_cast<int>(ceil(topOffset / m_prototypeElementSize.GetY())) - 1, 0);

                // Calculate last visible element index
                float bottomOffset = parentRect.bottom - contentRect.top;
                lastVisibleElementIndex = static_cast<int>(ceil(bottomOffset / m_prototypeElementSize.GetY())) - 1;
                int lastElementIndex = max(m_numElements - 1, 0);
                Limit(lastVisibleElementIndex, 0, lastElementIndex);
            }
        }
        else
        {
            if (m_prototypeElementSize.GetX() > 0.0f)
            {
                // Calculate first visible element index
                float leftOffset = parentRect.left - contentRect.left;
                firstVisibleElementIndex = max(static_cast<int>(ceil(leftOffset / m_prototypeElementSize.GetX())) - 1, 0);

                // Calculate last visible element index
                float rightOffset = parentRect.right - contentRect.left;
                lastVisibleElementIndex = static_cast<int>(ceil(rightOffset / m_prototypeElementSize.GetX())) - 1;
                int lastElementIndex = max(m_numElements - 1, 0);
                Limit(lastVisibleElementIndex, 0, lastElementIndex);
            }
        }

        // Add an extra element on each end to be able to navigate to all elements using gamepad/keyboard
        if (firstVisibleElementIndex > 0)
        {
            firstVisibleElementIndex--;
        }

        if (lastVisibleElementIndex > -1 && lastVisibleElementIndex < m_numElements - 1)
        {
            lastVisibleElementIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::IsElementVisibleAtIndex(int index)
{
    if (m_firstVisibleElementIndex < 0)
    {
        return false;
    }

    return ((index >= m_firstVisibleElementIndex) && (index <= m_lastVisibleElementIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetElementForDisplay()
{
    AZ::EntityId element;

    // Check if there is an existing element
    if (!m_recycledElements.empty())
    {
        element = m_recycledElements.front();
        m_recycledElements.pop_front();

        // Enable element
        EBUS_EVENT_ID(element, UiElementBus, SetIsEnabled, true);
    }
    else
    {
        // Clone the prototype element and add it as a child of the content element
        AZ::Entity* prototypeEntity = nullptr;
        EBUS_EVENT_RESULT(prototypeEntity, AZ::ComponentApplicationBus, FindEntity, m_prototypeElement);
        if (prototypeEntity)
        {
            // Find the content element
            AZ::EntityId contentEntityId;
            EBUS_EVENT_ID_RESULT(contentEntityId, GetEntityId(), UiScrollBoxBus, GetContentEntity);

            AZ::Entity* contentEntity = GetContentEntity();
            if (contentEntity)
            {
                AZ::EntityId canvasEntityId;
                EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);

                AZ::Entity* clonedElement = nullptr;
                EBUS_EVENT_ID_RESULT(clonedElement, canvasEntityId, UiCanvasBus, CloneElement, prototypeEntity, contentEntity);

                if (clonedElement)
                {
                    element = clonedElement->GetId();
                }
            }
        }
    }

    return element;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::PositionElementAtIndex(AZ::EntityId element, int index)
{
    bool isVertical = true;
    EBUS_EVENT_ID_RESULT(isVertical, GetEntityId(), UiScrollBoxBus, GetIsVerticalScrollingEnabled);

    // Get the element anchors
    UiTransform2dInterface::Anchors anchors;
    EBUS_EVENT_ID_RESULT(anchors, element, UiTransform2dBus, GetAnchors);

    if (isVertical)
    {
        // Set anchors to top of parent
        anchors.m_top = 0.0f;
        anchors.m_bottom = 0.0f;
    }
    else
    {
        // Set anchors to left of parent
        anchors.m_left = 0.0f;
        anchors.m_right = 0.0f;
    }

    EBUS_EVENT_ID(element, UiTransform2dBus, SetAnchors, anchors, false, false);

    // Get the element offsets
    UiTransform2dInterface::Offsets offsets;
    EBUS_EVENT_ID_RESULT(offsets, element, UiTransform2dBus, GetOffsets);

    // Position offsets based on index
    if (isVertical)
    {
        float height = offsets.m_bottom - offsets.m_top;
        offsets.m_top = m_prototypeElementSize.GetY() * index;
        offsets.m_bottom = offsets.m_top + height;
    }
    else
    {
        float width = offsets.m_right - offsets.m_left;
        offsets.m_left = m_prototypeElementSize.GetX() * index;
        offsets.m_right = offsets.m_left + width;
    }

    EBUS_EVENT_ID(element, UiTransform2dBus, SetOffsets, offsets);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetImmediateContentChildFromDescendant(AZ::EntityId childElement)
{
    AZ::EntityId immediateChild;

    AZ::Entity* contentEntity = GetContentEntity();
    if (contentEntity)
    {
        immediateChild = childElement;
        AZ::Entity* parent = nullptr;
        EBUS_EVENT_ID_RESULT(parent, immediateChild, UiElementBus, GetParent);
        while (parent && parent != contentEntity)
        {
            immediateChild = parent->GetId();
            EBUS_EVENT_ID_RESULT(parent, immediateChild, UiElementBus, GetParent);
        }

        if (parent != contentEntity)
        {
            immediateChild.SetInvalid();
        }
    }

    return immediateChild;
}
