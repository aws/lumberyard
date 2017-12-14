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
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

namespace GraphCanvas
{
    struct Endpoint;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;
    namespace Widget
    {
        struct GraphTabMetadata;
    }

    class GeneralRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Opens an existing graph and returns the tab index in which it was open in.
        //! \param Asset structure used for holding ScriptCanvas Graph
        //! \param hostId of optional entity id of entity used to open the ScriptCanvas Asset. This will be used to track the Entity Context in which the graph was loaded in
        //! \return index of open tab if the asset was able to be open successfully or error message of why the open failed
        virtual AZ::Outcome<int, AZStd::string> OpenScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex = -1, AZ::EntityId hostId = AZ::EntityId()) = 0;
        virtual int CloseScriptCanvasAsset(const AZ::Data::AssetId&) = 0;

        virtual void OnChangeActiveGraphTab(const Widget::GraphTabMetadata&) {}

        virtual AZ::EntityId GetActiveSceneId() const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetActiveGraphId() const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetGraphId(const AZ::EntityId& /*sceneId*/) const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetSceneId(const AZ::EntityId& /*graphId*/) const
        {
            return AZ::EntityId();
        }

        virtual void UpdateName(const AZ::EntityId& /*graphId*/, const AZStd::string& /*name*/) {}

        virtual void DeleteNodes(const AZ::EntityId& /*sceneId*/, const AZStd::vector<AZ::EntityId>& /*nodes*/){}
        virtual void DeleteConnections(const AZ::EntityId& /*sceneId*/, const AZStd::vector<AZ::EntityId>& /*connections*/){}

        virtual void DisconnectEndpoints(const AZ::EntityId& /*sceneId*/, const AZStd::vector<GraphCanvas::Endpoint>& /*endpoints*/) {}

        virtual void PostUndoPoint(AZ::EntityId /*sceneId*/) = 0;
        virtual void SignalSceneDirty(const AZ::EntityId& sceneId) = 0;

        // Increment the value of the ignore undo point tracker
        virtual void PushPreventUndoStateUpdate() = 0;
        // Decrement the value of the ignore undo point tracker
        virtual void PopPreventUndoStateUpdate() = 0;
        // Sets the value of the ignore undo point tracker to 0.
        // Therefore allowing undo points to be posted
        virtual void ClearPreventUndoStateUpdate() = 0;
    };

    using GeneralRequestBus = AZ::EBus<GeneralRequests>;

    class NodeCreationNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) = 0;
    };

    using NodeCreationNotificationBus = AZ::EBus<NodeCreationNotifications>;
}
