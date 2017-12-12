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
        DataOut
    };

    class Slot
    {
    public:
        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator, 0);
        AZ_RTTI(Slot, "{FBFE0F02-4C26-475F-A28B-18D3A533C13C}");

        Slot() = default;

        Slot(const Slot& slot);

        Slot(Slot&& slot);
        
        Slot(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, int index, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{});
        
        Slot(AZStd::string_view name, AZStd::string_view toolTip, int index, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{})
            : Slot(name, toolTip, SlotType::None, index, contractDescs)
        {}

        Slot& operator=(const Slot& slot);

        virtual ~Slot() = default;

        void AddContract(const ContractDescriptor& contractDesc);

        AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() { return m_contracts; }
        const AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() const { return m_contracts; }

        const SlotType& GetType() const { return m_type; }        
        const SlotId& GetId() const { return m_id; }
        const int GetIndex() const { return m_index; }

        const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        void SetNodeId(const AZ::EntityId& nodeId) { m_nodeId = nodeId; }

        const AZStd::string& GetName() const { return m_name; }
        const AZStd::string& GetToolTip() const { return m_toolTip; }

        Data::Type GetDataType() const;
        
        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZStd::string m_name;
        AZStd::string m_toolTip;
        SlotType m_type{ SlotType::None };
        SlotId m_id;
        AZ::EntityId m_nodeId;
        int m_index;

        AZStd::vector<AZStd::unique_ptr<Contract>> m_contracts;
    };
}