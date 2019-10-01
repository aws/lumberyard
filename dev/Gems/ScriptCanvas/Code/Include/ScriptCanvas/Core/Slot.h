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
#include <AzCore/std/string/string_view.h>

#include <ScriptCanvas/Core/Contracts/SlotTypeContract.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class Contract;
    class Node;
    
    class Slot final
    {
        friend class Node;
    public:
        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Slot, "{FBFE0F02-4C26-475F-A28B-18D3A533C13C}");

        static void Reflect(AZ::ReflectContext* reflection);

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

        const SlotDescriptor& GetDescriptor() const { return m_descriptor; }
        const SlotId& GetId() const { return m_id; }
        const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        Endpoint GetEndpoint() const { return Endpoint(GetNodeId(), GetId()); }

        void SetNodeId(const AZ::EntityId& nodeId) { m_nodeId = nodeId; }

        const AZStd::string& GetName() const { return m_name; }
        const AZStd::string& GetToolTip() const { return m_toolTip; }

        Data::Type GetDataType() const;

        bool IsData() const;
        bool IsExecution() const;

        bool IsInput() const;
        bool IsOutput() const;
        bool IsLatent() const;        

        // Here to allow conversion of the previously untyped any slots into the dynamic type any.
        void SetDynamicDataType(DynamicDataType dynamicDataType);
        ////

        const DynamicDataType& GetDynamicDataType() const { return m_dynamicDataType; }
        bool IsDynamicSlot() const;

        void SetDisplayType(Data::Type displayType);
        void ClearDisplayType();
        Data::Type GetDisplayType() const;
        bool HasDisplayType() const;

        AZ::Crc32 GetDisplayGroup() const;

        // Should only be used for updating slots. And never really done at runtime as slots
        // won't be re-arranged.
        void SetDisplayGroup(AZStd::string displayGroup);

        AZ::Crc32 GetDynamicGroup() const;

        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Slot& slot) const;
        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Data::Type& dataType) const;

        void Rename(AZStd::string_view slotName);        

    protected:

        void SetDynamicGroup(const AZ::Crc32& dynamicGroup);

        AZStd::string m_name;
        AZStd::string m_toolTip;
        AZ::Crc32 m_displayGroup;
        AZ::Crc32 m_dynamicGroup;

        bool               m_isLatentSlot;
        SlotDescriptor     m_descriptor;

        DynamicDataType m_dynamicDataType{ DynamicDataType::None };
        ScriptCanvas::Data::Type m_displayDataType{ ScriptCanvas::Data::Type::Invalid() };

        SlotId m_id;
        AZ::EntityId m_nodeId;

        AZStd::vector<AZStd::unique_ptr<Contract>> m_contracts;
    };
} // namespace ScriptCanvas
