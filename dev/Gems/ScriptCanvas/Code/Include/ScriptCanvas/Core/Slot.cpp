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

#include "precompiled.h"

#include "Slot.h"
#include "Graph.h"
#include "Node.h"
#include "Contracts.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace ScriptCanvas
{
    static bool SlotVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 6)
        {
            auto slotNameElements = AZ::Utils::FindDescendantElements(context, classElement, AZStd::vector<AZ::Crc32>{AZ_CRC("id", 0xbf396750), AZ_CRC("m_name", 0xc08c4427)});
            AZStd::string slotName;
            if (slotNameElements.empty() || !slotNameElements.front()->GetData(slotName))
            {
                return false;
            }

            classElement.AddElementWithData(context, "slotName", slotName);
        }

        if (classElement.GetVersion() <= 8)
        {
            classElement.RemoveElementByName(AZ_CRC("index", 0x80736701));
        }
        return true;
    }

    Slot::Slot(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs)
        : m_name(name)
        , m_toolTip(toolTip)
        , m_type(type)
        , m_id(AZ::Uuid::CreateRandom())
    {
        // Add the slot type contract by default, It is used for filtering input/output slots and flow/data slots
        m_contracts.emplace_back(AZStd::make_unique<SlotTypeContract>());
        for (const auto contractDesc : contractDescs)
        {
            AddContract(contractDesc);
        }
    }

    Slot::Slot(const Slot& other)
        : m_name(other.m_name)
        , m_toolTip(other.m_toolTip)
        , m_type(other.m_type)
        , m_id(other.m_id)
        , m_nodeId(other.m_nodeId)
    {
        for (auto& otherContract : other.m_contracts)
        {
            m_contracts.emplace_back(AZ::EntityUtils::GetApplicationSerializeContext()->CloneObject(otherContract.get()));
        }
    }

    Slot::Slot(Slot&& slot)
        : m_name(AZStd::move(slot.m_name))
        , m_toolTip(AZStd::move(slot.m_toolTip))
        , m_type(AZStd::move(slot.m_type))
        , m_id(AZStd::move(slot.m_id))
        , m_nodeId(AZStd::move(slot.m_nodeId))
        , m_contracts(AZStd::move(slot.m_contracts))
    {
    }

    Slot& Slot::operator=(const Slot& slot)
    {
        m_name = slot.m_name;
        m_toolTip = slot.m_toolTip;
        m_type = slot.m_type;
        m_id = slot.m_id;
        m_nodeId = slot.m_nodeId;
        for (auto& otherContract : slot.m_contracts)
        {
            m_contracts.emplace_back(AZ::EntityUtils::GetApplicationSerializeContext()->CloneObject(otherContract.get()));
        }
        return *this;
    }


    void Slot::AddContract(const ContractDescriptor& contractDesc)
    {
        if (contractDesc.m_createFunc)
        {
            Contract* newContract = contractDesc.m_createFunc();
            if (newContract)
            {
                m_contracts.emplace_back(newContract);
            }
        }
    }

    Data::Type Slot::GetDataType() const
    {
        Data::Type retVal = Data::Type::Invalid();

        NodeRequestBus::EventResult(retVal, m_nodeId, &NodeRequests::GetSlotDataType, m_id);

        return retVal;
    }

    void Slot::Reflect(AZ::ReflectContext* reflection)
    {
        SlotId::Reflect(reflection);
        Contract::Reflect(reflection);
        TypeContract::Reflect(reflection);
        DynamicTypeContract::Reflect(reflection);
        SlotTypeContract::Reflect(reflection);
        ConnectionLimitContract::Reflect(reflection);
        DisallowReentrantExecutionContract::Reflect(reflection);
        ContractRTTI::Reflect(reflection);
        IsReferenceTypeContract::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Slot>()
                ->Version(9, &SlotVersionConverter)
                ->Field("id", &Slot::m_id)
                ->Field("nodeId", &Slot::m_nodeId)
                ->Field("type", &Slot::m_type)
                ->Field("contracts", &Slot::m_contracts)
                ->Field("slotName", &Slot::m_name)
                ->Field("toolTip", &Slot::m_toolTip)
                ;
        }

    }
}