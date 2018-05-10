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

namespace GraphCanvas
{    
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
    };

    using AssetEditorSettingsRequestBus = AZ::EBus<AssetEditorSettingsRequests>;

    // These are used to signal out to the editor on the whole, and general involve more singular elements rather then
    // per graph elements(so things like keeping track of which graph is active).
    class AssetEditorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        //! Request to create a new Graph. Returns the GraphId that represents the newly created Graph.
        virtual GraphId CreateNewGraph() = 0;
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
