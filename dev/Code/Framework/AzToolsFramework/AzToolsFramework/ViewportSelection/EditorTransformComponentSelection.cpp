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

#include "EditorTransformComponentSelection.h"

#include <AzCore/std/algorithm.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Commands/EntityManipulatorCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <Entity/EditorEntityContextBus.h>
#include <Entity/EditorEntityHelpers.h>
#include <QApplication>
#include <QRect>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorTransformComponentSelection, AZ::SystemAllocator, 0)

    // strings related to new viewport interaction model (EditorTransformComponentSelection)
    static const char* const s_togglePivotTitleRightClick = "Toggle pivot";
    static const char* const s_togglePivotTitleEditMenu = "Toggle Pivot Location";
    static const char* const s_togglePivotDesc = "Toggle pivot location";
    static const char* const s_manipulatorUndoRedoName = "Manipulator Adjustment";
    static const char* const s_lockSelectionTitle = "Lock Selection";
    static const char* const s_lockSelectionDesc = "Lock the selected entities so that they can't be selected in the viewport";
    static const char* const s_hideSelectionTitle = "Hide Selection";
    static const char* const s_hideSelectionDesc = "Hide the selected entities so that they don't appear in the viewport";
    static const char* const s_unlockAllTitle = "Unlock All Entities";
    static const char* const s_unlockAllDesc = "Unlock all entities the level";
    static const char* const s_showAllTitle = "Show All";
    static const char* const s_showAllDesc = "Show all entities so that they appear in the viewport";
    static const char* const s_selectAllTitle = "Select All";
    static const char* const s_selectAllDesc = "Select all entities";
    static const char* const s_invertSelectionTitle = "Invert Selection";
    static const char* const s_invertSelectionDesc = "Invert the current entity selection";
    static const char* const s_duplicateTitle = "Duplicate";
    static const char* const s_duplicateDesc = "Duplicate selected entities";
    static const char* const s_deleteTitle = "Delete";
    static const char* const s_deleteDesc = "Delete selected entities";
    static const char* const s_resetEntityTransformTitle = "Reset Entity Transform";
    static const char* const s_resetEntityTransformDesc = "Reset transform based on manipulator mode";
    static const char* const s_resetManipulatorTitle = "Reset Manipulator";
    static const char* const s_resetManipulatorDesc = "Reset the manipulator to recenter it on the selected entity";
    static const char* const s_resetTransformLocalTitle = "Reset Transform (Local)";
    static const char* const s_resetTransformLocalDesc = "Reset transform to local space";
    static const char* const s_resetTransformWorldTitle = "Reset Transform (World)";
    static const char* const s_resetTransformWorldDesc = "Reset transform to world space";

    static const char* const s_entityBoxSelectUndoRedoDesc = "Box Select Entities";
    static const char* const s_entityDeselectUndoRedoDesc = "Deselect Entity";
    static const char* const s_entitiesDeselectUndoRedoDesc = "Deselect Entities";
    static const char* const s_entitySelectUndoRedoDesc = "Select Entity";
    static const char* const s_dittoManipulatorUndoRedoDesc = "Ditto Manipulator";
    static const char* const s_resetManipulatorTranslationUndoRedoDesc = "Reset Manipulator Translation";
    static const char* const s_resetManipulatorOrientationUndoRedoDesc = "Reset Manipulator Orientation";
    static const char* const s_dittoEntityOrientationIndividualUndoRedoDesc = "Ditto orientation individual";
    static const char* const s_dittoEntityOrientationGroupUndoRedoDesc = "Ditto orientation group";
    static const char* const s_resetTranslationToParentUndoRedoDesc = "Reset translation to parent";
    static const char* const s_resetOrientationToParentUndoRedoDesc = "Reset orientation to parent";
    static const char* const s_dittoTranslationGroupUndoRedoDesc = "Ditto translation group";
    static const char* const s_dittoTranslationIndividualUndoRedoDesc = "Ditto translation individual";
    static const char* const s_dittoScaleIndividualWorldUndoRedoDesc = "Ditto scale individual world";
    static const char* const s_dittoScaleIndividualLocalUndoRedoDesc = "Ditto scale individual local";
    static const char* const s_showAllEntitiesUndoRedoDesc = s_showAllTitle;
    static const char* const s_lockSelectionUndoRedoDesc = s_lockSelectionTitle;
    static const char* const s_hideSelectionUndoRedoDesc = s_hideSelectionTitle;
    static const char* const s_unlockAllUndoRedoDesc = s_unlockAllTitle;
    static const char* const s_selectAllEntitiesUndoRedoDesc = s_selectAllTitle;
    static const char* const s_invertSelectionUndoRedoDesc = s_invertSelectionTitle;
    static const char* const s_duplicateUndoRedoDesc = s_duplicateTitle;
    static const char* const s_deleteUndoRedoDesc = s_deleteTitle;

    static const AZ::Color s_xAxisColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    static const AZ::Color s_yAxisColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    static const AZ::Color s_zAxisColor = AZ::Color(0.0f, 0.0f, 1.0f, 1.0f);

    static const AZ::Color s_fadedXAxisColor = AZ::Color(AZ::u8(200), AZ::u8(127), AZ::u8(127), AZ::u8(255));
    static const AZ::Color s_fadedYAxisColor = AZ::Color(AZ::u8(127), AZ::u8(190), AZ::u8(127), AZ::u8(255));
    static const AZ::Color s_fadedZAxisColor = AZ::Color(AZ::u8(120), AZ::u8(120), AZ::u8(180), AZ::u8(255));

    static const AZ::Color s_pickedOrientationColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    static const AZ::Color s_selectedEntityAabbColor = AZ::Color(0.6f, 0.6f, 0.6f, 0.4f);

    static const AZ::VectorFloat s_pivotSize = AZ::VectorFloat(0.075f); ///< The size of the pivot (box) to render when selected.

    // result from calculating the entity (transform component) space
    // does the entity have a parent or not, and what orientation should
    // the manipulator have when displayed at the object pivot.
    // (determined by the entity hierarchy and what modifiers are held)
    struct PivotOrientationResult
    {
        AZ::Quaternion m_worldOrientation;
        AZ::EntityId m_parentId;
    };

    // data passed to manipulators when processing mouse interactions
    // m_entityIds should be sorted based on the entity hierarchy
    // (see SortEntitiesByLocationInHierarchy and BuildSortedEntityIdVectorFromEntityIdContainer)
    struct ManipulatorEntityIds
    {
        EntityIdList m_entityIds;
    };

    bool OptionalFrame::HasTranslationOverride() const
    {
        return m_translationOverride.has_value();
    }

    bool OptionalFrame::HasOrientationOverride() const
    {
        return m_orientationOverride.has_value();
    }

    bool OptionalFrame::HasTransformOverride() const
    {
        return  m_translationOverride.has_value()
            || m_orientationOverride.has_value();
    }

    bool OptionalFrame::HasEntityOverride() const
    {
        return m_pickedEntityIdOverride.IsValid();
    }

    bool OptionalFrame::PickedTranslation() const
    {
        return (m_pickTypes & PickType::Translation) != 0;
    }

    bool OptionalFrame::PickedOrientation() const
    {
        return (m_pickTypes & PickType::Orientation) != 0;
    }

    void OptionalFrame::Reset()
    {
        m_orientationOverride.reset();
        m_translationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes = PickType::None;
    }

    void OptionalFrame::ResetPickedTranslation()
    {
        m_translationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes &= ~PickType::Translation;
    }

    void OptionalFrame::ResetPickedOrientation()
    {
        m_orientationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes &= ~PickType::Orientation;
    }

    using EntityIdTransformMap = AZStd::unordered_map<AZ::EntityId, AZ::Transform>;

    // wrap input/controls for EditorTransformComponentSelection to
    // make visibility clearer and adjusting them easier.
    namespace Input
    {
        static bool CycleManipulator(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Wheel &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool DeselectAll(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left();
        }

        static bool AdditiveIndividualSelect(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool GroupDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool IndividualDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
        }

        static bool SnapTerrain(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() ||
                    mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl());
        }

        static bool ManipulatorDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
        }

        static bool IndividualSelect(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down;
        }
    } // namespace Input

    static void DestroyManipulators(EntityIdManipulators& manipulators)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (manipulators.m_manipulators)
        {
            manipulators.m_lookups.clear();
            manipulators.m_manipulators->Unregister();
            manipulators.m_manipulators.reset();
        }
    }

    static EditorTransformComponentSelectionRequests::Pivot TogglePivotMode(
        const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        switch (pivot)
        {
        case EditorTransformComponentSelectionRequests::Pivot::Object:
            return EditorTransformComponentSelectionRequests::Pivot::Center;
        case EditorTransformComponentSelectionRequests::Pivot::Center:
            return EditorTransformComponentSelectionRequests::Pivot::Object;
        default:
            AZ_Assert(false, "Invalid Pivot");
            return EditorTransformComponentSelectionRequests::Pivot::Object;
        }
    }

    // take a generic, non-associative container and return a vector
    template<typename EntityIdContainer>
    static AZStd::vector<AZ::EntityId> EntityIdVectorFromContainer(const EntityIdContainer& entityIdContainer)
    {
        static_assert(AZStd::is_same<typename EntityIdContainer::value_type, AZ::EntityId>::value,
            "Container type is not an EntityId");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        return AZStd::vector<AZ::EntityId>(entityIdContainer.begin(), entityIdContainer.end());
    }

    // take a generic, associative container and return a vector
    template<typename EntityIdMap>
    static AZStd::vector<AZ::EntityId> EntityIdVectorFromMap(const EntityIdMap& entityIdMap)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(entityIdMap.size());

        for (typename EntityIdMap::const_iterator it = entityIdMap.begin(); it != entityIdMap.end(); ++it)
        {
            entityIds.push_back(it->first);
        }

        return entityIds;
    }

    // WIP - currently just using positions - should use aabb here
    // e.g.
    //   get aabb
    //   get each edge/line of aabb
    //   convert to screen space
    //   test intersect segment against aabb screen rect

    template<typename EntitySelectFuncType, typename EntityIdContainer, typename Compare>
    static void BoxSelectAddRemoveToEntitySelection(
        const AZStd::optional<QRect>& boxSelect, const QPoint& screenPosition, const AZ::EntityId visibleEntityId,
        const EntityIdContainer& incomingEntityIds, EntityIdContainer& outgoingEntityIds,
        EditorTransformComponentSelection& entityTransformComponentSelection,
        EntitySelectFuncType selectFunc1, EntitySelectFuncType selectFunc2, Compare outgoingCheck)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (boxSelect->contains(screenPosition))
        {
            const auto entityIt = incomingEntityIds.find(visibleEntityId);

            if (outgoingCheck(entityIt, incomingEntityIds))
            {
                outgoingEntityIds.insert(visibleEntityId);
                (entityTransformComponentSelection.*selectFunc1)(visibleEntityId);
            }
        }
        else
        {
            const auto entityIt = outgoingEntityIds.find(visibleEntityId);

            if (entityIt != outgoingEntityIds.end())
            {
                outgoingEntityIds.erase(entityIt);
                (entityTransformComponentSelection.*selectFunc2)(visibleEntityId);
            }
        }
    }

    template<typename EntityIdContainer>
    static void EntityBoxSelectUpdateGeneral(
        const AZStd::optional<QRect>& boxSelect, EditorTransformComponentSelection& editorTransformComponentSelection,
        const EntityIdContainer& activeSelectedEntityIds, EntityIdContainer& selectedEntityIdsBeforeBoxSelect,
        EntityIdContainer& potentialSelectedEntityIds, EntityIdContainer& potentialDeselectedEntityIds,
        const EditorVisibleEntityDataCache& entityDataCache, const int viewportId,
        const ViewportInteraction::KeyboardModifiers currentKeyboardModifiers,
        const ViewportInteraction::KeyboardModifiers& previousKeyboardModifiers)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (boxSelect)
        {
            // modifier has changed - swapped from additive to subtractive box select (or vice versa)
            if (previousKeyboardModifiers != currentKeyboardModifiers)
            {
                for (const AZ::EntityId entityId : potentialDeselectedEntityIds)
                {
                    editorTransformComponentSelection.AddEntityToSelection(entityId);
                }
                potentialDeselectedEntityIds.clear();

                for (const AZ::EntityId entityId : potentialSelectedEntityIds)
                {
                    editorTransformComponentSelection.RemoveEntityFromSelection(entityId);
                }
                potentialSelectedEntityIds.clear();
            }

            // set the widget context before calls to ViewportWorldToScreen so we are not
            // going to constantly be pushing/popping the widget context
            ViewportInteraction::WidgetContextGuard widgetContextGuard(viewportId);

            for (size_t entityCacheIndex = 0; entityCacheIndex < entityDataCache.VisibleEntityDataCount(); ++entityCacheIndex)
            {
                if (    entityDataCache.IsVisibleEntityLocked(entityCacheIndex)
                    || !entityDataCache.IsVisibleEntityVisible(entityCacheIndex))
                {
                    continue;
                }

                const AZ::EntityId entityId = entityDataCache.GetVisibleEntityId(entityCacheIndex);
                const AZ::Vector3& entityPosition = entityDataCache.GetVisibleEntityPosition(entityCacheIndex);

                const QPoint screenPosition = GetScreenPosition(viewportId, entityPosition);

                if (currentKeyboardModifiers.Ctrl())
                {
                    BoxSelectAddRemoveToEntitySelection(
                        boxSelect, screenPosition, entityId,
                        selectedEntityIdsBeforeBoxSelect, potentialDeselectedEntityIds,
                        editorTransformComponentSelection,
                        &EditorTransformComponentSelection::RemoveEntityFromSelection,
                        &EditorTransformComponentSelection::AddEntityToSelection,
                        [](const typename EntityIdContainer::const_iterator entityId, const EntityIdContainer& entityIds)
                        {
                            return entityId != entityIds.end();
                        });
                }
                else
                {
                    BoxSelectAddRemoveToEntitySelection(
                        boxSelect, screenPosition, entityId,
                        activeSelectedEntityIds, potentialSelectedEntityIds,
                        editorTransformComponentSelection,
                        &EditorTransformComponentSelection::AddEntityToSelection,
                        &EditorTransformComponentSelection::RemoveEntityFromSelection,
                        [](const typename EntityIdContainer::const_iterator entityId, const EntityIdContainer& entityIds)
                        {
                            return entityId == entityIds.end();
                        });
                }
            }
        }
    }

    static void InitializeTranslationLookup(
        EntityIdManipulators& entityIdManipulators, const AZ::Vector3& snapOffset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        for (auto& entityIdLookup : entityIdManipulators.m_lookups)
        {
            entityIdLookup.second.m_initial =
                AZ::Transform::CreateTranslation(GetWorldTranslation(entityIdLookup.first) + snapOffset);
        }
    }

    // return either center or entity pivot
    static AZ::Vector3 CalculatePivotTranslation(
        const AZ::EntityId entityId, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        return worldFromLocal * CalculateCenterOffset(entityId, pivot);
    }

    static PivotOrientationResult CalculatePivotOrientation(
        const AZ::EntityId entityId, const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // initialize to world space, no parent
        PivotOrientationResult result{ AZ::Quaternion::CreateIdentity(), AZ::EntityId() };

        switch (referenceFrame)
        {
        case ReferenceFrame::Local:
            AZ::TransformBus::EventResult(
                result.m_worldOrientation, entityId,
                &AZ::TransformBus::Events::GetWorldRotationQuaternion);
            break;
        case ReferenceFrame::Parent:
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(
                parentId,entityId, &AZ::TransformBus::Events::GetParentId);

            if (parentId.IsValid())
            {
                AZ::TransformBus::EventResult(
                    result.m_worldOrientation, parentId,
                    &AZ::TransformBus::Events::GetWorldRotationQuaternion);

                result.m_parentId = parentId;
            }
        }
        break;
        default:
            // do nothing
            break;
        }

        return result;
    }

    // return parent space from selection - if entity(s) share a common parent,
    // return that space, otherwise return world space
    template<typename EntityIdMapIterator>
    static PivotOrientationResult CalculateParentSpace(EntityIdMapIterator begin, EntityIdMapIterator end)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // initialize to world with no parent
        PivotOrientationResult result{ AZ::Quaternion::CreateIdentity(), AZ::EntityId() };

        AZ::EntityId commonParentId;
        for (EntityIdMapIterator entityIdLookupIt = begin; entityIdLookupIt != end; ++entityIdLookupIt)
        {
            // check if this entity has a parent
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(
                parentId, entityIdLookupIt->first, &AZ::TransformBus::Events::GetParentId);

            // if no parent, space will be world, terminate
            if (!parentId.IsValid())
            {
                commonParentId.SetInvalid();
                result.m_worldOrientation = AZ::Quaternion::CreateIdentity();

                break;
            }

            // if we've not yet set common parent, set it to first valid parent id
            if (!commonParentId.IsValid())
            {
                commonParentId = parentId;
                AZ::TransformBus::EventResult(
                    result.m_worldOrientation, parentId,
                    &AZ::TransformBus::Events::GetWorldRotationQuaternion);
            }

            // if we know we still have a parent in common
            if (parentId != commonParentId)
            {
                // if the current parent is different to the common parent,
                // space will be world, terminate
                commonParentId.SetInvalid();
                result.m_worldOrientation = AZ::Quaternion::CreateIdentity();

                break;
            }
        }

        // finally record if we have a parent or not
        result.m_parentId = commonParentId;

        return result;
    }

    template<typename EntityIdMap>
    static AZ::Vector3 CalculatePivotTranslationForEntityIds(
        const EntityIdMap& entityIdMap, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!entityIdMap.empty())
        {
            // calculate average position of selected vertices for translation manipulator
            AZ::Vector3 minPosition = AZ::Vector3(AZ::g_fltMax);
            AZ::Vector3 maxPosition = AZ::Vector3(-AZ::g_fltMax);
            for (const auto& entityId : entityIdMap)
            {
                const AZ::Vector3 pivotPosition = CalculatePivotTranslation(entityId.first, pivot);
                minPosition = pivotPosition.GetMin(minPosition);
                maxPosition = pivotPosition.GetMax(maxPosition);
            }

            return minPosition + (maxPosition - minPosition) * 0.5f;
        }

        return AZ::Vector3::CreateZero();
    }

    template<typename EntityIdMap>
    static PivotOrientationResult CalculatePivotOrientationForEntityIds(
        const EntityIdMap& entityIdMap, const ReferenceFrame referenceFrame)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // simple case with one entity
        if (entityIdMap.size() == 1)
        {
            return CalculatePivotOrientation(entityIdMap.begin()->first, referenceFrame);
        }

        // local/parent logic the same
        switch (referenceFrame)
        {
        case ReferenceFrame::Local:
        case ReferenceFrame::Parent:
            return CalculateParentSpace(entityIdMap.begin(), entityIdMap.end());
        default:
            return { AZ::Quaternion::CreateIdentity(), AZ::EntityId() };
        }
    }

    template<typename EntityIdMap>
    static AZStd::optional<PivotOrientationResult> TryCalculatePickedPivot(
        const OptionalFrame& pivotOverrideFrame, const ReferenceFrame referenceFrame)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");

        // if reference frame is parent, check if we have picked an entity
        // as our reference space, and use its parent orientation
        if (pivotOverrideFrame.m_pickedEntityIdOverride.IsValid() && pivotOverrideFrame.PickedOrientation())
        {
            const EntityIdMap entityIds =
            {
                { pivotOverrideFrame.m_pickedEntityIdOverride, { AZ::Transform::CreateIdentity() } }
            };

            return AZStd::optional<PivotOrientationResult>(
                CalculatePivotOrientationForEntityIds(entityIds, referenceFrame));
        }

        return {};
    }

    template<typename EntityIdMap>
    static PivotOrientationResult CalculateSelectionPivotOrientation(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const ReferenceFrame referenceFrame)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");
        static_assert(AZStd::is_same<typename EntityIdMap::mapped_type, EntityIdManipulators::Lookup>::value,
            "Container value type is not an EntityIdManipulators::Lookup");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // start - calculate orientation without considering current overrides/modifications
        PivotOrientationResult pivot = CalculatePivotOrientationForEntityIds(entityIdMap, referenceFrame);

        // if there is already an orientation override
        if (pivotOverrideFrame.m_orientationOverride)
        {
            if (referenceFrame == ReferenceFrame::Local)
            {
                // override orientation
                pivot.m_worldOrientation = pivotOverrideFrame.m_orientationOverride.value();
            }
            else
            {
                // check to see if we have picked another entity, use it or its parent if we have
                pivot = TryCalculatePickedPivot<EntityIdMap>(pivotOverrideFrame, referenceFrame).value_or(pivot);
            }
        }
        else
        {
            // check to see if we have picked another entity, use it or its parent if we have
            if (const auto pickedPivot = TryCalculatePickedPivot<EntityIdMap>(pivotOverrideFrame, referenceFrame))
            {
                pivot = pickedPivot.value();
            }
            else if (entityIdMap.size() > 1)
            {
                // if there's no orientation override and reference frame is parent, orientation
                // should be aligned to world frame/space when there's more than one entity
                if (referenceFrame == ReferenceFrame::Parent)
                {
                    pivot.m_worldOrientation = AZ::Quaternion::CreateIdentity();
                }
            }
        }

        return pivot;
    }

    template<typename EntityIdMap>
    static AZ::Vector3 RecalculateAverageManipulatorTranslation(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        return pivotOverrideFrame.m_translationOverride.value_or(
            CalculatePivotTranslationForEntityIds(entityIdMap, pivot));
    }

    template<typename EntityIdMap>
    static AZ::Quaternion RecalculateAverageManipulatorOrientation(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        return CalculateSelectionPivotOrientation(
            entityIdMap, pivotOverrideFrame, referenceFrame).m_worldOrientation;
    }

    template<typename EntityIdMap>
    static AZ::Transform RecalculateAverageManipulatorTransform(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot,
        const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // return final transform, if we have an override for translation use that, otherwise
        // use centered translation of selection
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            RecalculateAverageManipulatorOrientation(
                entityIdMap, pivotOverrideFrame, referenceFrame),
            RecalculateAverageManipulatorTranslation(
                entityIdMap, pivotOverrideFrame, pivot));
    }

    template<typename EntityIdMap>
    static void BuildSortedEntityIdVectorFromEntityIdMap(
        const EntityIdMap& entityIds, EntityIdList& sortedEntityIdsOut)
    {
        sortedEntityIdsOut = EntityIdVectorFromMap(entityIds);
        SortEntitiesByLocationInHierarchy(sortedEntityIdsOut);
    }

    static void UpdateInitialRotation(EntityIdManipulators& entityManipulators)
    {
        // save new start orientation (if moving rotation axes separate from object
        // or switching type of rotation (modifier keys change))
        for (auto& entityIdLookup : entityManipulators.m_lookups)
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

            entityIdLookup.second.m_initial = worldFromLocal;
        }
    }

    template<typename Action, typename EntityIdContainer>
    static void UpdateTranslationManipulator(
        const Action& action, const EntityIdContainer& entityIdContainer,
        EntityIdManipulators& entityIdManipulators,
        OptionalFrame& pivotOverrideFrame,
        ViewportInteraction::KeyboardModifiers& prevModifiers)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        entityIdManipulators.m_manipulators->SetLocalPosition(action.LocalPosition());

        if (action.m_modifiers.Ctrl())
        {
            // moving with ctrl - setting override
            pivotOverrideFrame.m_translationOverride = entityIdManipulators.m_manipulators->GetPosition();
            InitializeTranslationLookup(entityIdManipulators, -action.LocalPositionOffset());
        }
        else
        {
            const auto pivotOrientation =
                CalculateSelectionPivotOrientation(
                    entityIdManipulators.m_lookups, pivotOverrideFrame, ReferenceFrame::Parent);

            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (const AZ::EntityId entityId : entityIdContainer)
            {
                const auto entityItLookupIt = entityIdManipulators.m_lookups.find(entityId);
                if (entityItLookupIt == entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Vector3 worldTranslation = GetWorldTranslation(entityId);

                if (action.m_modifiers.Alt() && !action.m_modifiers.Shift())
                {
                    // move in each entities local space at once
                    AZ::Quaternion worldOrientation = AZ::Quaternion::CreateIdentity();
                    AZ::TransformBus::EventResult(
                        worldOrientation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);

                    const AZ::Transform space =
                        entityIdManipulators.m_manipulators->GetLocalTransform().GetInverseFull() *
                        AZ::Transform::CreateFromQuaternionAndTranslation(
                            worldOrientation, worldTranslation);

                    const AZ::Vector3 localOffset = space.Multiply3x3(action.LocalPositionOffset());

                    if (action.m_modifiers != prevModifiers)
                    {
                        entityItLookupIt->second.m_initial =
                            AZ::Transform::CreateTranslation(worldTranslation - localOffset);
                    }

                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTranslation,
                        entityItLookupIt->second.m_initial.GetTranslation() +
                        localOffset);
                }
                else if (action.m_modifiers.Shift())
                {
                    AZ::Quaternion offsetRotation =
                        pivotOrientation.m_worldOrientation *
                        QuaternionFromTransformNoScaling(
                            entityIdManipulators.m_manipulators->GetLocalTransform().GetInverseFull());

                    const AZ::Vector3 localOffset = offsetRotation * action.LocalPositionOffset();

                    if (action.m_modifiers != prevModifiers)
                    {
                        entityItLookupIt->second.m_initial =
                            AZ::Transform::CreateTranslation(worldTranslation - localOffset);
                    }

                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTranslation,
                        entityItLookupIt->second.m_initial.GetTranslation() + localOffset);
                }
                else
                {
                    if (action.m_modifiers != prevModifiers)
                    {
                        entityItLookupIt->second.m_initial =
                            AZ::Transform::CreateTranslation(worldTranslation - action.LocalPositionOffset());
                    }

                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTranslation,
                        entityItLookupIt->second.m_initial.GetTranslation() +
                        action.LocalPositionOffset());
                }

                ToolsApplicationNotificationBus::Broadcast(
                    &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
            }

            // if transform pivot override has been set, make sure to update it when we move it
            if (pivotOverrideFrame.m_translationOverride)
            {
                pivotOverrideFrame.m_translationOverride = entityIdManipulators.m_manipulators->GetPosition();
            }
        }

        prevModifiers = action.m_modifiers;
    }

    static void HandleAccents(
        const bool hasSelectedEntities, const AZ::EntityId entityIdUnderCursor,
        const bool ctrlHeld, AZ::EntityId& hoveredEntityId,
        const ViewportInteraction::MouseButtons mouseButtons,
        const bool usingBoxSelect)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const bool invalidMouseButtonHeld = mouseButtons.Middle() || mouseButtons.Right();

        if ((hoveredEntityId.IsValid() && hoveredEntityId != entityIdUnderCursor) ||
            (hasSelectedEntities && !ctrlHeld && hoveredEntityId.IsValid()) ||
            invalidMouseButtonHeld)
        {
            if (hoveredEntityId.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequests::SetEntityHighlighted, hoveredEntityId, false);

                hoveredEntityId.SetInvalid();
            }
        }

        if (!invalidMouseButtonHeld && !usingBoxSelect && (!hasSelectedEntities || ctrlHeld))
        {
            if (entityIdUnderCursor.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequests::SetEntityHighlighted, entityIdUnderCursor, true);

                hoveredEntityId = entityIdUnderCursor;
            }
        }
    }

    static AZ::Vector3 PickTerrainPosition(const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const int viewportId = mouseInteraction.m_interactionId.m_viewportId;
        // get unsnapped terrain position (world space)
        AZ::Vector3 worldSurfacePosition;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            worldSurfacePosition, viewportId,
            &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
            QPointFromScreenPoint(mouseInteraction.m_mousePick.m_screenCoordinates));

        // convert to local space - snap if enabled
        const AZ::Vector3 finalSurfacePosition = GridSnapping(viewportId)
            ? CalculateSnappedTerrainPosition(
                worldSurfacePosition, AZ::Transform::CreateIdentity(), viewportId, GridSize(viewportId))
            : worldSurfacePosition;

        return finalSurfacePosition;
    }

    // is the passed entity id contained with in the entity id list
    template<typename EntityIdContainer>
    static bool IsEntitySelectedInternal(AZ::EntityId entityId, const EntityIdContainer& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        const auto entityIdIt = selectedEntityIds.find(entityId);
        return entityIdIt != selectedEntityIds.end();
    }

    static EntityIdTransformMap RecordTransformsBefore(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // save initial transforms - this is necessary in cases where entities exist
        // in a hierarchy. We want to make sure a parent transform does not affect
        // a child transform (entities seeming to get the same transform applied twice)
        EntityIdTransformMap transformsBefore;
        transformsBefore.reserve(entityIds.size());
        for (const AZ::EntityId entityId : entityIds)
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            transformsBefore.insert({ entityId,  worldFromLocal });
        }

        return transformsBefore;
    }

    // ask the visible entity data cache if the entity is selectable in the viewport
    // (useful in the context of drawing when we only care about entities we can see)
    static bool SelectableInVisibleViewportCache(
        const EditorVisibleEntityDataCache& entityDataCache, const AZ::EntityId entityId)
    {
        if (auto entityIndex = entityDataCache.GetVisibleEntityIndexFromId(entityId))
        {
            return entityDataCache.IsVisibleEntitySelectableInViewport(*entityIndex);
        }

        return false;
    }

    EditorTransformComponentSelection::EditorTransformComponentSelection(
        const EditorVisibleEntityDataCache* entityDataCache)
        : m_entityDataCache(entityDataCache)
    {
        const AzFramework::EntityContextId entityContextId = GetEntityContextId();

        m_editorHelpers = AZStd::make_unique<EditorHelpers>(entityDataCache);

        PropertyEditorChangeNotificationBus::Handler::BusConnect();
        EditorEventsBus::Handler::BusConnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusConnect(entityContextId);
        ToolsApplicationNotificationBus::Handler::BusConnect();
        Camera::EditorCameraNotificationBus::Handler::BusConnect();
        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusConnect(entityContextId);
        EditorContextVisibilityNotificationBus::Handler::BusConnect(entityContextId);
        EditorContextLockComponentNotificationBus::Handler::BusConnect(entityContextId);
        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusConnect(entityContextId);

        RegisterActions();
        SetupBoxSelect();
        RefreshSelectedEntityIdsAndRegenerateManipulators();
    }

    EditorTransformComponentSelection::~EditorTransformComponentSelection()
    {
        m_selectedEntityIds.clear();
        DestroyManipulators(m_entityIdManipulators);

        UnregisterActions();

        m_pivotOverrideFrame.Reset();

        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusDisconnect();
        EditorContextLockComponentNotificationBus::Handler::BusDisconnect();
        EditorContextVisibilityNotificationBus::Handler::BusDisconnect();
        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusDisconnect();
        Camera::EditorCameraNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusDisconnect();
        EditorEventsBus::Handler::BusDisconnect();
        PropertyEditorChangeNotificationBus::Handler::BusDisconnect();
    }

    void EditorTransformComponentSelection::SetupBoxSelect()
    {
        // data to be used during a box select action
        struct EntityBoxSelectData
        {
            AZStd::unique_ptr<SelectionCommand> m_boxSelectSelectionCommand; // track all added/removed entities after a BoxSelect
            AZStd::unordered_set<AZ::EntityId> m_selectedEntityIdsBeforeBoxSelect;
            AZStd::unordered_set<AZ::EntityId> m_potentialSelectedEntityIds;
            AZStd::unordered_set<AZ::EntityId> m_potentialDeselectedEntityIds;
        };

        // lambdas capture shared_ptr by value to increment ref count
        auto entityBoxSelectData = AZStd::make_shared<EntityBoxSelectData>();

        m_boxSelect.InstallLeftMouseDown(
            [this, entityBoxSelectData](const ViewportInteraction::MouseInteractionEvent& /*mouseInteraction*/)
        {
            // begin selection undo/redo command
            entityBoxSelectData->m_boxSelectSelectionCommand =
                AZStd::make_unique<SelectionCommand>(EntityIdList(), s_entityBoxSelectUndoRedoDesc);
            // grab currently selected entities
            entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect = m_selectedEntityIds;
        });

        m_boxSelect.InstallMouseMove(
            [this, entityBoxSelectData](const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            EntityBoxSelectUpdateGeneral(
                m_boxSelect.BoxRegion(), *this, m_selectedEntityIds, entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect,
                entityBoxSelectData->m_potentialSelectedEntityIds, entityBoxSelectData->m_potentialDeselectedEntityIds,
                *m_entityDataCache, mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers,
                m_boxSelect.PreviousModifiers());
        });

        m_boxSelect.InstallLeftMouseUp(
            [this, entityBoxSelectData]()
        {
            entityBoxSelectData->m_boxSelectSelectionCommand->UpdateSelection(EntityIdVectorFromContainer(m_selectedEntityIds));

            // if we know a change in selection has occurred, record the undo step
            if (    !entityBoxSelectData->m_potentialDeselectedEntityIds.empty()
                ||  !entityBoxSelectData->m_potentialSelectedEntityIds.empty())
            {
                ScopedUndoBatch undoBatch(s_entityBoxSelectUndoRedoDesc);

                // restore manipulator overrides when undoing
                if (m_entityIdManipulators.m_manipulators && m_selectedEntityIds.empty())
                {
                    CreateEntityManipulatorDeselectCommand(undoBatch);
                }

                entityBoxSelectData->m_boxSelectSelectionCommand->SetParent(undoBatch.GetUndoBatch());
                entityBoxSelectData->m_boxSelectSelectionCommand.release();

                SetSelectedEntities(EntityIdVectorFromContainer(m_selectedEntityIds));
                // note: manipulators will be updated in AfterEntitySelectionChanged

                // clear pivot override when selection is empty
                if (m_selectedEntityIds.empty())
                {
                    m_pivotOverrideFrame.Reset();
                }
            }
            else
            {
                entityBoxSelectData->m_boxSelectSelectionCommand.reset();
            }

            entityBoxSelectData->m_potentialSelectedEntityIds.clear();
            entityBoxSelectData->m_potentialDeselectedEntityIds.clear();
            entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.clear();
        });

        m_boxSelect.InstallDisplayScene(
            [this, entityBoxSelectData]
            (const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            const auto modifiers = ViewportInteraction::KeyboardModifiers(
                ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));

            if (m_boxSelect.PreviousModifiers() != modifiers)
            {
                EntityBoxSelectUpdateGeneral(
                    m_boxSelect.BoxRegion(), *this, m_selectedEntityIds,
                    entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect,
                    entityBoxSelectData->m_potentialSelectedEntityIds,
                    entityBoxSelectData->m_potentialDeselectedEntityIds,
                    *m_entityDataCache, viewportInfo.m_viewportId, modifiers,
                    m_boxSelect.PreviousModifiers());
            }

            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(s_selectedEntityAabbColor);

            for (const AZ::EntityId entityId : entityBoxSelectData->m_potentialSelectedEntityIds)
            {
                const auto entityIdIt = entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.find(entityId);

                // don't show box when re-adding from previous selection
                if (entityIdIt != entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.end())
                {
                    continue;
                }

                AZ::EBusReduceResult<AZ::Aabb, AabbAggregator> aabbResult(AZ::Aabb::CreateNull());
                EditorComponentSelectionRequestsBus::EventResult(
                    aabbResult, entityId, &EditorComponentSelectionRequests::GetEditorSelectionBoundsViewport, viewportInfo);

                debugDisplay.DrawSolidBox(aabbResult.value.GetMin(), aabbResult.value.GetMax());
            }

            debugDisplay.DepthTestOn();
        });
    }

    EntityManipulatorCommand::State EditorTransformComponentSelection::CreateManipulatorCommandStateFromSelf() const
    {
        // if we're in the process of creating a new entity without a selection we
        // will not have created any manipulators at this point - to record this state
        // just return a default constructed EntityManipulatorCommand which will not be used
        if (!m_entityIdManipulators.m_manipulators)
        {
            return {};
        }

        return {
            BuildPivotOverride(
                m_pivotOverrideFrame.HasTranslationOverride(),
                m_pivotOverrideFrame.HasOrientationOverride()),
            TransformNormalizedScale(
                m_entityIdManipulators.m_manipulators->GetLocalTransform()),
                m_pivotOverrideFrame.m_pickedEntityIdOverride
        };
    }

    void EditorTransformComponentSelection::BeginRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // we must have an existing parent undo batch active when beginning to record
        // a manipulator command
        UndoSystem::URSequencePoint* currentUndoOperation = nullptr;
        ToolsApplicationRequests::Bus::BroadcastResult(
            currentUndoOperation, &ToolsApplicationRequests::GetCurrentUndoBatch);

        if (currentUndoOperation)
        {
            // check here if translation or orientation override are set
            m_manipulatorMoveCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);
        }
    }

    void EditorTransformComponentSelection::EndRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_manipulatorMoveCommand)
        {
            m_manipulatorMoveCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            UndoSystem::URSequencePoint* currentUndoOperation = nullptr;
            ToolsApplicationRequests::Bus::BroadcastResult(
                currentUndoOperation, &ToolsApplicationRequests::GetCurrentUndoBatch);

            AZ_Assert(currentUndoOperation, "The only way we should have reached this block is if "
                "m_manipulatorMoveCommand was created by calling BeginRecordManipulatorMouseMoveCommand. "
                "If we've reached this point and currentUndoOperation is null, something bad has happened "
                "in the undo system");

            if (currentUndoOperation)
            {
                m_manipulatorMoveCommand->SetParent(currentUndoOperation);
                m_manipulatorMoveCommand.release();
            }
        }
    }

    void EditorTransformComponentSelection::CreateTranslationManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<TranslationManipulators> translationManipulators =
            AZStd::make_unique<TranslationManipulators>(
                TranslationManipulators::Dimensions::Three,
                AZ::Transform::CreateIdentity());

        InitializeManipulators(*translationManipulators);

        ConfigureTranslationManipulatorAppearance3d(&*translationManipulators);

        translationManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        // lambdas capture shared_ptr by value to increment ref count
        auto manipulatorEntityIds = AZStd::make_shared<ManipulatorEntityIds>();

        // [ref 1.] Note: in  BaseManipulator, for every mouse down event, we automatically start
        // recording an undo command in case something changes - here we take advantage of that
        // knowledge by asking for the current undo operation to add our manipulator entity command to

        // linear
        translationManipulators->InstallLinearManipulatorMouseDownCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& action) mutable
        {
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            InitializeTranslationLookup(m_entityIdManipulators, action.m_start.m_positionSnapOffset);

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());

            // [ref 1.]
            BeginRecordManipulatorCommand();
        });

        ViewportInteraction::KeyboardModifiers prevModifiers{};
        translationManipulators->InstallLinearManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const LinearManipulator::Action& action) mutable -> void
        {
            UpdateTranslationManipulator(
                action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators,
                m_pivotOverrideFrame, prevModifiers);
        });

        translationManipulators->InstallLinearManipulatorMouseUpCallback(
            [this](const LinearManipulator::Action& /*action*/) mutable
        {
            EndRecordManipulatorCommand();
        });

        // planar
        translationManipulators->InstallPlanarManipulatorMouseDownCallback(
            [this, manipulatorEntityIds](const PlanarManipulator::Action& action)
        {
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            InitializeTranslationLookup(m_entityIdManipulators, action.m_start.m_snapOffset);

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());

            // [ref 1.]
            BeginRecordManipulatorCommand();
        });

        translationManipulators->InstallPlanarManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const PlanarManipulator::Action& action) mutable -> void
        {
            UpdateTranslationManipulator(
                action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators,
                m_pivotOverrideFrame, prevModifiers);
        });

        translationManipulators->InstallPlanarManipulatorMouseUpCallback(
            [this, manipulatorEntityIds](const PlanarManipulator::Action& /*action*/)
        {
            EndRecordManipulatorCommand();
        });

        // surface
        translationManipulators->InstallSurfaceManipulatorMouseDownCallback(
            [this, manipulatorEntityIds](const SurfaceManipulator::Action& action)
        {
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            InitializeTranslationLookup(m_entityIdManipulators, action.m_start.m_snapOffset);

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());

            // [ref 1.]
            BeginRecordManipulatorCommand();
        });

        translationManipulators->InstallSurfaceManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const SurfaceManipulator::Action& action) mutable -> void
        {
            UpdateTranslationManipulator(
                action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators,
                m_pivotOverrideFrame, prevModifiers);
        });

        translationManipulators->InstallSurfaceManipulatorMouseUpCallback(
            [this, manipulatorEntityIds](const SurfaceManipulator::Action& /*action*/)
        {
            EndRecordManipulatorCommand();
        });

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(translationManipulators);
    }

    void EditorTransformComponentSelection::CreateRotationManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<RotationManipulators> rotationManipulators =
            AZStd::make_unique<RotationManipulators>(AZ::Transform::CreateIdentity());

        InitializeManipulators(*rotationManipulators);

        rotationManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        // view
        rotationManipulators->SetLocalAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY(),
            AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            2.0f, s_xAxisColor, s_yAxisColor, s_zAxisColor);

        struct SharedRotationState
        {
            AZ::Quaternion m_savedOrientation = AZ::Quaternion::CreateIdentity();
            ReferenceFrame m_referenceFrameAtMouseDown = ReferenceFrame::Local;
            EntityIdList m_entityIds;
        };

        // lambdas capture shared_ptr by value to increment ref count
        AZStd::shared_ptr<SharedRotationState> sharedRotationState =
            AZStd::make_shared<SharedRotationState>();

        rotationManipulators->InstallLeftMouseDownCallback(
            [this, sharedRotationState](const AngularManipulator::Action& /*action*/) mutable -> void
        {
            sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();
            sharedRotationState->m_referenceFrameAtMouseDown = m_referenceFrame;
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, sharedRotationState->m_entityIds);

            for (auto& entityIdLookup : m_entityIdManipulators.m_lookups)
            {
                AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(
                    worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

                entityIdLookup.second.m_initial = worldFromLocal;
            }

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());

            // [ref 1.]
            BeginRecordManipulatorCommand();

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        ViewportInteraction::KeyboardModifiers prevModifiers{};
        rotationManipulators->InstallMouseMoveCallback(
            [this, prevModifiers, sharedRotationState]
            (const AngularManipulator::Action& action) mutable -> void
        {
            const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;
            // always store the pivot override frame so we don't lose the
            // orientation when adding/removing entities from the selection
            m_pivotOverrideFrame.m_orientationOverride = manipulatorOrientation;

            // don't update manipulator if we're rotating in an outer referenceFrame
            // frame as we're not changing that space
            if (sharedRotationState->m_referenceFrameAtMouseDown == ReferenceFrame::Local)
            {
                m_entityIdManipulators.m_manipulators->SetLocalTransform(
                    AZ::Transform::CreateFromQuaternionAndTranslation(
                        manipulatorOrientation,
                        m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));
            }

            // save state if we change the type of rotation we're doing to to prevent snapping
            if (prevModifiers != action.m_modifiers)
            {
                UpdateInitialRotation(m_entityIdManipulators);
                sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
            }

            // allow the user to modify the orientation without moving the object if ctrl is held
            if (action.m_modifiers.Ctrl())
            {
                UpdateInitialRotation(m_entityIdManipulators);
                sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
            }
            else
            {
                const auto pivotOrientation =
                    CalculateSelectionPivotOrientation(
                        m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, ReferenceFrame::Parent);

                // note: must use sorted entityIds based on hierarchy order when updating transforms
                for (const AZ::EntityId entityId : sharedRotationState->m_entityIds)
                {
                    auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                    if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                    {
                        continue;
                    }

                    // make sure we take into account how we move the axis independent of object
                    // if Ctrl was held to adjust the orientation of the axes separately
                    const AZ::Transform offsetRotation = AZ::Transform::CreateFromQuaternion(
                        sharedRotationState->m_savedOrientation * action.m_current.m_delta);

                    // local rotation
                    if (action.m_modifiers.Alt())
                    {
                        const AZ::Quaternion rotation =
                            AZ::Quaternion::CreateFromTransform(TransformNormalizedScale(
                                entityIdLookupIt->second.m_initial)).GetNormalizedExact();
                        const AZ::Vector3 position = entityIdLookupIt->second.m_initial.GetPosition();
                        const AZ::Vector3 scale = entityIdLookupIt->second.m_initial.RetrieveScaleExact();

                        const AZ::Vector3 centerOffset = CalculateCenterOffset(entityId, m_pivotMode);

                        // scale -> rotate -> translate
                        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM,
                            AZ::Transform::CreateTranslation(position) *
                            AZ::Transform::CreateFromQuaternion(rotation) *
                            AZ::Transform::CreateTranslation(centerOffset) *
                            offsetRotation *
                            AZ::Transform::CreateTranslation(-centerOffset) *
                            AZ::Transform::CreateScale(scale));
                    }
                    // parent space rotation
                    else if (action.m_modifiers.Shift())
                    {
                        const AZ::Transform pivotTransform =
                            AZ::Transform::CreateFromQuaternionAndTranslation(
                                pivotOrientation.m_worldOrientation,
                                m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());

                        const AZ::Transform transformInPivotSpace = pivotTransform.GetInverseFull() *
                            entityIdLookupIt->second.m_initial;

                        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM,
                            pivotTransform *
                            offsetRotation *
                            transformInPivotSpace);
                    }
                    else
                    {
                        const AZ::Transform pivotTransform = m_entityIdManipulators.m_manipulators->GetLocalTransform();
                        const AZ::Transform transformInPivotSpace =
                            pivotTransform.GetInverseFull() * entityIdLookupIt->second.m_initial;

                        AZ::TransformBus::Event(entityId,
                            &AZ::TransformBus::Events::SetWorldTM,
                            pivotTransform *
                            offsetRotation *
                            transformInPivotSpace);
                    }
                }

                ToolsApplicationNotificationBus::Broadcast(
                    &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
            }

            prevModifiers = action.m_modifiers;
        });

        rotationManipulators->InstallLeftMouseUpCallback(
            [this](const AngularManipulator::Action& /*action*/)
        {
            EndRecordManipulatorCommand();
        });

        rotationManipulators->Register(g_mainManipulatorManagerId);

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(rotationManipulators);
    }

    void EditorTransformComponentSelection::CreateScaleManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<ScaleManipulators> scaleManipulators =
            AZStd::make_unique<ScaleManipulators>(AZ::Transform::CreateIdentity());

        InitializeManipulators(*scaleManipulators);

        scaleManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        scaleManipulators->SetAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY(),
            AZ::Vector3::CreateAxisZ());
        scaleManipulators->ConfigureView(
            2.0f, s_xAxisColor, s_yAxisColor, s_zAxisColor);

        // lambdas capture shared_ptr by value to increment ref count
        auto manipulatorEntityIds = AZStd::make_shared<ManipulatorEntityIds>();

        scaleManipulators->InstallAxisLeftMouseDownCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& action)
        {
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            // here we are calling SetLocalScale, so order we visit entities in hierarchy is important
            for (const AZ::EntityId entityId : manipulatorEntityIds->m_entityIds)
            {
                auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                AZ::Vector3 localScale = AZ::Vector3::CreateZero();
                AZ::TransformBus::EventResult(
                    localScale, entityId, &AZ::TransformBus::Events::GetLocalScale);

                AZ::TransformBus::Event(
                    entityId, &AZ::TransformBus::Events::SetLocalScale,
                    localScale + action.m_start.m_scaleSnapOffset);

                AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(
                    worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

                entityIdLookupIt->second.m_initial = worldFromLocal;
            }

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation =
                QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        scaleManipulators->InstallAxisLeftMouseUpCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& /*action*/)
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(
                RecalculateAverageManipulatorTransform(
                    m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));
        });

        scaleManipulators->InstallAxisMouseMoveCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& action)
        {
            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (const AZ::EntityId entityId : manipulatorEntityIds->m_entityIds)
            {
                auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Transform initial = entityIdLookupIt->second.m_initial;

                const AZ::Vector3 scale = (AZ::Vector3::CreateOne() +
                    ((action.LocalScaleOffset() *
                        action.m_start.m_sign) / initial.RetrieveScale())).GetMax(AZ::Vector3(0.01f));
                const AZ::Transform scaleTransform = AZ::Transform::CreateScale(scale);

                if (action.m_modifiers.Alt())
                {
                    const AZ::Quaternion pivotRotation = AZ::Quaternion::CreateFromTransform(
                        TransformNormalizedScale(entityIdLookupIt->second.m_initial)).GetNormalizedExact();
                    const AZ::Vector3 pivotPosition = entityIdLookupIt->second.m_initial *
                        CalculateCenterOffset(entityId, m_pivotMode);

                    const AZ::Transform pivotTransform =
                        AZ::Transform::CreateFromQuaternionAndTranslation(pivotRotation, pivotPosition);

                    const AZ::Transform transformInPivotSpace = pivotTransform.GetInverseFull() * initial;

                    AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM,
                        pivotTransform *
                        scaleTransform *
                        transformInPivotSpace);
                }
                else
                {
                    const AZ::Transform pivotTransform =
                        TransformNormalizedScale(m_entityIdManipulators.m_manipulators->GetLocalTransform());
                    const AZ::Transform transformInPivotSpace = pivotTransform.GetInverseFull() * initial;

                    // pivot
                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTM,
                        pivotTransform * scaleTransform * transformInPivotSpace);
                }
            }

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        scaleManipulators->InstallUniformLeftMouseDownCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& /*action*/)
        {
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            for (auto& entityIdLookup : m_entityIdManipulators.m_lookups)
            {
                AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(
                    worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

                entityIdLookup.second.m_initial = worldFromLocal;
            }

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());
        });

        scaleManipulators->InstallUniformLeftMouseUpCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& /*action*/)
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(
                RecalculateAverageManipulatorTransform(
                    m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));
        });

        scaleManipulators->InstallUniformMouseMoveCallback(
            [this, manipulatorEntityIds](const LinearManipulator::Action& action)
        {
            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (const AZ::EntityId entityId : manipulatorEntityIds->m_entityIds)
            {
                auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Transform initial = entityIdLookupIt->second.m_initial;
                const AZ::Vector3 initialScale = initial.RetrieveScale();

                const auto sumVectorElements = [](const AZ::Vector3& vec) {
                    return vec.GetX() + vec.GetY() + vec.GetZ();
                };

                const AZ::Vector3 uniformScale = AZ::Vector3(sumVectorElements(action.LocalScaleOffset()));
                const AZ::Vector3 scale = (AZ::Vector3::CreateOne() +
                    (uniformScale / initialScale)).GetMax(AZ::Vector3(0.01f));
                const AZ::Transform scaleTransform = AZ::Transform::CreateScale(scale);

                if (action.m_modifiers.Alt())
                {
                    const AZ::Transform pivotTransform = TransformNormalizedScale(
                        entityIdLookupIt->second.m_initial);
                    const AZ::Transform transformInPivotSpace =
                        pivotTransform.GetInverseFull() * initial;

                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTM,
                        pivotTransform * scaleTransform * transformInPivotSpace);
                }
                else
                {
                    const AZ::Transform pivotTransform = TransformNormalizedScale(
                        m_entityIdManipulators.m_manipulators->GetLocalTransform());
                    const AZ::Transform transformInPivotSpace =
                        pivotTransform.GetInverseFull() * initial;

                    // pivot
                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTM,
                        pivotTransform * scaleTransform * transformInPivotSpace);
                }
            }

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(scaleManipulators);
    }

    void EditorTransformComponentSelection::InitializeManipulators(Manipulators& manipulators)
    {
        manipulators.Register(g_mainManipulatorManagerId);

        // camera editor component might have been set
        if (m_editorCameraComponentEntityId.IsValid())
        {
            for (const AZ::EntityId entityId : m_selectedEntityIds)
            {
                // if so, ensure we do not register it with the manipulators
                if (entityId != m_editorCameraComponentEntityId)
                {
                    if (IsSelectableInViewport(entityId))
                    {
                        manipulators.AddEntityId(entityId);
                        m_entityIdManipulators.m_lookups.insert_key(entityId);
                    }
                }
            }
        }
        else
        {
            // common case - editor camera component not set, ignore check
            for (const AZ::EntityId entityId : m_selectedEntityIds)
            {
                if (IsSelectableInViewport(entityId))
                {
                    manipulators.AddEntityId(entityId);
                    m_entityIdManipulators.m_lookups.insert_key(entityId);
                }
            }
        }
    }

    void EditorTransformComponentSelection::DeselectEntities()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!UndoRedoOperationInProgress())
        {
            ScopedUndoBatch undoBatch(s_entitiesDeselectUndoRedoDesc);

            // restore manipulator overrides when undoing
            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // select must happen after to ensure in the undo/redo step the selection command
            // happens before the manipulator command
            auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), s_entitiesDeselectUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();
        }

        m_selectedEntityIds.clear();
        SetSelectedEntities(EntityIdList());

        DestroyManipulators(m_entityIdManipulators);
        m_pivotOverrideFrame.Reset();
    }

    bool EditorTransformComponentSelection::SelectDeselect(const AZ::EntityId entityIdUnderCursor)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (entityIdUnderCursor.IsValid())
        {
            if (IsEntitySelectedInternal(entityIdUnderCursor, m_selectedEntityIds))
            {
                if (!UndoRedoOperationInProgress())
                {
                    RemoveEntityFromSelection(entityIdUnderCursor);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(s_entityDeselectUndoRedoDesc);

                    // store manipulator state when removing last entity from selection
                    if (m_entityIdManipulators.m_manipulators && nextEntityIds.empty())
                    {
                        CreateEntityManipulatorDeselectCommand(undoBatch);
                    }

                    auto selectionCommand =
                        AZStd::make_unique<SelectionCommand>(nextEntityIds, s_entityDeselectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }
            else
            {
                if (!UndoRedoOperationInProgress())
                {
                    AddEntityToSelection(entityIdUnderCursor);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(s_entitySelectUndoRedoDesc);
                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_entitySelectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }

            return true;
        }

        return false;
    }

    bool EditorTransformComponentSelection::HandleMouseInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        CheckDirtyEntityIds();

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        const AzFramework::CameraState cameraState = GetCameraState(viewportId);

        // set the widget context before calls to ViewportWorldToScreen so we are not
        // going to constantly be pushing/popping the widget context
        ViewportInteraction::WidgetContextGuard widgetContextGuard(viewportId);

        m_cachedEntityIdUnderCursor = m_editorHelpers->HandleMouseInteraction(cameraState, mouseInteraction);

        // for entities selected with no bounds of their own (just TransformComponent)
        // check selection against the selection indicator aabb
        for (const AZ::EntityId entityId : m_selectedEntityIds)
        {
            if (!SelectableInVisibleViewportCache(*m_entityDataCache, entityId))
            {
                continue;
            }

            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(
                worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            const AZ::Vector3 boxPosition = worldFromLocal * CalculateCenterOffset(entityId, m_pivotMode);

            const AZ::Vector3 scaledSize = AZ::Vector3(s_pivotSize) *
                CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

            if (AabbIntersectMouseRay(
                mouseInteraction.m_mouseInteraction,
                AZ::Aabb::CreateFromMinMax(boxPosition - scaledSize, boxPosition + scaledSize)))
            {
                m_cachedEntityIdUnderCursor = entityId;
            }
        }

        const AZ::EntityId entityIdUnderCursor = m_cachedEntityIdUnderCursor;

        EditorContextMenuUpdate(
            m_contextMenu, mouseInteraction);

        m_boxSelect.HandleMouseInteraction(mouseInteraction);

        if (Input::CycleManipulator(mouseInteraction))
        {
            const size_t scrollBound = 2;
            const auto nextMode = (static_cast<int>(m_mode) + scrollBound +
                (MouseWheelDelta(mouseInteraction) < 0.0f ? 1 : -1)) % scrollBound;

            SetTransformMode(static_cast<Mode>(nextMode));

            return true;
        }

        // double click to deselect all
        if (Input::DeselectAll(mouseInteraction))
        {
            // note: even if m_selectedEntityIds is technically empty, we
            // may still have an entity selected that was clicked in the
            // entity outliner - we still want to make sure the deselect all
            // action clears the selection
            DeselectEntities();
            return false;
        }

        if (!m_selectedEntityIds.empty())
        {
            // select/deselect (add/remove) entities with ctrl held
            if (Input::AdditiveIndividualSelect(mouseInteraction))
            {
                if (SelectDeselect(entityIdUnderCursor))
                {
                    if (m_selectedEntityIds.empty())
                    {
                        m_pivotOverrideFrame.Reset();
                    }

                    return false;
                }
            }

            // group copying/alignment to specific entity - 'ditto' position/orientation for group
            if (Input::GroupDitto(mouseInteraction))
            {
                if (entityIdUnderCursor.IsValid())
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(
                        worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

                    switch (m_mode)
                    {
                    case Mode::Rotation:
                        CopyOrientationToSelectedEntitiesGroup(QuaternionFromTransformNoScaling(worldFromLocal));
                        break;
                    case Mode::Scale:
                        CopyScaleToSelectedEntitiesIndividualWorld(worldFromLocal.RetrieveScaleExact());
                        break;
                    case Mode::Translation:
                        CopyTranslationToSelectedEntitiesGroup(worldFromLocal.GetTranslation());
                        break;
                    default:
                        // do nothing
                        break;
                    }

                    return false;
                }
            }

            // individual copying/alignment to specific entity - 'ditto' position/orientation for individual
            if (Input::IndividualDitto(mouseInteraction))
            {
                if (entityIdUnderCursor.IsValid())
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(
                        worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

                    switch (m_mode)
                    {
                    case Mode::Rotation:
                        CopyOrientationToSelectedEntitiesIndividual(QuaternionFromTransformNoScaling(worldFromLocal));
                        break;
                    case Mode::Scale:
                        CopyScaleToSelectedEntitiesIndividualWorld(worldFromLocal.RetrieveScaleExact());
                        break;
                    case Mode::Translation:
                        CopyTranslationToSelectedEntitiesIndividual(worldFromLocal.GetTranslation());
                        break;
                    default:
                        // do nothing
                        break;
                    }

                    return false;
                }
            }

            // try snapping to the terrain (if in Translation mode) and entity wasn't picked
            if (Input::SnapTerrain(mouseInteraction))
            {
                for(const AZ::EntityId entityId : m_selectedEntityIds)
                {
                    ScopedUndoBatch::MarkEntityDirty(entityId);
                }

                if (m_mode == Mode::Translation)
                {
                    const AZ::Vector3 finalSurfacePosition =
                        PickTerrainPosition(mouseInteraction.m_mouseInteraction);

                    // handle modifier alternatives
                    if (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt())
                    {
                        CopyTranslationToSelectedEntitiesIndividual(finalSurfacePosition);
                    }
                    else if (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
                    {
                        CopyTranslationToSelectedEntitiesGroup(finalSurfacePosition);
                    }
                }
                else if(m_mode == Mode::Rotation)
                {
                    // handle modifier alternatives
                    if (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt())
                    {
                        CopyOrientationToSelectedEntitiesIndividual(AZ::Quaternion::CreateIdentity());
                    }
                    else if (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
                    {
                        CopyOrientationToSelectedEntitiesGroup(AZ::Quaternion::CreateIdentity());
                    }
                }

                return false;
            }

            // set manipulator pivot override translation or orientation (update manipulators)
            if (Input::ManipulatorDitto(mouseInteraction))
            {
                if (m_entityIdManipulators.m_manipulators)
                {
                    ScopedUndoBatch undoBatch(s_dittoManipulatorUndoRedoDesc);

                    auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                        CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                    if (entityIdUnderCursor.IsValid())
                    {
                        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(
                            worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

                        // set orientation/translation to match picked entity
                        switch (m_mode)
                        {
                        case Mode::Rotation:
                            OverrideManipulatorOrientation(QuaternionFromTransformNoScaling(worldFromLocal));
                            break;
                        case Mode::Translation:
                            OverrideManipulatorTranslation(worldFromLocal.GetTranslation());
                            break;
                        case Mode::Scale:
                            // do nothing
                            break;
                        default:
                            break;
                        }

                        // only update pivot override when in translation or rotation mode
                        switch (m_mode)
                        {
                        case Mode::Rotation:
                            m_pivotOverrideFrame.m_pickTypes |= OptionalFrame::PickType::Orientation;
                            // [[fallthrough]]
                        case Mode::Translation:
                            m_pivotOverrideFrame.m_pickTypes |= OptionalFrame::PickType::Translation;
                            m_pivotOverrideFrame.m_pickedEntityIdOverride = entityIdUnderCursor;
                            break;
                        case Mode::Scale:
                            // do nothing
                            break;
                        default:
                            break;
                        }
                    }
                    else
                    {
                        // match the same behavior as if we pressed Ctrl+R to reset the manipulator
                        DelegateClearManipulatorOverride();
                    }

                    manipulatorCommand->SetManipulatorAfter(
                        EntityManipulatorCommand::State(
                            BuildPivotOverride(
                                m_pivotOverrideFrame.HasTranslationOverride(),
                                m_pivotOverrideFrame.HasOrientationOverride()),
                            m_entityIdManipulators.m_manipulators->GetLocalTransform(),
                            entityIdUnderCursor));

                    manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                    manipulatorCommand.release();
                }
            }

            return false;
        }

        // standard toggle selection
        if (Input::IndividualSelect(mouseInteraction))
        {
            SelectDeselect(entityIdUnderCursor);
        }

        return false;
    }

    template<typename T>
    static void AddAction(
        AZStd::vector<AZStd::unique_ptr<QAction>>& actions,
        const QList<QKeySequence>& keySequences,
        int actionId,
        const QString& name,
        const QString& statusTip,
        const T& callback)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        actions.emplace_back(AZStd::make_unique<QAction>(nullptr));

        actions.back()->setShortcuts(keySequences);
        actions.back()->setText(name);
        actions.back()->setStatusTip(statusTip);

        QObject::connect(actions.back().get(), &QAction::triggered, actions.back().get(), callback);

        EditorActionRequestBus::Broadcast(
            &EditorActionRequests::AddActionViaBus,
            actionId, actions.back().get());
    }

    void EditorTransformComponentSelection::OnEscape()
    {
        DeselectEntities();
    }

    // helper to enumerate all scene/level entities (will filter out system entities)
    template<typename Func>
    static void EnumerateEditorEntities(const Func& func)
    {
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::EnumerateEntities,
            [&func](const AZ::Entity* entity)
        {
            const AZ::EntityId entityId = entity->GetId();

            bool editorEntity = false;
            EditorEntityContextRequestBus::BroadcastResult(
                editorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);

            if (editorEntity)
            {
                func(entityId);
            }
        });
    }

    void EditorTransformComponentSelection::DelegateClearManipulatorOverride()
    {
        switch (m_mode)
        {
        case Mode::Rotation:
            ClearManipulatorOrientationOverride();
            break;
        case Mode::Scale:
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();
            break;
        case Mode::Translation:
            ClearManipulatorTranslationOverride();
            break;
        }
    }

    void EditorTransformComponentSelection::RegisterActions()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // note: see Code/Sandbox/Editor/Resource.h for ID_EDIT_<action> ids

        const auto lockUnlock = [this](const bool lock)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_lockSelectionUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);
            for (const AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityLockState(entityId, lock);
            }

            RegenerateManipulators();
        };

        // lock selection
        AddAction(m_actions, { QKeySequence(Qt::Key_L) },
            /*ID_EDIT_FREEZE =*/ 32900,
            s_lockSelectionTitle, s_lockSelectionDesc,
            [lockUnlock]()
        {
            lockUnlock(true);
        });

        // unlock selection
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::Key_L) },
            /*ID_EDIT_UNFREEZE =*/ 32973,
            s_lockSelectionTitle, s_lockSelectionDesc,
            [lockUnlock]()
        {
            lockUnlock(false);
        });

        const auto showHide = [this](const bool show)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_hideSelectionUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);
            for (const AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityVisibility(entityId, show);
            }

            RegenerateManipulators();
        };

        // hide selection
        AddAction(m_actions, { QKeySequence(Qt::Key_H) },
            /*ID_EDIT_HIDE =*/ 32898,
            s_hideSelectionTitle, s_hideSelectionDesc,
            [showHide]()
        {
            showHide(false);
        });

        // show selection
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::Key_H) },
            /*ID_EDIT_UNHIDE =*/ 32974,
            s_hideSelectionTitle, s_hideSelectionDesc,
            [showHide]()
        {
            showHide(true);
        });

        // unlock all entities in the level/scene
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L) },
            /*ID_EDIT_UNFREEZEALL =*/ 32901,
            s_unlockAllTitle, s_unlockAllDesc,
            []()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_unlockAllUndoRedoDesc);

            EnumerateEditorEntities([](const AZ::EntityId entityId)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityLockState(entityId, false);
            });
        });

        // show all entities in the level/scene
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H) },
            /*ID_EDIT_UNHIDEALL =*/ 32899,
            s_showAllTitle, s_showAllDesc,
            []()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_showAllEntitiesUndoRedoDesc);

            EnumerateEditorEntities([](const AZ::EntityId entityId)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityVisibility(entityId, true);
            });
        });

        // select all entities in the level/scene
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::Key_A) },
            /*ID_EDIT_SELECTALL =*/ 33376,
            s_selectAllTitle, s_selectAllDesc,
            [this]()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_selectAllEntitiesUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                    CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                // note, nothing will change that the manipulatorCommand needs to keep track
                // for after so no need to call SetManipulatorAfter

                manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                manipulatorCommand.release();
            }

            EnumerateEditorEntities([this](const AZ::EntityId entityId)
            {
                AddEntityToSelection(entityId);
            });

            auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

            auto selectionCommand = AZStd::make_unique<SelectionCommand>(
                nextEntityIds, s_selectAllEntitiesUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();

            SetSelectedEntities(nextEntityIds);
            RegenerateManipulators();
        });

        // invert current selection
        AddAction(m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I) },
            /*ID_EDIT_INVERTSELECTION =*/ 33692,
            s_invertSelectionTitle, s_invertSelectionDesc,
            [this]()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_invertSelectionUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                    CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                // note, nothing will change that the manipulatorCommand needs to keep track
                // for after so no need to call SetManipulatorAfter

                manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                manipulatorCommand.release();
            }

            EntityIdSet entityIds;
            EnumerateEditorEntities([this, &entityIds](const AZ::EntityId entityId)
            {
                const auto entityIdIt = AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), entityId);
                if (entityIdIt == m_selectedEntityIds.end())
                {
                    entityIds.insert(entityId);
                }
            });

            m_selectedEntityIds = entityIds;

            auto nextEntityIds = EntityIdVectorFromContainer(entityIds);

            auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_invertSelectionUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();

            SetSelectedEntities(nextEntityIds);
            RegenerateManipulators();
        });

        // duplicate selection
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_D) },
            /*ID_EDIT_CLONE =*/ 33525,
            s_duplicateTitle, s_duplicateDesc, []()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_duplicateUndoRedoDesc);
            auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), s_duplicateUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();

            bool handled = false;
            EditorRequestBus::Broadcast(&EditorRequests::CloneSelection, handled);

            // selection update handled in AfterEntitySelectionChanged
        });

        // delete selection
        AddAction(
            m_actions, { QKeySequence(Qt::Key_Delete) },
            /*ID_EDIT_DELETE=*/ 33480,
            s_deleteTitle, s_deleteDesc,
            [this]()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_deleteUndoRedoDesc);

            CreateEntityManipulatorDeselectCommand(undoBatch);

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::DeleteEntitiesAndAllDescendants,
                EntityIdVectorFromContainer(m_selectedEntityIds));

            m_selectedEntityIds.clear();
            m_pivotOverrideFrame.Reset();
        });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_Space) },
            /*ID_EDIT_ESCAPE=*/ 33513,
            "", "",
            [this]()
        {
            DeselectEntities();
        });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_P) },
            /*ID_EDIT_PIVOT=*/ 36203,
            s_togglePivotTitleEditMenu, s_togglePivotDesc,
            [this]()
        {
            ToggleCenterPivotSelection();
        });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_R) },
            /*ID_EDIT_RESET=*/ 36204,
            s_resetEntityTransformTitle, s_resetEntityTransformDesc,
            [this]()
        {
            switch (m_mode)
            {
            case Mode::Rotation:
                ResetOrientationForSelectedEntitiesLocal();
                break;
            case Mode::Scale:
                CopyScaleToSelectedEntitiesIndividualLocal(AZ::Vector3::CreateOne());
                break;
            case Mode::Translation:
                ResetTranslationForSelectedEntitiesLocal();
                break;
            }

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_R) },
            /*ID_EDIT_RESET_MANIPULATOR=*/ 36207,
            s_resetManipulatorTitle, s_resetManipulatorDesc,
            AZStd::bind(AZStd::mem_fn(&EditorTransformComponentSelection::DelegateClearManipulatorOverride), this));

        AddAction(
            m_actions, { QKeySequence(Qt::ALT + Qt::Key_R) },
            /*ID_EDIT_RESET_LOCAL=*/ 36205,
            s_resetTransformLocalTitle, s_resetTransformLocalDesc,
            [this]()
        {
            switch (m_mode)
            {
            case Mode::Rotation:
                ResetOrientationForSelectedEntitiesLocal();
                break;
            case Mode::Scale:
                CopyScaleToSelectedEntitiesIndividualWorld(AZ::Vector3::CreateOne());
                break;
            case Mode::Translation:
                // do nothing
                break;
            }

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        AddAction(
            m_actions, { QKeySequence(Qt::SHIFT + Qt::Key_R) },
            /*ID_EDIT_RESET_WORLD=*/ 36206,
            s_resetTransformWorldTitle, s_resetTransformWorldDesc,
            [this]()
        {
            switch (m_mode)
            {
            case Mode::Rotation:
                {
                    // begin an undo batch so operations inside CopyOrientation... and
                    // DelegateClear... are grouped into a single undo/redo
                    ScopedUndoBatch undoBatch { s_resetTransformWorldTitle };
                    CopyOrientationToSelectedEntitiesIndividual(AZ::Quaternion::CreateIdentity());
                    ClearManipulatorOrientationOverride();
                }
                break;
            case Mode::Scale:
            case Mode::Translation:
                break;
            }

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        });

        EditorMenuRequestBus::Broadcast(&EditorMenuRequests::RestoreEditMenuToDefault);
    }

    void EditorTransformComponentSelection::UnregisterActions()
    {
        for (auto& action : m_actions)
        {
            EditorActionRequestBus::Broadcast(
                &EditorActionRequests::RemoveActionViaBus, action.get());
        }

        m_actions.clear();
    }

    void EditorTransformComponentSelection::UnregisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Unregister();
        }
    }

    void EditorTransformComponentSelection::RegisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && !m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Register(g_mainManipulatorManagerId);
        }
    }

    void EditorTransformComponentSelection::CreateEntityIdManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_selectedEntityIds.empty())
        {
            return;
        }

        switch (m_mode)
        {
        case Mode::Translation:
            CreateTranslationManipulators();
            break;
        case Mode::Rotation:
            CreateRotationManipulators();
            break;
        case Mode::Scale:
            CreateScaleManipulators();
            break;
        }

        // workaround to ensure we never show a manipulator that is bound to no
        // entities (may happen if entities are locked/hidden but currently selected)
        if (m_entityIdManipulators.m_lookups.empty())
        {
            DestroyManipulators(m_entityIdManipulators);
        }
    }

    void EditorTransformComponentSelection::RegenerateManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // note: create/destroy pattern to be addressed
        DestroyManipulators(m_entityIdManipulators);
        CreateEntityIdManipulators();
    }

    EditorTransformComponentSelectionRequests::Mode EditorTransformComponentSelection::GetTransformMode()
    {
        return m_mode;
    }

    void EditorTransformComponentSelection::SetTransformMode(const Mode mode)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (mode == m_mode)
        {
            return;
        }

        if (m_pivotOverrideFrame.m_orientationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_orientationOverride = QuaternionFromTransformNoScaling(
                m_entityIdManipulators.m_manipulators->GetLocalTransform());
        }

        if (m_pivotOverrideFrame.m_translationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_translationOverride = m_entityIdManipulators.m_manipulators->GetPosition();
        }

        m_mode = mode;

        RegenerateManipulators();
    }

    void EditorTransformComponentSelection::UndoRedoEntityManipulatorCommand(
        const AZ::u8 pivotOverride, const AZ::Transform& transform, const AZ::EntityId entityId)
    {
        m_pivotOverrideFrame.Reset();

        RefreshSelectedEntityIdsAndRegenerateManipulators();

        if (m_entityIdManipulators.m_manipulators && PivotHasTransformOverride(pivotOverride))
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(transform);

            if (PivotHasOrientationOverride(pivotOverride))
            {
                m_pivotOverrideFrame.m_orientationOverride = QuaternionFromTransformNoScaling(transform);
            }

            if (PivotHasTranslationOverride(pivotOverride))
            {
                m_pivotOverrideFrame.m_translationOverride = transform.GetTranslation();
            }

            if (entityId.IsValid())
            {
                m_pivotOverrideFrame.m_pickedEntityIdOverride = entityId;
            }
        }
    }

    void EditorTransformComponentSelection::AddEntityToSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_selectedEntityIds.insert(entityId);
    }

    void EditorTransformComponentSelection::RemoveEntityFromSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_selectedEntityIds.erase(entityId);
    }

    bool EditorTransformComponentSelection::IsEntitySelected(const AZ::EntityId entityId) const
    {
        return IsEntitySelectedInternal(entityId, m_selectedEntityIds);
    }

    void EditorTransformComponentSelection::SetSelectedEntities(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // we are responsible for updating the current selection
        m_didSetSelectedEntities = true;
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, entityIds);
    }

    void EditorTransformComponentSelection::RefreshManipulators(const RefreshType refreshType)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            if (!m_entityIdManipulators.m_manipulators->PerformingAction())
            {
                AZ::Transform transform;
                switch (refreshType)
                {
                case RefreshType::All:
                    transform = RecalculateAverageManipulatorTransform(
                        m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame);
                    break;
                case RefreshType::Orientation:
                    transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        RecalculateAverageManipulatorOrientation(
                            m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_referenceFrame),
                        m_entityIdManipulators.m_manipulators->GetPosition());
                    break;
                case RefreshType::Translation:
                    transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        AZ::Quaternion::CreateFromTransform(
                            m_entityIdManipulators.m_manipulators->GetLocalTransform()),
                        RecalculateAverageManipulatorTranslation(
                            m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode));
                    break;
                }

                m_entityIdManipulators.m_manipulators->SetLocalTransform(transform);

                m_triedToRefresh = false;
            }
            else
            {
                m_triedToRefresh = true;
            }
        }
    }

    void EditorTransformComponentSelection::OverrideManipulatorOrientation(const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_pivotOverrideFrame.m_orientationOverride = orientation;

        if (m_entityIdManipulators.m_manipulators)
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    orientation, m_entityIdManipulators.m_manipulators->GetPosition()));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();
        }
    }

    void EditorTransformComponentSelection::OverrideManipulatorTranslation(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_pivotOverrideFrame.m_translationOverride = translation;

        if (m_entityIdManipulators.m_manipulators)
        {
            m_entityIdManipulators.m_manipulators->SetLocalPosition(translation);
            m_entityIdManipulators.m_manipulators->SetBoundsDirty();
        }
    }

    void EditorTransformComponentSelection::ClearManipulatorTranslationOverride()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_resetManipulatorTranslationUndoRedoDesc);

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedTranslation();
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();

            m_entityIdManipulators.m_manipulators->SetLocalTransform(
                RecalculateAverageManipulatorTransform(
                    m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    void EditorTransformComponentSelection::ClearManipulatorOrientationOverride()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch { s_resetManipulatorOrientationUndoRedoDesc };

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedOrientation();
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();

            m_entityIdManipulators.m_manipulators->SetLocalTransform(
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    CalculatePivotOrientationForEntityIds(
                        m_entityIdManipulators.m_lookups, ReferenceFrame::Local).m_worldOrientation,
                    m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    void EditorTransformComponentSelection::ToggleCenterPivotSelection()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_pivotMode = TogglePivotMode(m_pivotMode);
        RefreshManipulators(RefreshType::Translation);
    }

    template<typename EntityIdMap>
    static bool ShouldUpdateEntityTransform(const AZ::EntityId entityId, const EntityIdMap& entityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value,
            "Container key type is not an EntityId");

        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);
        while (parentId.IsValid())
        {
            const auto parentIt = entityIdMap.find(parentId);
            if (parentIt != entityIdMap.end())
            {
                return false;
            }

            const AZ::EntityId currentParentId = parentId;
            parentId.SetInvalid();
            AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
        }

        return true;
    }

    void EditorTransformComponentSelection::CopyTranslationToSelectedEntitiesGroup(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoTranslationGroupUndoRedoDesc);

            // store previous translation manipulator position
            const AZ::Vector3 previousPivotTranslation = m_entityIdManipulators.m_manipulators->GetPosition();

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(
                EntityManipulatorCommand::State(
                    BuildPivotOverride(
                        m_pivotOverrideFrame.HasTranslationOverride(),
                        m_pivotOverrideFrame.HasOrientationOverride()),
                    AZ::Transform::CreateFromQuaternionAndTranslation(
                        QuaternionFromTransformNoScaling(
                            m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation),
                    m_pivotOverrideFrame.m_pickedEntityIdOverride));

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                // do not update position of child if parent/ancestor is in selection
                if (!ShouldUpdateEntityTransform(entityId, m_entityIdManipulators.m_lookups))
                {
                    continue;
                }

                const AZ::Vector3 entityTranslation = GetWorldTranslation(entityId);

                AZ::TransformBus::Event(
                    entityId, &AZ::TransformBus::Events::SetWorldTranslation,
                    translation + (entityTranslation - previousPivotTranslation));
            }

            RefreshManipulators(RefreshType::Translation);

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        }
    }

    void EditorTransformComponentSelection::CopyTranslationToSelectedEntitiesIndividual(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoTranslationIndividualUndoRedoDesc);

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(
                EntityManipulatorCommand::State(
                    BuildPivotOverride(
                        m_pivotOverrideFrame.HasTranslationOverride(),
                        m_pivotOverrideFrame.HasOrientationOverride()),
                    AZ::Transform::CreateFromQuaternionAndTranslation(
                        QuaternionFromTransformNoScaling(
                            m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation),
                    m_pivotOverrideFrame.m_pickedEntityIdOverride));

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                AZ::TransformBus::Event(
                    entityId, &AZ::TransformBus::Events::SetWorldTranslation, translation);
            }

            RefreshManipulators(RefreshType::Translation);

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        }
    }

    void EditorTransformComponentSelection::CopyScaleToSelectedEntitiesIndividualWorld(const AZ::Vector3& scale)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_dittoScaleIndividualWorldUndoRedoDesc);

        ManipulatorEntityIds manipulatorEntityIds;
        BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

        // save initial transforms - this is necessary in cases where entities exist
        // in a hierarchy. We want to make sure a parent transform does not affect
        // a child transform (entities seeming to get the same transform applied twice)
        const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

        // update scale relative to initial
        const AZ::Transform scaleTransform = AZ::Transform::CreateScale(scale);
        for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);

            const auto transformIt = transformsBefore.find(entityId);
            if (transformIt != transformsBefore.end())
            {
                AZ::Transform transformBefore = transformIt->second;
                transformBefore.ExtractScaleExact();
                AZ::Transform newWorldFromLocal = transformBefore * scaleTransform;

                AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, newWorldFromLocal);
            }
        }

        ToolsApplicationNotificationBus::Broadcast(
            &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
    }

    void EditorTransformComponentSelection::CopyScaleToSelectedEntitiesIndividualLocal(const AZ::Vector3& scale)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_dittoScaleIndividualLocalUndoRedoDesc);

        ManipulatorEntityIds manipulatorEntityIds;
        BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

        // update scale relative to initial
        for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);
            AZ::TransformBus::Event(
                entityId, &AZ::TransformBus::Events::SetLocalScale, scale);
        }

        ToolsApplicationNotificationBus::Broadcast(
            &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
    }

    void EditorTransformComponentSelection::CopyOrientationToSelectedEntitiesIndividual(
        const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch { s_dittoEntityOrientationIndividualUndoRedoDesc };

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            // save initial transforms
            const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

            // update orientations relative to initial
            for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);

                const auto transformIt = transformsBefore.find(entityId);
                if (transformIt != transformsBefore.end())
                {
                    AZ::Transform newWorldFromLocal = transformIt->second;
                    const AZ::Vector3 scale = newWorldFromLocal.RetrieveScaleExact();
                    newWorldFromLocal.SetRotationPartFromQuaternion(orientation);
                    newWorldFromLocal *= AZ::Transform::CreateScale(scale);

                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetWorldTM, newWorldFromLocal);
                }
            }

            OverrideManipulatorOrientation(orientation);

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        }
    }

    void EditorTransformComponentSelection::CopyOrientationToSelectedEntitiesGroup(
        const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoEntityOrientationGroupUndoRedoDesc);

            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            // save initial transforms
            const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

            const AZ::Transform currentTransform =
                m_entityIdManipulators.m_manipulators->GetLocalTransform();
            const AZ::Transform nextTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                orientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());

            for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                if (!ShouldUpdateEntityTransform(entityId, m_entityIdManipulators.m_lookups))
                {
                    continue;
                }

                const auto transformIt = transformsBefore.find(entityId);
                if (transformIt != transformsBefore.end())
                {
                    const AZ::Transform transformInPivotSpace =
                        currentTransform.GetInverseFull() * transformIt->second;

                    AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM,
                        nextTransform * transformInPivotSpace);
                }
            }

            OverrideManipulatorOrientation(orientation);

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        }
    }

    void EditorTransformComponentSelection::ResetOrientationForSelectedEntitiesLocal()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_resetOrientationToParentUndoRedoDesc);
        for (const auto& entityIdLookup: m_entityIdManipulators.m_lookups)
        {
            ScopedUndoBatch::MarkEntityDirty(entityIdLookup.first);
            AZ::TransformBus::Event(
                entityIdLookup.first, &AZ::TransformBus::Events::SetLocalRotation,
                AZ::Vector3::CreateZero());
        }

        ClearManipulatorOrientationOverride();
    }

    void EditorTransformComponentSelection::ResetTranslationForSelectedEntitiesLocal()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_resetTranslationToParentUndoRedoDesc);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(
                m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (const AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                AZ::EntityId parentId;
                AZ::TransformBus::EventResult(
                    parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                if (parentId.IsValid())
                {
                    ScopedUndoBatch::MarkEntityDirty(entityId);
                    AZ::TransformBus::Event(
                        entityId, &AZ::TransformBus::Events::SetLocalTranslation,
                        AZ::Vector3::CreateZero());
                }
            }

            ClearManipulatorTranslationOverride();
        }
    }

    void EditorTransformComponentSelection::PopulateEditorGlobalContextMenu(
        QMenu* menu, const AZ::Vector2& /*point*/, const int /*flags*/)
    {
        QAction* action = menu->addAction(QObject::tr(s_togglePivotTitleRightClick));
        QObject::connect(action, &QAction::triggered, action, [this]() { ToggleCenterPivotSelection(); });
    }

    void EditorTransformComponentSelection::BeforeEntitySelectionChanged()
    {
        // EditorTransformComponentSelection was not responsible for the change in selection
        if (!m_didSetSelectedEntities)
        {
            if (!UndoRedoOperationInProgress())
            {
                // we know we will be tracking an undo step when selection changes so ensure
                // we also record the state of the manipulator before and after the change
                BeginRecordManipulatorCommand();

                // ensure we reset/refresh any pivot overrides when a selection
                // change originated outside EditorTransformComponentSelection
                m_pivotOverrideFrame.Reset();
            }
        }
    }

    void EditorTransformComponentSelection::AfterEntitySelectionChanged(
        const EntityIdList& /*newlySelectedEntities*/, const EntityIdList& /*newlyDeselectedEntities*/)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // EditorTransformComponentSelection was not responsible for the change in selection
        if (!m_didSetSelectedEntities)
        {
            if (!UndoRedoOperationInProgress())
            {
                EndRecordManipulatorCommand();
            }

            RefreshSelectedEntityIds();
        }
        else
        {
            // clear if we instigated the selection change after selection changes
            m_didSetSelectedEntities = false;
        }

        RegenerateManipulators();
    }

    static void DrawPreviewAxis(
        AzFramework::DebugDisplayRequests& display, const AZ::Transform& transform,
        const AZ::VectorFloat axisLength, const AzFramework::CameraState& cameraState)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const int prevState = display.GetState();

        display.DepthWriteOff();
        display.DepthTestOff();
        display.CullOff();

        display.SetLineWidth(4.0f);

        const auto axisFlip = [&transform, &cameraState](const AZ::Vector3& axis) -> AZ::VectorFloat
        {
            return ShouldFlipCameraAxis(
                AZ::Transform::CreateIdentity(), transform.GetTranslation(),
                TransformDirectionNoScaling(transform, axis), cameraState)
                ? -AZ::VectorFloat::CreateOne()
                : AZ::VectorFloat::CreateOne();
        };

        display.SetColor(s_fadedXAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafeExact() *
            axisLength * axisFlip(AZ::Vector3::CreateAxisX()));
        display.SetColor(s_fadedYAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafeExact() *
            axisLength * axisFlip(AZ::Vector3::CreateAxisY()));
        display.SetColor(s_fadedZAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisZ().GetNormalizedSafeExact() *
            axisLength * axisFlip(AZ::Vector3::CreateAxisZ()));

        display.DepthWriteOn();
        display.DepthTestOn();
        display.CullOn();
        display.SetLineWidth(1.0f);

        display.SetState(prevState);
    }

    void EditorTransformComponentSelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        CheckDirtyEntityIds();

        const auto modifiers = ViewportInteraction::KeyboardModifiers(
            ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));

        HandleAccents(
            !m_selectedEntityIds.empty(), m_cachedEntityIdUnderCursor,
            modifiers.Ctrl(), m_hoveredEntityId,
            ViewportInteraction::BuildMouseButtons(
                QGuiApplication::mouseButtons()), m_boxSelect.Active());

        const ReferenceFrame referenceFrame = modifiers.Shift()
            ? ReferenceFrame::Parent
            : ReferenceFrame::Local;

        bool refresh = false;
        if (referenceFrame != m_referenceFrame)
        {
            m_referenceFrame = referenceFrame;
            refresh = true;
        }

        refresh = refresh
            ||  (m_triedToRefresh
                && m_entityIdManipulators.m_manipulators
                && !m_entityIdManipulators.m_manipulators->PerformingAction());

        // we've moved from local to parent space (or vice versa) by holding or
        // releasing shift - make sure we update the manipulator orientation appropriately
        if (refresh)
        {
            RefreshManipulators(RefreshType::Orientation);
        }

        const AzFramework::CameraState cameraState = GetCameraState(viewportInfo.m_viewportId);

        const auto entityFilter = [this](const AZ::EntityId entityId)
        {
            const bool entityHasManipulator =
                m_entityIdManipulators.m_lookups.find(entityId) != m_entityIdManipulators.m_lookups.end();

            return !entityHasManipulator;
        };

        m_editorHelpers->DisplayHelpers(viewportInfo, cameraState, debugDisplay, entityFilter);

        if (!m_selectedEntityIds.empty())
        {
            if (m_pivotOverrideFrame.m_pickedEntityIdOverride.IsValid())
            {
                const AZ::Transform pickedEntityWorldTransform =
                    AZ::Transform::CreateFromQuaternionAndTranslation(
                        CalculatePivotOrientation(
                            m_pivotOverrideFrame.m_pickedEntityIdOverride, referenceFrame).m_worldOrientation,
                        CalculatePivotTranslation(
                            m_pivotOverrideFrame.m_pickedEntityIdOverride, m_pivotMode));

                const AZ::VectorFloat scaledSize = s_pivotSize *
                    CalculateScreenToWorldMultiplier(pickedEntityWorldTransform.GetTranslation(), cameraState);

                debugDisplay.DepthWriteOff();
                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(s_pickedOrientationColor);

                debugDisplay.DrawWireSphere(pickedEntityWorldTransform.GetTranslation(), scaledSize);

                debugDisplay.DepthTestOn();
                debugDisplay.DepthWriteOn();
            }

            // check what pivot orientation we are in (based on if the a modifier is
            // held to move us to parent space) or if we set a pivot override
            const auto pivotResult = CalculateSelectionPivotOrientation(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_referenceFrame);

            // if the reference frame was parent space and the selection does have a
            // valid parent, draw a preview axis at its position/orientation
            if (pivotResult.m_parentId.IsValid())
            {
                if (auto parentEntityIndex = m_entityDataCache->GetVisibleEntityIndexFromId(pivotResult.m_parentId))
                {
                    const AZ::Transform& worldFromLocal = m_entityDataCache->GetVisibleEntityTransform(*parentEntityIndex);

                    const AZ::VectorFloat adjustedLineLength =
                        CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

                    DrawPreviewAxis(debugDisplay, worldFromLocal, adjustedLineLength, cameraState);
                }
            }

            debugDisplay.DepthTestOff();
            debugDisplay.DepthWriteOff();

            for (const AZ::EntityId entityId : m_selectedEntityIds)
            {
                if (auto entityIndex = m_entityDataCache->GetVisibleEntityIndexFromId(entityId))
                {
                    const AZ::Transform& worldFromLocal = m_entityDataCache->GetVisibleEntityTransform(*entityIndex);
                    const bool hidden = !m_entityDataCache->IsVisibleEntityVisible(*entityIndex);
                    const bool locked = m_entityDataCache->IsVisibleEntityLocked(*entityIndex);

                    const AZ::Vector3 boxPosition = worldFromLocal * CalculateCenterOffset(entityId, m_pivotMode);

                    const AZ::Vector3 scaledSize = AZ::Vector3(s_pivotSize) *
                        CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

                    const AZ::Color hiddenNormal[] = { AzFramework::ViewportColors::SelectedColor, AzFramework::ViewportColors::HiddenColor };
                    AZ::Color boxColor = hiddenNormal[hidden];
                    const AZ::Color lockedOther[] = { boxColor, AzFramework::ViewportColors::LockColor };
                    boxColor = lockedOther[locked];

                    debugDisplay.SetColor(boxColor);

                    // draw the pivot (origin) of the box
                    debugDisplay.DrawWireBox(boxPosition - scaledSize, boxPosition + scaledSize);
                }
            }

            debugDisplay.DepthWriteOn();
            debugDisplay.DepthTestOn();
        }

        // draw a preview axis of where the manipulator was when the action started
        // only do this while the manipulator is being interacted with
        if (m_entityIdManipulators.m_manipulators)
        {
            // if we have an active manipulator, ensure we refresh the view while drawing
            // in case it needs to be recalculated based on the current view position
            m_entityIdManipulators.m_manipulators->RefreshView(cameraState.m_position);

            if (m_entityIdManipulators.m_manipulators->PerformingAction())
            {
                const float adjustedLineLength = 2.0f *
                    CalculateScreenToWorldMultiplier(m_axisPreview.m_translation, cameraState);

                DrawPreviewAxis(debugDisplay, AZ::Transform::CreateFromQuaternionAndTranslation(
                    m_axisPreview.m_orientation, m_axisPreview.m_translation),
                    adjustedLineLength, cameraState);
            }
        }

        m_boxSelect.DisplayScene(viewportInfo, debugDisplay);
    }

    void EditorTransformComponentSelection::DisplayViewportSelection2d(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_boxSelect.Display2d(debugDisplay);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // check what the 'authoritative' selected entity ids are after an undo/redo
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        RefreshSelectedEntityIds(selectedEntityIds);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds(const EntityIdList& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // update selected entityId set
        m_selectedEntityIds.clear();
        m_selectedEntityIds.reserve(selectedEntityIds.size());
        AZStd::copy(selectedEntityIds.begin(), selectedEntityIds.end(),
            AZStd::inserter(m_selectedEntityIds, m_selectedEntityIds.end()));
    }

    void EditorTransformComponentSelection::OnComponentPropertyChanged(const AZ::Uuid componentType)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // the user made a change in the entity inspector - ensure the manipulator updates
        // to reflect the new entity transform
        if (componentType == AZ::EditorTransformComponentTypeId)
        {
            RefreshManipulators(RefreshType::All);
        }
    }

    void EditorTransformComponentSelection::OnViewportViewEntityChanged(const AZ::EntityId& newViewId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // if a viewport view entity has been set (e.g. we have set EditorCameraComponent to
        // match the editor camera translation/orientation), record the entity id if we have
        // a manipulator tracking it (entity id exists in m_entityIdManipulator lookups)
        // and remove it when recreating manipulators (see InitializeManipulators)
        if (newViewId.IsValid())
        {
            const auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(newViewId);
            if (entityIdLookupIt != m_entityIdManipulators.m_lookups.end())
            {
                m_editorCameraComponentEntityId = newViewId;
                RegenerateManipulators();
            }
        }
        else
        {
            if (m_editorCameraComponentEntityId.IsValid())
            {
                const auto editorCameraEntityId = m_editorCameraComponentEntityId;
                m_editorCameraComponentEntityId.SetInvalid();

                if (m_entityIdManipulators.m_manipulators)
                {
                    if (IsEntitySelectedInternal(editorCameraEntityId, m_selectedEntityIds))
                    {
                        RegenerateManipulators();
                    }
                }
            }
        }
    }

    void EditorTransformComponentSelection::CheckDirtyEntityIds()
    {
        if (m_selectedEntityIdsAndManipulatorsDirty)
        {
            RefreshSelectedEntityIdsAndRegenerateManipulators();
            m_selectedEntityIdsAndManipulatorsDirty = false;
        }
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIdsAndRegenerateManipulators()
    {
        RefreshSelectedEntityIds();
        RegenerateManipulators();
    }

    void EditorTransformComponentSelection::OnEntityVisibilityChanged(
        const AZ::EntityId /*entityId*/, const bool /*visibility*/)
    {
        m_selectedEntityIdsAndManipulatorsDirty = true;
    }

    void EditorTransformComponentSelection::OnEntityLockChanged(
        const AZ::EntityId /*entityId*/, const bool /*locked*/)
    {
        m_selectedEntityIdsAndManipulatorsDirty = true;
    }

    void EditorTransformComponentSelection::EnteredComponentMode(
        const AZStd::vector<AZ::Uuid>& /*componentModeTypes*/)
    {
        EditorContextLockComponentNotificationBus::Handler::BusDisconnect();
        EditorContextVisibilityNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
    }

    void EditorTransformComponentSelection::LeftComponentMode(
        const AZStd::vector<AZ::Uuid>& /*componentModeTypes*/)
    {
        const AzFramework::EntityContextId entityContextId = GetEntityContextId();

        ToolsApplicationNotificationBus::Handler::BusConnect();
        EditorContextVisibilityNotificationBus::Handler::BusConnect(entityContextId);
        EditorContextLockComponentNotificationBus::Handler::BusConnect(entityContextId);
    }

    void EditorTransformComponentSelection::CreateEntityManipulatorDeselectCommand(ScopedUndoBatch& undoBatch)
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            auto manipulatorCommand = AZStd::make_unique<EntityManipulatorCommand>(
                CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    AZStd::optional<AZ::Transform> EditorTransformComponentSelection::GetManipulatorTransform()
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            return { m_entityIdManipulators.m_manipulators->GetLocalTransform() };
        }

        return {};
    }
} // namespace AzToolsFramework