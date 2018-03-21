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
#include "UiElementComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "UiCanvasComponent.h"
#include <LyShine/IDraw2d.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiEditorBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiUpdateBus.h>
#include <LyShine/Bus/UiInteractionMaskBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiEntityContextBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

#include "UiTransform2dComponent.h"

#include "IConsole.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementComponent::UiElementComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementComponent::~UiElementComponent()
{
    // In normal (correct) usage we have nothing to do here.
    // But if a user calls DeleteEntity or just deletes an entity pointer they can delete a UI element
    // and leave its parent with a dangling child pointer.
    // So we report an error in that case and do some recovery code.

    // If we were being deleted via DestroyElement m_parentId would be invalid 
    if (m_parentId.IsValid())
    {
        // Note we do not rely on the m_parent pointer because if the canvas is being unloaded for example the
        // parent entity could already have been deleted. So we use the parent entity Id to try to find the parent.
        AZ::Entity* parent = nullptr;
        EBUS_EVENT_RESULT(parent, AZ::ComponentApplicationBus, FindEntity, m_parentId);

        // If the parent is found and it is active that suggests something is wrong. When unloading a canvas we
        // deactivate all of the UI elements before any are deleted
        if (parent && parent->GetState() == AZ::Entity::ES_ACTIVE)
        {
            // As a final check see if this element's parent thinks that this is a child, this is almost certain to be the
            // case if we got here but, if not, there is nothing more to do
            UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
            if (parentElementComponent)
            {
                if (parentElementComponent->FindChildByEntityId(GetEntityId()))
                {
                    // This is an error, report the error
                    AZ_Error("UI", false, "Deleting a UI element entity directly rather than using DestroyElement. Element is named '%s'", m_entity->GetName().c_str());

                    // Attempt to recover by removing this element from the parent's child list
                    parentElementComponent->RemoveChild(m_entity);

                    // And recursively delete any child UI elements (like DestroyElement on this element would have done)
                    auto childElementComponents = m_childElementComponents;
                    for (auto child : childElementComponents)
                    {
                        // destroy the child
                        child->DestroyElement();
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::UpdateElement(float deltaTime)
{
    if (!m_isEnabled)
    {
        // Nothing to do - whole element and all children are disabled
        return;
    }

    // update any components connected to the UiUpdateBus
    EBUS_EVENT_ID(GetEntityId(), UiUpdateBus, Update, deltaTime);

    if (AreChildPointersValid())
    {
        // now update child elements
        int numChildren = m_children.size();
        for (int i = 0; i < numChildren; ++i)
        {
            GetChildElementComponent(i)->UpdateElement(deltaTime);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RenderElement(bool isInGame, bool displayBounds)
{
    if (!IsFullyInitialized())
    {
        return;
    }

    if (!isInGame)
    {
        // We are in editing mode (not running the game)
        // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
        // hidden in the editor
        bool isVisible = true;
        EBUS_EVENT_ID_RESULT(isVisible, GetEntityId(), UiEditorBus, GetIsVisible);
        if (!isVisible)
        {
            return;
        }
    }

    if (!m_isEnabled)
    {
        // Nothing to do - whole element and all children are disabled
        return;
    }

    // This is to allow components to update within the editor window
    if (!isInGame)
    {
        EBUS_EVENT_ID(GetEntityId(), UiUpdateBus, UpdateInEditor);
    }

    // give any components connected to the UiRenderControlBus the opportunity to
    // change render state
    EBUS_EVENT_ID(GetEntityId(), UiRenderControlBus,
        SetupBeforeRenderingComponents, UiRenderControlInterface::Pass::First);

    // render any components connected to the UiRenderBus
    EBUS_EVENT_ID(GetEntityId(), UiRenderBus, Render);

    // give any components connected to the UiRenderControlBus the opportunity to take
    // action after the components are rendered and before the children are
    EBUS_EVENT_ID(GetEntityId(), UiRenderControlBus,
        SetupAfterRenderingComponents, UiRenderControlInterface::Pass::First);

    // now render child elements
    int numChildren = m_children.size();
    for (int i = 0; i < numChildren; ++i)
    {
        GetChildElementComponent(i)->RenderElement(isInGame, displayBounds);
    }

    // give any components connected to the UiRenderControlBus the opportunity to take
    // action after children are rendered
    bool isSecondComponentsPassRequired = false;
    EBUS_EVENT_ID(GetEntityId(), UiRenderControlBus,
        SetupAfterRenderingChildren, isSecondComponentsPassRequired);

    if (isSecondComponentsPassRequired)
    {
        // change render state for second pass of components render
        EBUS_EVENT_ID(GetEntityId(), UiRenderControlBus,
            SetupBeforeRenderingComponents, UiRenderControlInterface::Pass::Second);

        // render any components connected to the UiRenderBus
        EBUS_EVENT_ID(GetEntityId(), UiRenderBus, Render);

        // change render state after second pass of components render
        EBUS_EVENT_ID(GetEntityId(), UiRenderControlBus,
            SetupAfterRenderingComponents, UiRenderControlInterface::Pass::Second);
    }

    // We use deferred render for the bounds so that they draw on top of everything else
    if (displayBounds && m_parent != nullptr)
    {
        AZ::Color white(1.0f, 1.0f, 1.0f, 1.0f);
        UiTransformInterface::RectPoints points;
        GetTransform2dComponent()->GetViewportSpacePoints(points);
        // Note: because we pass true to defer these renders the outlines will draw on top of everything on the canvas
        // This is a nested call to begin 2d drawing but Draw2d supports that.
        Draw2dHelper draw2d(true);
        draw2d.DrawLine(points.TopLeft(), points.TopRight(), white);
        draw2d.DrawLine(points.TopRight(), points.BottomRight(), white);
        draw2d.DrawLine(points.BottomRight(), points.BottomLeft(), white);
        draw2d.DrawLine(points.BottomLeft(), points.TopLeft(), white);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::ElementId UiElementComponent::GetElementId()
{
    return m_elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::NameType UiElementComponent::GetName()
{
    return GetEntity() ? GetEntity()->GetName() : "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetCanvasEntityId()
{
    return m_canvas ? m_canvas->GetEntityId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::GetParent()
{
    return m_parent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetParentEntityId()
{
    return m_parentId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetNumChildElements()
{
    return m_children.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::GetChildElement(int index)
{
    AZ::Entity* childEntity = nullptr;
    if (index >= 0 && index < m_children.size())
    {
        if (AreChildPointersValid())
        {
            childEntity = GetChildElementComponent(index)->GetEntity();
        }
        else
        {
            EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, m_children[index]);
        }
    }
    return childEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetChildEntityId(int index)
{
    AZ::EntityId childEntityId;
    if (index >= 0 && index < m_children.size())
    {
        childEntityId = m_children[index];
    }
    return childEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetIndexOfChild(const AZ::Entity* child)
{
    AZ::EntityId childEntityId = child->GetId();
    auto p = std::find(m_children.begin(), m_children.end(), childEntityId);
    AZ_Assert(p != m_children.end(), "The given entity is not a child of this UI element");
    return static_cast<int>(std::distance(m_children.begin(), p));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetIndexOfChildByEntityId(AZ::EntityId childId)
{
    auto p = std::find(m_children.begin(), m_children.end(), childId);
    AZ_Assert(p != m_children.end(), "The given entity is not a child of this UI element");
    return static_cast<int>(std::distance(m_children.begin(), p));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiElementComponent::GetChildElements()
{
    int numChildren = m_children.size();
    LyShine::EntityArray children;
    children.reserve(numChildren);

    // This is one of the rare functions that needs to work before FixupPostLoad has been called because it is called
    // from OnSliceInstantiated, so only use m_childElementComponents if it is setup
    if (AreChildPointersValid())
    {
        for (int i = 0; i < numChildren; ++i)
        {
            children.push_back(GetChildElementComponent(i)->GetEntity());
        }
    }
    else
    {
        for (auto child : m_children)
        {
            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);
            if (childEntity)
            {
                children.push_back(childEntity);
            }
        }
    }

    return children;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiElementComponent::GetChildEntityIds()
{
    return m_children;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::CreateChildElement(const LyShine::NameType& name)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_ID_RESULT(contextId, GetEntityId(), AzFramework::EntityIdContextQueryBus, GetOwningContextId);

    AZ::Entity* child = nullptr;
    EBUS_EVENT_ID_RESULT(child, contextId, UiEntityContextRequestBus, CreateUiEntity, name.c_str());
    AZ_Assert(child, "Failed to create child entity");

    child->Deactivate();    // deactivate so that we can add components

    UiElementComponent* elementComponent = child->CreateComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "Failed to create UiElementComponent");

    elementComponent->m_canvas = m_canvas;
    elementComponent->SetParentReferences(m_entity, this);
    elementComponent->m_elementId = m_canvas->GenerateId();

    child->Activate();      // re-activate

    if (AreChildPointersValid())    // must test before m_children.push_back
    {
        m_childElementComponents.push_back(elementComponent);
    }
    m_children.push_back(child->GetId());

    return child;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::DestroyElement()
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_ID_RESULT(contextId, GetEntityId(), AzFramework::EntityIdContextQueryBus, GetOwningContextId);

    // destroy child elements, this is complicated by the fact that the child elements
    // will attempt to remove themselves from the m_children list in their DestroyElement method.
    // But, if the entities are not initialized yet the child parent pointer will be null.
    // So the child may or may not remove itself from the list.
    // So make a local copy of the list and iterate on that
    if (AreChildPointersValid())
    {
        auto childElementComponents = m_childElementComponents;
        for (auto child : childElementComponents)
        {
            // destroy the child
            child->DestroyElement();
        }
    }
    else
    {
        auto children = m_children;
        for (auto child : children)
        {
            // destroy the child
            EBUS_EVENT_ID(child, UiElementBus, DestroyElement);
        }
    }

    // remove this element from parent
    if (m_parent)
    {
        GetParentElementComponent()->RemoveChild(GetEntity());
    }

    // Notify listeners that the element is being destroyed
    EBUS_EVENT_ID(GetEntityId(), UiElementNotificationBus, OnUiElementBeingDestroyed);

    EBUS_EVENT_ID(contextId, UiEntityContextRequestBus, DestroyUiEntity, GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Reparent(AZ::Entity* newParent, AZ::Entity* insertBefore)
{
    if (!newParent)
    {
        if (IsFullyInitialized())
        {
            newParent = GetCanvasComponent()->GetRootElement();
        }
        else
        {
            EmitNotInitializedWarning();
            return;
        }
    }

    if (newParent == GetEntity())
    {
        AZ_Warning("UI", false, "Cannot set an entity's parent to itself")
        return;
    }

    UiElementComponent* newParentElement = newParent->FindComponent<UiElementComponent>();
    AZ_Assert(newParentElement, "New parent entity has no UiElementComponent");

    // check if the new parent is in a different canvas if so a reparent is not allowed
    // and the caller should do a CloneElement and DestroyElement
    if (m_canvas != newParentElement->m_canvas)
    {
        AZ_Warning("UI", false, "Reparent: Cannot reparent an element to a different canvas");
        return;
    }

    if (m_parent)
    {
        // remove from parent
        GetParentElementComponent()->RemoveChild(GetEntity());
    }

    newParentElement->AddChild(GetEntity(), insertBefore);

    SetParentReferences(newParent, newParentElement);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::ReparentByEntityId(AZ::EntityId newParent, AZ::EntityId insertBefore)
{
    AZ::Entity* newParentEntity = nullptr;
    if (newParent.IsValid())
    {
        EBUS_EVENT_RESULT(newParentEntity, AZ::ComponentApplicationBus, FindEntity, newParent);
        AZ_Assert(newParentEntity, "Entity for newParent not found");
    }

    AZ::Entity* insertBeforeEntity = nullptr;
    if (insertBefore.IsValid())
    {
        EBUS_EVENT_RESULT(insertBeforeEntity, AZ::ComponentApplicationBus, FindEntity, insertBefore);
        AZ_Assert(insertBeforeEntity, "Entity for insertBefore not found");
    }

    Reparent(newParentEntity, insertBeforeEntity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::AddToParentAtIndex(AZ::Entity* newParent, int index)
{
    AZ_Assert(!m_parent, "Element already has a parent");

    if (!newParent)
    {
        if (IsFullyInitialized())
        {
            newParent = GetCanvasComponent()->GetRootElement();
        }
        else
        {
            EmitNotInitializedWarning();
            return;
        }
    }

    UiElementComponent* newParentElement = newParent->FindComponent<UiElementComponent>();
    AZ_Assert(newParentElement, "New parent entity has no UiElementComponent");

    AZ::Entity* insertBefore = nullptr;
    if (index >= 0 && index < newParentElement->GetNumChildElements())
    {
        insertBefore = newParentElement->GetChildElement(index);
    }

    newParentElement->AddChild(GetEntity(), insertBefore);

    SetParentReferences(newParent, newParentElement);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RemoveFromParent()
{
    if (m_parent)
    {
        // remove from parent
        GetParentElementComponent()->RemoveChild(GetEntity());

        SetParentReferences(nullptr, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindFrontmostChildContainingPoint(AZ::Vector2 point, bool isInGame)
{
    if (!IsFullyInitialized())
    {
        return nullptr;
    }

    AZ::Entity* matchElem = nullptr;

    // this traverses all of the elements in reverse hierarchy order and returns the first one that
    // is containing the point.
    // If necessary, this could be optimized using a spatial partitioning data structure.
    for (int i = m_children.size() - 1; !matchElem && i >= 0; i--)
    {
        AZ::EntityId child = m_children[i];

        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
            // hidden in the editor
            bool isVisible = true;
            EBUS_EVENT_ID_RESULT(isVisible, child, UiEditorBus, GetIsVisible);
            if (!isVisible)
            {
                continue;
            }
        }

        UiElementComponent* childElementComponent = GetChildElementComponent(i);

        // Check children of this child first
        // child elements do not have to be contained in the parent element's bounds
        matchElem = childElementComponent->FindFrontmostChildContainingPoint(point, isInGame);

        if (!matchElem)
        {
            bool isSelectable = true;
            if (!isInGame)
            {
                // We are in editing mode (not running the game)
                // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element
                // can be selected in the editor
                EBUS_EVENT_ID_RESULT(isSelectable, child, UiEditorBus, GetIsSelectable);
            }

            if (isSelectable)
            {
                // if no children of this child matched then check if point is in bounds of this child element
                bool isPointInRect = childElementComponent->GetTransform2dComponent()->IsPointInRect(point);
                if (isPointInRect)
                {
                    matchElem = childElementComponent->GetEntity();
                }
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiElementComponent::FindAllChildrenIntersectingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1, bool isInGame)
{
    LyShine::EntityArray result;

    if (!IsFullyInitialized())
    {
        return result;
    }

    // this traverses all of the elements in hierarchy order
    for (int i = 0; i < m_children.size(); ++i)
    {
        AZ::EntityId child = m_children[i];

        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
            // hidden in the editor
            bool isVisible = true;
            EBUS_EVENT_ID_RESULT(isVisible, child, UiEditorBus, GetIsVisible);
            if (!isVisible)
            {
                continue;
            }
        }

        UiElementComponent* childElementComponent = GetChildElementComponent(i);

        // Check children of this child first
        // child elements do not have to be contained in the parent element's bounds
        LyShine::EntityArray childMatches = childElementComponent->FindAllChildrenIntersectingRect(bound0, bound1, isInGame);
        result.push_back(childMatches);

        bool isSelectable = true;
        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element
            // can be selected in the editor
            EBUS_EVENT_ID_RESULT(isSelectable, child, UiEditorBus, GetIsSelectable);
        }

        if (isSelectable)
        {
            // check if point is in bounds of this child element
            bool isInRect = childElementComponent->GetTransform2dComponent()->BoundsAreOverlappingRect(bound0, bound1);
            if (isInRect)
            {
                result.push_back(childElementComponent->GetEntity());
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindInteractableToHandleEvent(AZ::Vector2 point)
{
    AZ::EntityId result;

    if (!IsFullyInitialized() || !m_isEnabled)
    {
        // Nothing to do
        return result;
    }

    // first check the children (in reverse order) since children are in front of parent
    {
        // if this element is masking children at this point then don't check the children
        bool isMasked = false;
        EBUS_EVENT_ID_RESULT(isMasked, GetEntityId(), UiInteractionMaskBus, IsPointMasked, point);
        if (!isMasked)
        {
            for (int i = m_children.size() - 1; !result.IsValid() && i >= 0; i--)
            {
                result = GetChildElementComponent(i)->FindInteractableToHandleEvent(point);
            }
        }
    }

    // if no match then check this element
    if (!result.IsValid())
    {
        // if this element has an interactable component and the point is in this element's rect
        if (UiInteractableBus::FindFirstHandler(GetEntityId()))
        {
            bool isInRect = GetTransform2dComponent()->IsPointInRect(point);
            if (isInRect)
            {
                // check if this interactable component is in a state where it can handle an event at the given point
                bool canHandle = false;
                EBUS_EVENT_ID_RESULT(canHandle, GetEntityId(), UiInteractableBus, CanHandleEvent, point);
                if (canHandle)
                {
                    result = GetEntityId();
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindParentInteractableSupportingDrag(AZ::Vector2 point)
{
    AZ::EntityId result;

    // if this element has a parent and this element is not completely disabled
    if (m_parent && m_isEnabled)
    {
        AZ::EntityId parentEntity = m_parent->GetId();

        // if the parent supports drag hand off then return it
        bool supportsDragOffset = false;
        EBUS_EVENT_ID_RESULT(supportsDragOffset, parentEntity, UiInteractableBus, DoesSupportDragHandOff, point);
        if (supportsDragOffset)
        {
            result = parentEntity;
        }
        else
        {
            // else keep going up the parent links
            result = GetParentElementComponent()->FindParentInteractableSupportingDrag(point);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindChildByName(const LyShine::NameType& name)
{
    AZ::Entity* matchElem = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = m_childElementComponents.size();
        for (int i = 0; i < numChildren; ++i)
        {
            AZ::Entity* childEntity = GetChildElementComponent(i)->GetEntity();
            if (name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }
        }
    }
    else
    {
        for (auto child : m_children)
        {
            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);
            if (childEntity && name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindDescendantByName(const LyShine::NameType& name)
{
    AZ::Entity* matchElem = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = m_childElementComponents.size();
        for (int i = 0; i < numChildren; ++i)
        {
            UiElementComponent* childElementComponent = GetChildElementComponent(i);
            AZ::Entity* childEntity = childElementComponent->GetEntity();

            if (name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }

            matchElem = childElementComponent->FindDescendantByName(name);
            if (matchElem)
            {
                break;
            }
        }
    }
    else
    {
        for (auto child : m_children)
        {
            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);

            if (childEntity && name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }

            EBUS_EVENT_ID_RESULT(matchElem, child, UiElementBus, FindDescendantByName, name);
            if (matchElem)
            {
                break;
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindChildEntityIdByName(const LyShine::NameType& name)
{
    AZ::Entity* childEntity = FindChildByName(name);
    return childEntity ? childEntity->GetId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindDescendantEntityIdByName(const LyShine::NameType& name)
{
    AZ::Entity* childEntity = FindDescendantByName(name);
    return childEntity ? childEntity->GetId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindChildByEntityId(AZ::EntityId id)
{
    AZ::Entity* matchElem = nullptr;

    int numChildren = m_children.size();
    for (int i = 0; i < numChildren; ++i)
    {
        if (id == m_children[i])
        {
            if (AreChildPointersValid())
            {
                matchElem = GetChildElementComponent(i)->GetEntity();
            }
            else
            {
                EBUS_EVENT_RESULT(matchElem, AZ::ComponentApplicationBus, FindEntity, id);
            }
            break;
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindDescendantById(LyShine::ElementId id)
{
    if (id == m_elementId)
    {
        return GetEntity();
    }

    AZ::Entity* match = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = m_children.size();
        for (int i = 0; !match && i < numChildren; ++i)
        {
            match = GetChildElementComponent(i)->FindDescendantById(id);
        }
    }
    else
    {
        for (auto iter = m_children.begin(); !match && iter != m_children.end(); iter++)
        {
            EBUS_EVENT_ID_RESULT(match, *iter, UiElementBus, FindDescendantById, id);
        }
    }

    return match;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::FindDescendantElements(std::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result)
{
    if (AreChildPointersValid())
    {
        int numChildren = m_children.size();
        for (int i = 0; i < numChildren; ++i)
        {
            UiElementComponent* childElementComponent = GetChildElementComponent(i);

            AZ::Entity* childEntity = childElementComponent->GetEntity();
            if (predicate(childEntity))
            {
                result.push_back(childEntity);
            }

            childElementComponent->FindDescendantElements(predicate, result);
        }
    }
    else
    {
        for (auto child : m_children)
        {
            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);
            if (childEntity && predicate(childEntity))
            {
                result.push_back(childEntity);
            }

            EBUS_EVENT_ID(child, UiElementBus, FindDescendantElements, predicate, result);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::CallOnDescendantElements(std::function<void(const AZ::EntityId)> callFunction)
{
    if (AreChildPointersValid())
    {
        int numChildren = m_children.size();
        for (int i = 0; i < numChildren; ++i)
        {
            callFunction(m_children[i]);

            GetChildElementComponent(i)->CallOnDescendantElements(callFunction);
        }
    }
    else
    {
        for (auto child : m_children)
        {
            callFunction(child);
            EBUS_EVENT_ID(child, UiElementBus, CallOnDescendantElements, callFunction);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::IsAncestor(AZ::EntityId id)
{
    UiElementComponent* parentElementComponent = GetParentElementComponent();
    while (parentElementComponent)
    {
        if (parentElementComponent->GetEntityId() == id)
        {
            return true;
        }

        parentElementComponent = parentElementComponent->GetParentElementComponent();
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::IsEnabled()
{
    return m_isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsVisible()
{
    return m_isVisibleInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsVisible(bool isVisible)
{
    m_isVisibleInEditor = isVisible;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsSelectable()
{
    return m_isSelectableInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsSelectable(bool isSelectable)
{
    m_isSelectableInEditor = isSelectable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsSelected()
{
    return m_isSelectedInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsSelected(bool isSelected)
{
    m_isSelectedInEditor = isSelected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsExpanded()
{
    return m_isExpandedInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsExpanded(bool isExpanded)
{
    m_isExpandedInEditor = isExpanded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::AreAllAncestorsVisible()
{
    UiElementComponent* parentElementComponent = GetParentElementComponent();
    while (parentElementComponent)
    {
        bool isParentVisible = true;
        EBUS_EVENT_ID_RESULT(isParentVisible, parentElementComponent->GetEntityId(), UiEditorBus, GetIsVisible);
        if (!isParentVisible)
        {
            return false;
        }

        // Walk up the hierarchy.
        parentElementComponent = parentElementComponent->GetParentElementComponent();
    }

    // there is no ancestor entity that is not visible
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::AddChild(AZ::Entity* child, AZ::Entity* insertBefore)
{
    // debug check that this element is not already a child
    AZ_Assert(FindChildByEntityId(child->GetId()) == nullptr, "Attempting to add a duplicate child");

    UiElementComponent* childElementComponent = child->FindComponent<UiElementComponent>();
    AZ_Assert(childElementComponent, "Attempting to add a child with no element component");
    if (!childElementComponent)
    {
        return;
    }

    bool wasInserted = false;

    if (insertBefore)
    {
        int numChildren = m_children.size();
        for (int i = 0; i < numChildren; ++i)
        {
            if (m_children[i] == insertBefore->GetId())
            {
                if (AreChildPointersValid())    // must test before m_children.insert
                {
                    m_childElementComponents.insert(m_childElementComponents.begin() + i, childElementComponent);
                }

                m_children.insert(m_children.begin() + i, child->GetId());

                wasInserted = true;
                break;
            }
        }
    }

    // either insertBefore is null or it is not found, insert at end
    if (!wasInserted)
    {
        if (AreChildPointersValid())    // must test before m_children.push_back
        {
            m_childElementComponents.push_back(childElementComponent);
        }
        m_children.push_back(child->GetId());
    }

    // Adding or removing child elements may require recomputing the
    // transforms of all children
    EBUS_EVENT_ID(GetCanvasEntityId(), UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
    EBUS_EVENT_ID(GetCanvasEntityId(), UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);

    // It will always require recomputing the transform for the child just added
    if (IsFullyInitialized())
    {
        GetTransform2dComponent()->SetRecomputeTransformFlag();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RemoveChild(AZ::Entity* child)
{
    if (stl::find_and_erase(m_children, child->GetId()))
    {
        UiElementComponent* elementComponent = child->FindComponent<UiElementComponent>();
        AZ_Assert(elementComponent, "Child element has no UiElementComponent");

        // Also erase from m_childElementComponents
        stl::find_and_erase(m_childElementComponents, elementComponent);

        // Clear child's parent
        elementComponent->SetParentReferences(nullptr, nullptr);

        // Adding or removing child elements may require recomputing the
        // transforms of all children
        EBUS_EVENT_ID(GetCanvasEntityId(), UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
        EBUS_EVENT_ID(GetCanvasEntityId(), UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetCanvas(UiCanvasComponent* canvas, LyShine::ElementId elementId)
{
    m_canvas = canvas;
    m_elementId = elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::FixupPostLoad(AZ::Entity* entity, UiCanvasComponent* canvas, AZ::Entity* parent, bool makeNewElementIds)
{
    if (makeNewElementIds)
    {
        m_elementId = canvas->GenerateId();
    }

    m_canvas = canvas;

    if (parent)
    {
        UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
        AZ_Assert(parentElementComponent, "Parent element has no UiElementComponent");
        SetParentReferences(parent, parentElementComponent);
    }
    else
    {
        SetParentReferences(nullptr, nullptr);
    }

    AZStd::vector<AZ::EntityId> missingChildren;

    for (auto child : m_children)
    {
        AZ::Entity* childEntity = nullptr;
        EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);
        if (!childEntity)
        {
            // with slices it is possible for users to get themselves into situations where a child no
            // longer exists, we should report an error in this case rather than asserting
            AZ_Error("UI", false, "Child element with Entity ID %llu no longer exists. Data will be lost.", child);
            // This case could happen if a slice asset has been deleted. We should try to continue and load the
            // canvas with errors.
            missingChildren.push_back(child);
            continue;
        }

        UiElementComponent* elementComponent = childEntity->FindComponent<UiElementComponent>();
        if (!elementComponent)
        {
            // with slices it is possible for users to get themselves into situations where a child no
            // longer has an element component. In this case report an error and fail to load the data but do not
            // crash.
            AZ_Error("UI", false, "Child element with Entity ID %llu no longer has a UiElementComponent. Data cannot be loaded.", child);
            return false;
        }

        bool success = elementComponent->FixupPostLoad(childEntity, canvas, entity, makeNewElementIds);
        if (!success)
        {
            return false;
        }
    }

    // If there were any missing children remove them from the m_children list
    // This is recovery code for the case that a slice asset that we were using has been removed.
    for (auto child : missingChildren)
    {
        stl::find_and_erase(m_children, child);
    }

    // Initialize the m_childElementComponents array that is used for performance optimization
    m_childElementComponents.clear();
    for (auto child : m_children)
    {
        AZ::Entity* childEntity = nullptr;
        EBUS_EVENT_RESULT(childEntity, AZ::ComponentApplicationBus, FindEntity, child);
        AZ_Assert(childEntity, "Child element not found");
        UiElementComponent* childElementComponent = childEntity->FindComponent<UiElementComponent>();
        AZ_Assert(childElementComponent, "Child element has no UiElementComponent");
        m_childElementComponents.push_back(childElementComponent);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetSliceEntityParentId()
{
    return GetParentEntityId();
}

AZStd::vector<AZ::EntityId> UiElementComponent::GetSliceEntityChildren()
{
    return GetChildEntityIds();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiElementComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiElementComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("Id", &UiElementComponent::m_elementId)
            ->Field("Children", &UiElementComponent::m_children)
            ->Field("IsVisibleInEditor", &UiElementComponent::m_isVisibleInEditor)
            ->Field("IsSelectableInEditor", &UiElementComponent::m_isSelectableInEditor)
            ->Field("IsSelectedInEditor", &UiElementComponent::m_isSelectedInEditor)
            ->Field("IsExpandedInEditor", &UiElementComponent::m_isExpandedInEditor);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiElementComponent>("Element", "Adds UI Element behavior to an entity");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiElement.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiElement.png")
                ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // Cannot be added or removed by user
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("String", &UiElementComponent::m_elementId, "Id",
                "This read-only ID is used to reference the element from FlowGraph")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable);

            // This is not visible in the PropertyGrid but is required for Push to Slice to be able
            // to push the addition/removal of children
            editInfo->DataElement(0, &UiElementComponent::m_children, "Children", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible | AZ::Edit::SliceFlags::DontGatherReference);

            // These are not visible in the PropertyGrid since they are managed through the Hierarch Pane
            // We do want to be able to push them to a slice though.
            editInfo->DataElement(0, &UiElementComponent::m_isVisibleInEditor, "IsVisibleInEditor", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
            editInfo->DataElement(0, &UiElementComponent::m_isSelectableInEditor, "IsSelectableInEditor", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiElementBus>("UiElementBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
            ->Event("GetName", &UiElementBus::Events::GetName)
            ->Event("GetCanvas", &UiElementBus::Events::GetCanvasEntityId)
            ->Event("GetParent", &UiElementBus::Events::GetParentEntityId)
            ->Event("GetNumChildElements", &UiElementBus::Events::GetNumChildElements)
            ->Event("GetChild", &UiElementBus::Events::GetChildEntityId)
            ->Event("GetIndexOfChildByEntityId", &UiElementBus::Events::GetIndexOfChildByEntityId)
            ->Event("GetChildren", &UiElementBus::Events::GetChildEntityIds)
            ->Event("DestroyElement", &UiElementBus::Events::DestroyElement)
            ->Event("Reparent", &UiElementBus::Events::ReparentByEntityId)
            ->Event("FindChildByName", &UiElementBus::Events::FindChildEntityIdByName)
            ->Event("FindDescendantByName", &UiElementBus::Events::FindDescendantEntityIdByName)
            ->Event("IsAncestor", &UiElementBus::Events::IsAncestor)
            ->Event("IsEnabled", &UiElementBus::Events::IsEnabled)
            ->Event("SetIsEnabled", &UiElementBus::Events::SetIsEnabled);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Initialize()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Activate()
{
    UiElementBus::Handler::BusConnect(m_entity->GetId());
    UiEditorBus::Handler::BusConnect(m_entity->GetId());
    AZ::SliceEntityHierarchyRequestBus::Handler::BusConnect(m_entity->GetId());

    // Once added the transform component is never removed
    if (!m_transformComponent)
    {
        m_transformComponent = GetEntity()->FindComponent<UiTransform2dComponent>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Deactivate()
{
    UiElementBus::Handler::BusDisconnect();
    UiEditorBus::Handler::BusDisconnect();
    AZ::SliceEntityHierarchyRequestBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::EmitNotInitializedWarning() const
{
    AZ_Warning("UI", false, "UiElementComponent used before fully initialized, possibly on activate before FixupPostLoad was called on this element")
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetParentReferences(AZ::Entity* parent, UiElementComponent* parentElementComponent)
{
    m_parent = parent;
    m_parentId = (parent) ? parent->GetId() : AZ::EntityId();
    m_parentElementComponent = parentElementComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    if (classElement.GetVersion() < 2)
    {
        // No need to actually convert anything because the CanvasFileObject takes care of it
        // But it makes sense to bump the version number because m_children is now a container
        // of EntityId rather than Entity*
    }

    return true;
}
