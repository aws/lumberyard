/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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

#include <QColor>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class DataSlotConnectionPin;

    enum class DataSlotType
    {
        Unknown,

        // These are options that can be used on most DataSlots
        Value,
        Reference,

        // This is a special value that is used as a 'source' variable.
        // Rather then a set to a variable.
        Variable,

        // Container types
        Container
    };

    struct DataSlotConfiguration
        : public SlotConfiguration
    {
    public:
        AZ_RTTI(DataSlotConfiguration, "{76933814-A77A-4877-B72D-5DB0F541EDA5}", SlotConfiguration);
        AZ_CLASS_ALLOCATOR(DataSlotConfiguration, AZ::SystemAllocator, 0);

        DataSlotConfiguration() = default;

        DataSlotConfiguration(const SlotConfiguration& slotConfiguration)
            : SlotConfiguration(slotConfiguration)
        {
        }

        DataSlotType            m_dataSlotType;

        AZ::Uuid                m_typeId;
        AZStd::vector<AZ::Uuid> m_containerTypeIds;
    };

    class DataSlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool AssignVariable(const AZ::EntityId& variableSourceId) = 0;
        virtual AZ::EntityId GetVariableId() const = 0;

        virtual bool ConvertToReference() = 0;
        virtual bool CanConvertToReference() const = 0;

        virtual bool ConvertToValue() = 0;
        virtual bool CanConvertToValue() const = 0;

        virtual DataSlotType GetDataSlotType() const = 0;

        virtual AZ::Uuid GetDataTypeId() const = 0;
        virtual void SetDataTypeId(AZ::Uuid typeId) = 0;

        virtual const Styling::StyleHelper* GetDataColorPalette() const = 0;

        virtual size_t GetContainedTypesCount() const = 0;
        virtual AZ::Uuid GetContainedTypeId(size_t index) const = 0;
        virtual const Styling::StyleHelper* GetContainedTypeColorPalette(size_t index) const = 0;

        virtual void SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds = AZStd::vector<AZ::Uuid>()) = 0;
    };

    using DataSlotRequestBus = AZ::EBus<DataSlotRequests>;
    
    class DataSlotNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnVariableAssigned(const AZ::EntityId& variableId) { };
        virtual void OnDataSlotTypeChanged(const DataSlotType& dataSlotType) { };
        virtual void OnDisplayTypeChanged(const AZ::Uuid& dataType, const AZStd::vector<AZ::Uuid>& typeIds) {};
    };
    
    using DataSlotNotificationBus = AZ::EBus<DataSlotNotifications>;

    class DataSlotLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        // BusId here is the specific slot that we want to make requests to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual const DataSlotConnectionPin* GetConnectionPin() const = 0;

        virtual void UpdateDisplay() = 0;
    };

    using DataSlotLayoutRequestBus = AZ::EBus<DataSlotLayoutRequests>;

    //! Actions that are keyed off of the Node, but should be handled by the individual slots
    class NodeDataSlotRequests : public AZ::EBusTraits
    {
    public:
        // The id here is the Node that the slot belongs to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZ::EntityId;

        //! Signals that the slots should try and recreate all of the slot property displays.
        virtual void RecreatePropertyDisplay() = 0;
    };

    using NodeDataSlotRequestBus = AZ::EBus<NodeDataSlotRequests>;

}