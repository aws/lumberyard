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

#include <Components/Slots/SlotComponent.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>

namespace GraphCanvas
{
    class DataSlotComponent
        : public SlotComponent
        , public DataSlotRequestBus::Handler
        , public NodeNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(DataSlotComponent, "{DB13C73D-2453-44F8-BB38-316C90264B73}", SlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);		
        
        static AZ::Entity* CreateDataSlot(const AZ::EntityId& nodeId, const AZ::Uuid& dataTypeId, bool isReference, const SlotConfiguration& slotConfiguration);
        static AZ::Entity* CreateVariableSlot(const AZ::EntityId& nodeId, const AZ::Uuid& dataTypeId, const AZ::EntityId& variableId, const SlotConfiguration& slotConfiguration);
        
        DataSlotComponent();
        DataSlotComponent(const AZ::Uuid& slotTypeId, const SlotConfiguration& slotConfiguration);
        ~DataSlotComponent();
        
        // Component
        void Init();
        void Activate();
        void Deactivate();
        ////

        // NodeNotificationBus
        using NodeNotificationBus::Handler::OnNameChanged;
        void OnNodeAboutToSerialize(GraphSerialization& sceneSerialization) override;
        void OnNodeDeserialized(const AZ::EntityId& graphId, const GraphSerialization& sceneSerialization) override;
        ////

        // SlotRequestBus
        void DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        void AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        void SetNode(const AZ::EntityId& nodeId) override;
        ////

        // DataSlotRequestBus
        bool AssignVariable(const AZ::EntityId& variableId) override;

        AZ::EntityId GetVariableId() const override;

        bool ConvertToReference() override;
        bool CanConvertToReference() const override;

        bool ConvertToValue() override;
        bool CanConvertToValue() const override;

        DataSlotType GetDataSlotType() const override;

        const AZ::Uuid& GetDataTypeId() const override;
        QColor GetDataColor() const override;
        ////

    protected:
        DataSlotComponent(DataSlotType dataSlotType, const AZ::Uuid& slotTypeId, const SlotConfiguration& slotConfiguration);

        void UpdateDisplay();
        void RestoreDisplay(bool updateDisplay = false);

        // SlotComponent
        void OnFinalizeDisplay() override;
        ////

    private:
        DataSlotComponent(const DataSlotComponent&) = delete;
        DataSlotComponent& operator=(const DataSlotComponent&) = delete;
        AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection) const override;

        bool            m_fixedType;
        DataSlotType    m_dataSlotType;
        AZ::Uuid        m_dataTypeId;

        AZ::EntityId    m_variableId;
        AZ::u64         m_copiedVariableId;

        AZ::EntityId    m_displayedConnection;

        // Cached information for the display
        DataSlotType    m_previousDataSlotType;
        AZ::EntityId    m_cachedVariableId;
    };
}