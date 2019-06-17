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

#include "Core.h"
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/Contracts/SlotTypeContract.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/EBus/EBus.h>

namespace ScriptCanvas
{
    class Contract;

    enum class SlotType : AZ::s32
    {
        None = 0,

        ExecutionIn,
        ExecutionOut,
        DataIn,
        DataOut,
        LatentOut,
    };

    enum class DynamicDataType : AZ::s32
    {
        None = 0,

        Value,
        Container,
        Any
    };

    struct SlotConfiguration
    {
        AZ_CLASS_ALLOCATOR(SlotConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(SlotConfiguration, "{C169C86A-378F-4263-8B8D-C40D51631ECF}");

        SlotConfiguration()
            : m_slotId(AZ::Uuid::CreateRandom())
        {

        }

        SlotConfiguration(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true, DynamicDataType dataTypeOverride = DynamicDataType::None)
            : m_name(name)
            , m_toolTip(toolTip)
            , m_slotType(type)
            , m_contractDescs(contractDescs)
            , m_addUniqueSlotByNameAndType(addUniqueSlotByNameAndType)
            , m_slotId(AZ::Uuid::CreateRandom())
        {
        }

        AZStd::string m_name;
        AZStd::string m_toolTip;

        SlotType m_slotType = SlotType::None;

        AZStd::vector<ContractDescriptor> m_contractDescs;
        bool m_addUniqueSlotByNameAndType = true; // Only adds a new slot if a slot with the supplied name and SlotType does not exist on the node

        // Specifies the Id the slot will use. Generally necessary only in undo/redo case with dynamically added
        // slots to preserve data integrity
        SlotId m_slotId;

        AZStd::string m_displayGroup;
    };

    struct DataSlotConfiguration
        : public SlotConfiguration
    {
        AZ_CLASS_ALLOCATOR(DataSlotConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(DataSlotConfiguration, "{9411A82E-EB3E-4235-9DDA-12EF6C9ECB1D}", SlotConfiguration);

        DataSlotConfiguration() = default;

        Data::Type m_dataType = Data::Type::Invalid();
        DynamicDataType m_dynamicDataType = DynamicDataType::None;

        Data::Type m_displayType = Data::Type::Invalid();
    };

    //namespace SlotUtils
    //{
    //    bool CanConnect(SlotType a, SlotType b);
    //    bool CanDataConnect(SlotType a, SlotType b);
    //    bool CanExecutionConnect(SlotType a, SlotType b);
    //    bool IsData(SlotType type);
    //    bool IsExecution(SlotType type);
    //    bool IsExecutionOut(SlotType type);
    //    bool IsIn(SlotType type);
    //    bool IsOut(SlotType type);
    //}
    
    class Slot final
    {
    public:
        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Slot, "{FBFE0F02-4C26-475F-A28B-18D3A533C13C}");

        Slot() = default;
        Slot(const Slot& slot);
        Slot(Slot&& slot);

        Slot(const SlotConfiguration& slotConfiguration);

        Slot& operator=(const Slot& slot);

        ~Slot() = default;

        void AddContract(const ContractDescriptor& contractDesc);

        AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() { return m_contracts; }
        const AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() const { return m_contracts; }

        // ConvertToLatentExecutionOut
        //
        // Mainly here to limit scope of what manipulation can be done to the slots. We need to version convert the slots
        // but at a higher tier, so instead of allowing the type to be set, going to just make this specific function which does
        // the conversion we are after.
        void ConvertToLatentExecutionOut();
        ////

        const SlotType& GetType() const { return m_type; }
        const SlotId& GetId() const { return m_id; }

        const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        void SetNodeId(const AZ::EntityId& nodeId) { m_nodeId = nodeId; }

        const AZStd::string& GetName() const { return m_name; }
        const AZStd::string& GetToolTip() const { return m_toolTip; }

        Data::Type GetDataType() const;

        bool IsData() const;
        bool IsExecution() const;

        bool IsInput() const;
        bool IsOutput() const;

        // Here to allow conversion of the previously untyped any slots into the dynamic type any.
        void SetDynamicDataType(DynamicDataType dynamicDataType);
        ////

        const DynamicDataType& GetDynamicDataType() const { return m_dynamicDataType; }
        bool IsDynamicSlot() const;

        void SetDisplayType(Data::Type displayType);
        Data::Type GetDisplayType() const;
        bool HasDisplayType() const;

        AZ::Crc32 GetDisplayGroup() const;

        bool IsTypeMatchFor(const Slot& slot) const;
        bool IsTypeMatchFor(const Data::Type& dataType) const;

        void Rename(AZStd::string_view slotName);
        
        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZStd::string m_name;
        AZStd::string m_toolTip;
        AZ::Crc32 m_displayGroup;

        SlotType m_type{ SlotType::None };
        DynamicDataType m_dynamicDataType{ DynamicDataType::None };
        ScriptCanvas::Data::Type m_displayDataType{ ScriptCanvas::Data::Type::Invalid() };

        SlotId m_id;
        AZ::EntityId m_nodeId;

        AZStd::vector<AZStd::unique_ptr<Contract>> m_contracts;
    };

    namespace SlotUtils
    {
        AZ_INLINE bool IsData(SlotType type)
        {
            return type == SlotType::DataIn || type == SlotType::DataOut;
        }

        AZ_INLINE bool IsExecution(SlotType type)
        {
            return type == SlotType::ExecutionIn
                || type == SlotType::ExecutionOut
                || type == SlotType::LatentOut;
        }

        AZ_INLINE bool IsExecutionOut(SlotType type)
        {
            return type == SlotType::ExecutionOut || type == SlotType::LatentOut;
        }

        AZ_INLINE bool IsInput(SlotType type)
        {
            return type == SlotType::ExecutionIn || type == SlotType::DataIn;
        }

        AZ_INLINE bool IsOutput(SlotType type)
        {
            return type == SlotType::ExecutionOut
                || type == SlotType::DataOut
                || type == SlotType::LatentOut;
        }

        AZ_INLINE bool CanExecutionConnect(SlotType a, SlotType b)
        {
            return (a == SlotType::ExecutionIn && IsExecutionOut(b))
                || (b == SlotType::ExecutionIn && IsExecutionOut(a));
        }

        AZ_INLINE bool CanDataConnect(SlotType a, SlotType b)
        {
            return (a == SlotType::DataIn && b == SlotType::DataOut)
                || (b == SlotType::DataIn && a == SlotType::DataOut);
        }

        AZ_INLINE bool CanConnect(SlotType a, SlotType b)
        {
            return CanDataConnect(a, b) || CanExecutionConnect(a, b);
        }
    }

} // namespace ScriptCanvas
