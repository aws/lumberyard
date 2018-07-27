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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "GetComponentEntityPositionNode.h"

#include <AzCore/Math/Transform.h>
#include <MathConversion.h>


Vec3 GetLocalPosition(FlowEntityId triggerEntityId)
{
    AZ::Transform currentLocalTransform(AZ::Transform::Identity());
    EBUS_EVENT_ID_RESULT(currentLocalTransform, triggerEntityId, AZ::TransformBus, GetLocalTM);
    return AZVec3ToLYVec3(currentLocalTransform.GetPosition());
}

Vec3 GetWorldPosition(FlowEntityId triggerEntityId)
{
    AZ::Transform currentWorldTransform(AZ::Transform::Identity());
    EBUS_EVENT_ID_RESULT(currentWorldTransform, triggerEntityId, AZ::TransformBus, GetWorldTM);
    return AZVec3ToLYVec3(currentWorldTransform.GetPosition());
}

void ComponentEntityPositionNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<int>("CoordSystem", 1, _HELP("Indicates the co-ordinate system the Position is to be fetched in : Parent = 0, World = 1"),
            _HELP("Coordinate System"), _UICONFIG("enum_int:Parent=0,World=1")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        OutputPortConfig<Vec3>("CurrentPosition", _HELP("Current position of the entity")),
        { 0 }
    };

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.nFlags |= EFLN_ACTIVATION_INPUT | EFLN_TARGET_ENTITY;
    config.SetCategory(EFLN_APPROVED);
}

void ComponentEntityPositionNode::ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    switch (evt)
    {
    case eFE_Initialize:
    {
        if (flowEntityType == FlowEntityType::Component)
        {
            ResetHandlerForEntityId(activationInformation->entityId);
        }
        break;
    }
    case eFE_Activate:
    {
        if (IsPortActive(activationInformation, InputPorts::Activate))
        {
            m_coordinateSystem = static_cast<CoordSys>(GetPortInt(activationInformation, InputPorts::CoordSystem));

            if (flowEntityType == FlowEntityType::Component)
            {
                if (m_previousQueryEntityId != activationInformation->entityId)
                {
                    ResetHandlerForEntityId(activationInformation->entityId);
                }

                ActivateOutput(activationInformation, OutputPorts::CurrentPosition, m_cachedEntityPosition);
            }
        }
        break;
    }
    case eFE_Uninitialize:
    {
        ResetHandlerForEntityId(FlowEntityId(FlowEntityId::s_invalidFlowEntityID));
        break;
    }
    }
}

void ComponentEntityPositionNode::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
{
    if (m_coordinateSystem == CoordSys::Parent)
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, m_previousQueryEntityId, AZ::TransformBus, GetParentId);

        if (parentId.IsValid())
        {
            m_cachedEntityPosition = AZVec3ToLYVec3(local.GetPosition());
            return;
        }
    }

    m_cachedEntityPosition = AZVec3ToLYVec3(world.GetPosition());
}

void ComponentEntityPositionNode::ResetHandlerForEntityId(FlowEntityId triggerEntityId)
{
    if (triggerEntityId != FlowEntityId::s_invalidFlowEntityID)
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        m_previousQueryEntityId = triggerEntityId;

        if (m_coordinateSystem == CoordSys::Parent)
        {
            AZ::EntityId parentId;
            EBUS_EVENT_ID_RESULT(parentId, triggerEntityId, AZ::TransformBus, GetParentId);

            if (parentId.IsValid())
            {
                m_cachedEntityPosition = GetLocalPosition(m_previousQueryEntityId);
            }
            else
            {
                m_cachedEntityPosition = GetWorldPosition(m_previousQueryEntityId);
            }
        }
        else
        {
            m_cachedEntityPosition = GetWorldPosition(m_previousQueryEntityId);
        }

        AZ::TransformNotificationBus::Handler::BusConnect(m_previousQueryEntityId);
    }
    else
    {
        m_cachedEntityPosition.zero();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }
}

REGISTER_FLOW_NODE("ComponentEntity:TransformComponent:GetEntityPosition", ComponentEntityPositionNode);