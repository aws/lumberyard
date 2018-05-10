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

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <ScriptCanvas/Core/NodeBus.h>

namespace ScriptCanvasEditor
{
    class DynamicSlotComponent
        : public AZ::Component
        , public GraphCanvas::SceneMemberNotificationBus::Handler
        , public ScriptCanvas::NodeNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(DynamicSlotComponent, "{977152B6-1A7D-49A4-8E70-644AFAD1586A}");
        static void Reflect(AZ::ReflectContext* serializeContext);
         
        DynamicSlotComponent();
        DynamicSlotComponent(GraphCanvas::SlotGroup slotGroup);
        ~DynamicSlotComponent() override = default;
        
        // AZ::Component
        void Init();
        void Activate();
        void Deactivate();
        ////

        // GraphCanvas::NodeNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////
        
        // ScriptCanvas::EditorNodeNotificationBus
        void OnSlotAdded(const ScriptCanvas::SlotId& slotId) override;
        void OnSlotRemoved(const ScriptCanvas::SlotId& slotId) override;
        ////

    private:
        GraphCanvas::SlotGroup m_slotGroup;
    };
}