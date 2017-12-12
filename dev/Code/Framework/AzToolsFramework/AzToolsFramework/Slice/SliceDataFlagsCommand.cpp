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
#include "StdAfx.h"
#include "SliceDataFlagsCommand.h"
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    SliceDataFlagsCommand::SliceDataFlagsCommand(
        const AZ::EntityId& entityId,
        const AZ::DataPatch::AddressType& targetDataAddress,
        AZ::DataPatch::Flag dataFlag,
        bool additive,
        const AZStd::string& friendlyName,
        UndoSystem::URCommandID commandId/*=0*/)
        : URSequencePoint(friendlyName, commandId)
        , m_entityId(entityId)
        , m_dataAddress(targetDataAddress)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        AZ_Warning("Undo", sliceInstanceAddress.second, "Cannot find slice instance for entity ID %s", m_entityId.ToString().c_str());
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            m_previousDataFlags = sliceInstance->GetDataFlags().GetEntityDataFlagsAtAddress(m_entityId, m_dataAddress);
        }

        if (additive)
        {
            m_nextDataFlags = m_previousDataFlags | dataFlag;
        }
        else
        {
            m_nextDataFlags = m_previousDataFlags & ~dataFlag;
        }

        Redo();
    }

    void SliceDataFlagsCommand::Undo()
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            sliceInstance->GetDataFlags().SetEntityDataFlagsAtAddress(m_entityId, m_dataAddress, m_previousDataFlags);
        }
    }

    void SliceDataFlagsCommand::Redo()
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            sliceInstance->GetDataFlags().SetEntityDataFlagsAtAddress(m_entityId, m_dataAddress, m_nextDataFlags);
        }
    }

    ClearSliceDataFlagsBelowAddressCommand::ClearSliceDataFlagsBelowAddressCommand(
        const AZ::EntityId& entityId,
        const AZ::DataPatch::AddressType& targetDataAddress,
        const AZStd::string& friendlyName,
        UndoSystem::URCommandID commandId/*=0*/)
        : URSequencePoint(friendlyName, commandId)
        , m_entityId(entityId)
        , m_dataAddress(targetDataAddress)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        AZ_Warning("Undo", sliceInstanceAddress.second, "Cannot find slice instance for entity ID %s", m_entityId.ToString().c_str());
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            m_previousDataFlagsMap = sliceInstance->GetDataFlags().GetEntityDataFlags(m_entityId);

            // m_nextDataFlagsMap is a copy of the map, but without entries at or below m_dataAddress
            for (const auto& addressFlagsPair : m_previousDataFlagsMap)
            {
                const AZ::DataPatch::AddressType& address = addressFlagsPair.first;
                if (m_dataAddress.size() <= address.size())
                {
                    if (AZStd::equal(m_dataAddress.begin(), m_dataAddress.end(), address.begin()))
                    {
                        continue;
                    }
                }

                m_nextDataFlagsMap.emplace(addressFlagsPair);
            }
        }

        Redo();
    }

    void ClearSliceDataFlagsBelowAddressCommand::Undo()
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            sliceInstance->GetDataFlags().SetEntityDataFlags(m_entityId, m_previousDataFlagsMap);
        }
    }

    void ClearSliceDataFlagsBelowAddressCommand::Redo()
    {
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, m_entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
        if (AZ::SliceComponent::SliceInstance* sliceInstance = sliceInstanceAddress.second)
        {
            sliceInstance->GetDataFlags().SetEntityDataFlags(m_entityId, m_nextDataFlagsMap);
        }
    }

} // namespace AzToolsFramework