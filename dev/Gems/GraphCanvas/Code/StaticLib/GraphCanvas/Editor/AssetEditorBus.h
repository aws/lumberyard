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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace AZ
{
    class Entity;
}

namespace GraphCanvas
{
    class EditorContextMenu;

    class AssetEditorSettingsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = EditorId;

        //! Get the snapping distance for connections around slots
        virtual double GetSnapDistance() const { return 10.0; }

        virtual bool IsBookmarkViewportControlEnabled() const { return false; };

        // Various advance connection feature controls.
        virtual bool IsDragNodeCouplingEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDragCouplingTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsDragConnectionSpliceEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDragConnectionSpliceTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsDropConnectionSpliceEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDropConnectionSpliceTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsSplicedNodeNudgingEnabled() const { return false; }

        // Shake Configuration
        virtual bool IsShakeToDespliceEnabled() const { return false; }

        virtual int GetShakesToDesplice() const { return 3; };

        // REturns the minimum amount of distance and object must move in order for it to be considered
        // a shake.
        virtual float GetMinimumShakePercent() const { return 40.0f; }

        // Returns the minimum amount of distance the cursor must move before shake processing begins
        virtual float GetShakeDeadZonePercent() const { return 20.0f; }

        // Returns how 'straight' the given shakes must be in order to be classified.
        virtual float GetShakeStraightnessPercent() const { return 0.75f; }

        virtual AZStd::chrono::milliseconds GetMaximumShakeDuration() const { return AZStd::chrono::milliseconds(1000); }

        // Alignment
        virtual AZStd::chrono::milliseconds GetAlignmentTime() const { return AZStd::chrono::milliseconds(250); }

        // Edge of Screen Pan Configurations
        virtual float GetEdgePanningPercentage() const { return 0.1f; }
        virtual float GetEdgePanningScrollSpeed() const { return 100.0f; }
    };

    using AssetEditorSettingsRequestBus = AZ::EBus<AssetEditorSettingsRequests>;

    class AssetEditorSettingsNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;        
        using BusIdType = EditorId;

        virtual void OnSettingsChanged() {}
    };

    using AssetEditorSettingsNotificationBus = AZ::EBus<AssetEditorSettingsNotifications>;

    // These are used to signal out to the editor on the whole, and general involve more singular elements rather then
    // per graph elements(so things like keeping track of which graph is active).
    class AssetEditorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        // Signal to the editor that a lot of selection events are going to be occurring
        // And certain actions can wait until these are complete before triggering the next state.
        virtual void OnSelectionManipulationBegin() {};
        virtual void OnSelectionManipulationEnd() {};

        //! Request to create a new Graph. Returns the GraphId that represents the newly created Graph.
        virtual GraphId CreateNewGraph() = 0;

        virtual void CustomizeConnectionEntity(AZ::Entity* connectionEntity)
        {
            AZ_UNUSED(connectionEntity);
        }

        virtual ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;
        virtual ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowConnectionContextMenu(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;
    };

    using AssetEditorRequestBus = AZ::EBus< AssetEditorRequests >;

    class AssetEditorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual void OnGraphLoaded(const GraphId& /*graphId*/) {}
        virtual void OnGraphRefreshed(const GraphId& /*oldGraphId*/, const GraphId& /*newGraphId*/) {}
        virtual void OnGraphUnloaded(const GraphId& /*graphId*/) {}

        virtual void PreOnActiveGraphChanged() {}
        virtual void OnActiveGraphChanged(const AZ::EntityId& /*graphId*/) {}
        virtual void PostOnActiveGraphChanged() {}
    };

    using AssetEditorNotificationBus = AZ::EBus<AssetEditorNotifications>;
}
