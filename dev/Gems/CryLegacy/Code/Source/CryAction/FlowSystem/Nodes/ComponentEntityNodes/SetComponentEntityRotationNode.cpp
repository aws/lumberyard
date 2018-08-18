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
#include "SetComponentEntityRotationNode.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>

/*
* Sets the orientation of the indicated component entity to the indicated one
* @param entityId The Id of the entity to be moved
* @param newOrientation The Rotation that is to be set on the entity
*/
void SetEntityRotation(FlowEntityId entityId, Vec3 newOrientation, ComponentEntitySetRotationNode::CoordSys coordinateSystem)
{
    AZ::Transform newRotationTransform(AZ::Transform::CreateRotationX(AZ::DegToRad(newOrientation.x))
        * AZ::Transform::CreateRotationY(AZ::DegToRad(newOrientation.y))
        * AZ::Transform::CreateRotationZ(AZ::DegToRad(newOrientation.z)));

    AZ::Transform currentTransform(AZ::Transform::Identity());
    switch (coordinateSystem)
    {
    case ComponentEntitySetRotationNode::CoordSys::World:
    {
        EBUS_EVENT_ID_RESULT(currentTransform, entityId, AZ::TransformBus, GetWorldTM);
        newRotationTransform.SetTranslation(currentTransform.GetTranslation());
        EBUS_EVENT_ID(entityId, AZ::TransformBus, SetWorldTM, newRotationTransform);
        break;
    }
    case ComponentEntitySetRotationNode::CoordSys::Parent:
    {
        EBUS_EVENT_ID_RESULT(currentTransform, entityId, AZ::TransformBus, GetLocalTM);
        newRotationTransform.SetTranslation(currentTransform.GetTranslation());
        EBUS_EVENT_ID(entityId, AZ::TransformBus, SetLocalTM, newRotationTransform);
        break;
    }
    }
}

void ComponentEntitySetRotationNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<Vec3>("Rotation", _HELP("New Orientation (Euler angles in Degrees) for the Entity")),
        InputPortConfig<int>("CoordSystem", 1, _HELP("Indicates the co-ordinate system the Orientation is to be set in : Parent = 0, World = 1"),
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

void ComponentEntitySetRotationNode::ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation)
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
            if (flowEntityType == FlowEntityType::Component)
            {
                auto coordinateSystem = static_cast<CoordSys>(GetPortInt(activationInformation, InputPorts::CoordSystem));

                Vec3 newRotation = GetPortVec3(activationInformation, InputPorts::Rotation);
                SetEntityRotation(activationInformation->entityId, newRotation, coordinateSystem);
            }
        }
        break;
    }
    }
}

REGISTER_FLOW_NODE("ComponentEntity:TransformComponent:SetEntityRotation", ComponentEntitySetRotationNode);

