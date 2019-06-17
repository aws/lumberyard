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
#include <AzCore/Math/Vector2.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

#include <ScriptCanvas/Data/Data.h>

namespace GraphCanvas
{
    struct Endpoint;

    class GraphCanvasMimeEvent;
    class GraphCanvasTreeItem;
}

namespace ScriptCanvasEditor
{
    struct CategoryInformation;
    struct NodePaletteModelInformation;

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

        virtual AZ::Outcome<int, AZStd::string> OpenScriptCanvasAssetId(const AZ::Data::AssetId& scriptCanvasAsset) = 0;

        //! Opens an existing graph and returns the tab index in which it was open in.
        //! \param Asset structure used for holding ScriptCanvas Graph
        //! \return index of open tab if the asset was able to be open successfully or error message of why the open failed
        virtual AZ::Outcome<int, AZStd::string> OpenScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex = -1) = 0;
        virtual int CloseScriptCanvasAsset(const AZ::Data::AssetId&) = 0;

        virtual bool IsScriptCanvasAssetOpen(const AZ::Data::AssetId& assetId) const = 0;

        virtual void OnChangeActiveGraphTab(const Widget::GraphTabMetadata&) {}

        virtual AZ::EntityId GetActiveScriptCanvasGraphId() const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetActiveGraphCanvasGraphId() const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetGraphCanvasGraphId(const AZ::EntityId& scriptCanvasGraphId) const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId GetScriptCanvasGraphId(const AZ::EntityId& graphCanvasSceneId) const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId FindGraphCanvasGraphIdByAssetId(const AZ::Data::AssetId& assetId) const
        {
            return AZ::EntityId();
        }

        virtual AZ::EntityId FindScriptCanvasGraphIdByAssetId(const AZ::Data::AssetId& assetId) const
        {
            return AZ::EntityId();
        }
        
        virtual bool IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const = 0;

        virtual bool IsActiveInUndoRedo() const = 0;

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

        virtual void TriggerUndo() = 0;
        virtual void TriggerRedo() = 0;

        virtual const CategoryInformation* FindNodePaletteCategoryInformation(AZStd::string_view categoryPath) const = 0;
        virtual const NodePaletteModelInformation* FindNodePaletteModelInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const = 0;
    };

    using GeneralRequestBus = AZ::EBus<GeneralRequests>;

    class GeneralAssetNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        virtual void OnAssetVisualized() {};
        virtual void OnAssetUnloaded() {};
    };

    using GeneralAssetNotificationBus = AZ::EBus<GeneralAssetNotifications>;

    class NodeCreationNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) = 0;
    };

    using NodeCreationNotificationBus = AZ::EBus<NodeCreationNotifications>;

    class VariablePaletteRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void RegisterVariableType(const ScriptCanvas::Data::Type& variabletype) = 0;
    };

    using VariablePaletteRequestBus = AZ::EBus<VariablePaletteRequests>;

    class AutomationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual NodeIdPair ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos) = 0;
        virtual const GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRoot() const = 0;

        virtual void SignalAutomationBegin() = 0;
        virtual void SignalAutomationEnd() = 0;
    };

    using AutomationRequestBus = AZ::EBus<AutomationRequests>;
}
