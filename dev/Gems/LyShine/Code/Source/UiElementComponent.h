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

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiEditorBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>

#include "UiSerialize.h"
#include <LyShine/UiComponentTypes.h>

class UiCanvasComponent;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiElementComponent
    : public AZ::Component
    , public UiElementBus::Handler
    , public UiEditorBus::Handler
{
public: // types

    // used to map old EntityId's to new EntityId's when generating new ids for a paste or prefab
    typedef LyShine::EntityIdMap EntityIdMap;

public: // member functions

    AZ_COMPONENT(UiElementComponent, LyShine::UiElementComponentUuid, AZ::Component);

    //! Construct an uninitialized element component
    UiElementComponent();
    ~UiElementComponent() override;

    // UiElementInterface

    void UpdateElement() override;
    void RenderElement(bool isInGame, bool displayBounds) override;

    LyShine::ElementId GetElementId() override;
    LyShine::NameType GetName() override;

    AZ::EntityId GetCanvasEntityId() override;
    AZ::Entity* GetParent() override;
    AZ::EntityId GetParentEntityId() override;

    int GetNumChildElements() override;
    AZ::Entity* GetChildElement(int index) override;
    AZ::EntityId GetChildEntityId(int index) override;

    int GetIndexOfChild(const AZ::Entity* child) override;
    int GetIndexOfChildByEntityId(AZ::EntityId childId) override;

    LyShine::EntityArray GetChildElements() override;
    AZStd::vector<AZ::EntityId> GetChildEntityIds() override;

    AZ::Entity* CreateChildElement(const LyShine::NameType& name) override;
    void DestroyElement() override;
    void Reparent(AZ::Entity* newParent, AZ::Entity* insertBefore = nullptr) override;
    void ReparentByEntityId(AZ::EntityId newParent, AZ::EntityId insertBefore) override;
    void AddToParentAtIndex(AZ::Entity* newParent, int index = -1) override;
    void RemoveFromParent() override;

    AZ::Entity* FindFrontmostChildContainingPoint(AZ::Vector2 point, bool isInGame) override;
    LyShine::EntityArray FindAllChildrenIntersectingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1, bool isInGame) override;

    AZ::EntityId FindInteractableToHandleEvent(AZ::Vector2 point) override;
    AZ::EntityId FindParentInteractableSupportingDrag(AZ::Vector2 point) override;

    AZ::Entity* FindChildByName(const LyShine::NameType& name) override;
    AZ::Entity* FindDescendantByName(const LyShine::NameType& name) override;
    AZ::EntityId FindChildEntityIdByName(const LyShine::NameType& name) override;
    AZ::EntityId FindDescendantEntityIdByName(const LyShine::NameType& name) override;
    AZ::Entity* FindChildByEntityId(AZ::EntityId id) override;
    AZ::Entity* FindDescendantById(LyShine::ElementId id) override;
    void FindDescendantElements(std::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result) override;

    bool IsAncestor(AZ::EntityId id) override;

    bool IsEnabled() override;
    void SetIsEnabled(bool isEnabled) override;

    // ~UiElementInterface

    // UiEditorInterface
    //! The UiElementComponent implements the editor interface in order to store the state with the element on save
    bool GetIsVisible() override;
    void SetIsVisible(bool isVisible) override;
    bool GetIsSelectable() override;
    void SetIsSelectable(bool isSelectable) override;
    bool GetIsSelected() override;
    void SetIsSelected(bool isSelected) override;
    bool GetIsExpanded() override;
    void SetIsExpanded(bool isExpanded) override;
    bool AreAllAncestorsVisible() override;
    // ~UiEditorInterface

    void AddChild(AZ::Entity* child, AZ::Entity* insertBefore = nullptr);
    void RemoveChild(AZ::Entity* child);

    //! Only to be used by UiCanvasComponent when creating the root element
    void SetCanvas(UiCanvasComponent* canvas, LyShine::ElementId elementId);

    //! Only to be used by UiCanvasComponent when loading, cloning etc
    bool FixupPostLoad(AZ::Entity* entity, UiCanvasComponent* canvas, AZ::Entity* parent, bool makeNewElementIds);

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

    static void Initialize();

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    AZ_DISABLE_COPY_MOVE(UiElementComponent);

    LyShine::ElementId m_elementId;

    AZStd::vector<AZ::EntityId> m_children;
    AZ::Entity* m_parent;
    UiCanvasComponent* m_canvas;    // currently we store a pointer to the canvas component rather than an entity ID

    bool m_isEnabled;

    // this data is only relevant when running in the editor, it is accessed through UiEditorBus
    bool m_isVisibleInEditor;
    bool m_isSelectableInEditor;
    bool m_isSelectedInEditor;
    bool m_isExpandedInEditor;
};
