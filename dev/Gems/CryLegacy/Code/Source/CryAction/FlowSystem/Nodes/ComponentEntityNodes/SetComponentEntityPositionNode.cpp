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

#include "CryLegacy_precompiled.h"
#include "SetComponentEntityPositionNode.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>


namespace
{
    /**
    * Sets the position of the indicated component entity to the indicated one
    * @param entityId The Id of the entity to be moved
    * @param newPosition The Position that the entity is to be moved to
    */
    void SetEntityPosition(FlowEntityId entityId, Vec3 newPosition, ComponentEntitySetPositionNode::CoordSys coordinateSystem)
    {
        AZ::Transform currentTransform(AZ::Transform::Identity());
        switch (coordinateSystem)
        {
        case ComponentEntitySetPositionNode::CoordSys::World:
        {
            EBUS_EVENT_ID_RESULT(currentTransform, entityId, AZ::TransformBus, GetWorldTM);
            currentTransform.SetTranslation(LYVec3ToAZVec3(newPosition));
            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetWorldTM, currentTransform);
            break;
        }
        case ComponentEntitySetPositionNode::CoordSys::Parent:
        {
            EBUS_EVENT_ID_RESULT(currentTransform, entityId, AZ::TransformBus, GetLocalTM);
            currentTransform.SetTranslation(LYVec3ToAZVec3(newPosition));
            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetLocalTM, currentTransform);
            break;
        }
        }
    }
}

void ComponentEntitySetPositionNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<Vec3>("NewPosition", _HELP("New position for the Entity")),
        InputPortConfig<int>("CoordSystem", 1, _HELP("Indicates the coordinate system the Position is to be set in : Parent = 0, World = 1"),
            _HELP("Coordinate System"), _UICONFIG("enum_int:Parent=0,World=1")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        { 0 }
    };

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.nFlags |= EFLN_ACTIVATION_INPUT | EFLN_TARGET_ENTITY;
    config.SetCategory(EFLN_APPROVED);
}

void ComponentEntitySetPositionNode::ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    switch (evt)
    {
    case eFE_Activate:
    {
        if (IsPortActive(activationInformation, InputPorts::Activate))
        {
            auto coordinateSystem = static_cast<CoordSys>(GetPortInt(activationInformation, InputPorts::CoordSystem));

            if (flowEntityType == FlowEntityType::Component)
            {
                Vec3 newPosition = GetPortVec3(activationInformation, InputPorts::NewPosition);
                SetEntityPosition(activationInformation->entityId, newPosition, coordinateSystem);
            }
        }
        break;
    }
    }
}

REGISTER_FLOW_NODE("ComponentEntity:TransformComponent:SetEntityPosition", ComponentEntitySetPositionNode);

