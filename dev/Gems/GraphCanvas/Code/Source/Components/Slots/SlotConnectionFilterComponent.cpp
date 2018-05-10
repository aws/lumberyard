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
#include <precompiled.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <Components/Slots/SlotConnectionFilterComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/DataConnectionFilters.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    //////////////////////////////////
    // SlotConnectionFilterComponent
    //////////////////////////////////

    void SlotConnectionFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ConnectionFilter>()
            ->Version(1)
        ;

        serializeContext->Class<SlotTypeFilter, ConnectionFilter>()
            ->Version(2)
            ->Field("FilterType", &SlotTypeFilter::m_filterType)
            ->Field("Types", &SlotTypeFilter::m_slotTypes)
            ;

        serializeContext->Class<ConnectionTypeFilter, ConnectionFilter>()
            ->Version(1)
            ->Field("FilterType", &ConnectionTypeFilter::m_filterType)
            ->Field("Types", &ConnectionTypeFilter::m_connectionTypes)
            ;

        serializeContext->Class<SlotConnectionFilterComponent, AZ::Component>()
            ->Version(2)
            ->Field("m_filterSlotGroups", &SlotConnectionFilterComponent::m_filters)
            ;

        serializeContext->Class<DataSlotTypeFilter>()
            ->Version(1)
            ;
    }

    SlotConnectionFilterComponent::SlotConnectionFilterComponent()
    {
    }

    SlotConnectionFilterComponent::~SlotConnectionFilterComponent()
    {
        for (ConnectionFilter* filter : m_filters)
        {
            delete filter;
        }

        m_filters.clear();
    }

    void SlotConnectionFilterComponent::Activate()
    {
        ConnectableObjectRequestBus::Handler::BusConnect(GetEntityId());

        for (ConnectionFilter* filter : m_filters)
        {
            filter->SetEntityId(GetEntityId());
        }
    }

    void SlotConnectionFilterComponent::Deactivate()
    {
        ConnectableObjectRequestBus::Handler::BusDisconnect();
    }

    void SlotConnectionFilterComponent::AddFilter(ConnectionFilter* filter)
    {
        filter->SetEntityId(GetEntityId());
        m_filters.emplace_back(filter);
    }

    Connectability SlotConnectionFilterComponent::CanConnectWith(const AZ::EntityId& slotId) const
    {
        if (GetEntityId() == slotId)
        {
            return{ slotId, Connectability::NotConnectable, "Slot cannot connect to self" };
        }

        AZ::Entity* entity;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, slotId);

        if (!entity)
        {
            return{ GetEntityId(), Connectability::NotConnectable, "Unable to find the other slot's Entity!" };
        }

        AZ::EntityId nodeId;
        SlotRequestBus::EventResult(nodeId, GetEntityId(), &SlotRequests::GetNode);
        Endpoint endpoint(nodeId, GetEntityId());

        bool slotAcceptsConnection = false;
        SlotRequestBus::EventResult(slotAcceptsConnection, slotId, &SlotRequests::CanAcceptConnection, endpoint);

        if (!slotAcceptsConnection)
        {
            return { GetEntityId(), Connectability::AlreadyConnected, AZStd::string::format("Connection already exist between this slot %s and connecting slot %s", GetEntityId().ToString().data(), slotId.ToString().data()) };
        }

        Connectability connectableInfo(GetEntityId(), Connectability::Connectable);

        for (ConnectionFilter* filter : m_filters)
        {
            if (!filter->CanConnectWith(slotId, connectableInfo))
            {
                break;
            }
        }

        return connectableInfo;
    }
}