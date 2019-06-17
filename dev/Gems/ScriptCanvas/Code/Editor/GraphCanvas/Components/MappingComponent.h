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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/SlotMappingBus.h>

namespace ScriptCanvasEditor
{
    class SlotMappingComponent
        : public AZ::Component
        , public GraphCanvas::NodeNotificationBus::Handler
        , public SlotMappingRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SlotMappingComponent, "{94DBC04C-964D-46A0-AD66-6A779FE4DC61}");
        
        static void Reflect(AZ::ReflectContext* context);
        
        SlotMappingComponent() = default;
        ~SlotMappingComponent() = default;
        
        void Activate();
        void Deactivate();
        
        // GraphCanvas::NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId&) override;
        
        void OnSlotAdded(const AZ::EntityId& slotId) override;
        void OnSlotRemoved(const AZ::EntityId& slotId) override;
        ////
        
        // ScriptCanvasSlotRemappingBus
        AZ::EntityId MapToGraphCanvasId(const ScriptCanvas::SlotId& slotId) override;
        ////
        
    private:    
        AZStd::unordered_map< ScriptCanvas::SlotId, AZ::EntityId > m_slotMapping;
    };    
}