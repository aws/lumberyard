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

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptCanvas/GraphCanvas/VersionControlledNodeBus.h>

namespace ScriptCanvasEditor
{
    class ScriptEventSenderNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public GraphCanvas::NodeNotificationBus::Handler
        , public AZ::Data::AssetBus::Handler
        , public VersionControlledNodeInterface
    {
    public:    
        AZ_COMPONENT(ScriptEventSenderNodeDescriptorComponent, "{6B646A3A-CB7F-49C4-8146-D848F418E0B1}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        ScriptEventSenderNodeDescriptorComponent();
        ScriptEventSenderNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);
        ~ScriptEventSenderNodeDescriptorComponent() = default;

        // Component
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeNotificationBus::Handler
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        ////
        
        // AZ::Data::AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // VersionControlledNodeInterface
        bool IsOutOfDate() const override;

        void UpdateNodeVersion() override;
        ////

    private:

        void UpdateTitles();

        AZ::EntityId m_scriptCanvasId;
    
        AZ::Data::AssetId m_assetId;
        ScriptCanvas::EBusEventId m_eventId;
    };
}