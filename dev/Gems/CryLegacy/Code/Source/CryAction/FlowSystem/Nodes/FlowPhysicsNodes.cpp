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
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <IEntitySystem.h>
#include <IAISystem.h>
#include <IAgent.h>
#include "IAIObject.h"
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>
#include <MathConversion.h>

#include "Components/IComponentPhysics.h"
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>

//////////////////////////////////////////////////////////////////////////
class CFlowNode_Dynamics
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Enable,
        Disable,
    };

    enum OutputPorts
    {
        Velocity,
        Acceleration,
        AngularVelocity,
        AngularAcceleration,
        Mass,
    };

public:
    CFlowNode_Dynamics(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig_Void("Enable", _HELP("Turn on updates"), _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Turn off updates"), _HELP("Disable")),
            {0}
        };

        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<Vec3>("Velocity", _HELP("Velocity of entity")),
            OutputPortConfig<Vec3>("Acceleration", _HELP("Acceleration of entity")),
            OutputPortConfig<Vec3>("AngularVelocity", _HELP("Angular velocity of entity")),
            OutputPortConfig<Vec3>("AngularAcceleration", _HELP("Angular acceleration of entity")),
            OutputPortConfig<float>("Mass", _HELP("Mass of entity")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Dynamic physical state of an entity");
        config.SetCategory(EFLN_APPROVED); // POLICY CHANGE: Maybe an Enable/Disable Port
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            // Enable updates defaults to true.
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }

            if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            break;
        }
        case eFE_Update:
        {
            // this should only run if SetRegularlyUpdated has been set to true.
            FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);
            if (entityType == FlowEntityType::Invalid)
            {
                return;
            }

            pe_status_dynamics dyn;
            if (entityType == FlowEntityType::Legacy)
            {
                if (IEntity* pEntity = pActInfo->pEntity)
                {
                    if (IPhysicalEntity* pPhysEntity = pEntity->GetPhysics())
                    {
                        pPhysEntity->GetStatus(&dyn);
                    }
                }
            }
            else if (entityType == FlowEntityType::Component)
            {
                EBUS_EVENT_ID(pActInfo->entityId, LmbrCentral::CryPhysicsComponentRequestBus, GetPhysicsStatus, dyn);
            }
            ActivateOutput(pActInfo, OutputPorts::Velocity, dyn.v);
            ActivateOutput(pActInfo, OutputPorts::Acceleration, dyn.a);
            ActivateOutput(pActInfo, OutputPorts::AngularVelocity, dyn.w);
            ActivateOutput(pActInfo, OutputPorts::AngularAcceleration, dyn.wa);
            ActivateOutput(pActInfo, OutputPorts::Mass, dyn.mass);
            break;
        }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_PhysicsSleepQuery
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Condition,
        Reset,
    };

    enum OutputPorts
    {
        Sleeping,
        OnSleep,
        OnAwake,
    };

public:
    CFlowNode_PhysicsSleepQuery(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_PhysicsSleepQuery(pActInfo);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<bool>("Condition", _HELP("Setting [Condition] true sends the sleeping state of the entity to the [Sleeping] output, and activates [OnSleep] or [OnAwake].")),
            InputPortConfig<bool>("Reset", _HELP("Activating [Reset] resets the node.")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<bool>("Sleeping", _HELP("The sleeping state of the entity's physics.")),
            OutputPortConfig<SFlowSystemVoid>("OnSleep", _HELP("Activated when the entity's physics switches to sleep.")),
            OutputPortConfig<SFlowSystemVoid>("OnAwake", _HELP("Activated when the entity's physics switches to awake.")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Returns the sleeping state of the physics of a given entity.");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            m_Activated = false;
            break;
        }

        case eFE_Update:
        {
            bool bReset = GetPortBool(pActInfo, InputPorts::Reset);
            bool bCondition = GetPortBool(pActInfo, InputPorts::Condition);

            if (bReset)
            {
                ActivateOutput(pActInfo, OutputPorts::Sleeping, !bCondition);
            }
            else
            {
                if (bCondition != m_Activated)
                {
                    IEntity* pEntity = pActInfo->pEntity;
                    if (pEntity)
                    {
                        IPhysicalEntity* pPhysEntity = pEntity->GetPhysics();
                        if (pPhysEntity)
                        {
                            pe_status_awake psa;

                            bool isSleeping = pPhysEntity->GetStatus(&psa) ? false : true;

                            ActivateOutput(pActInfo, OutputPorts::Sleeping, isSleeping);

                            if (isSleeping)
                            {
                                ActivateOutput(pActInfo, OutputPorts::OnSleep, true);
                            }
                            else
                            {
                                ActivateOutput(pActInfo, OutputPorts::OnAwake, true);
                            }
                        }
                    }

                    m_Activated = bCondition;
                }
            }
        }
        }
    }

private:
    bool m_Activated;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ActionImpulse
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum CoordinateSystem
    {
        Parent,
        World,
        Local,
    };

    enum InputPorts
    {
        Activate,
        Impulse,
        AngularImpulse,
        Point,
        PartIndex,
        CoordSystem,
    };

    bool DoesEntityHaveParent(AZ::EntityId entityId) const
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, entityId, AZ::TransformBus, GetParentId);
        return parentId.IsValid();
    }

    bool DoesEntityHaveParent(const IEntity& entity) const
    {
        return entity.GetParent() != nullptr;
    }

    Vec3 GetEntityPosition(AZ::EntityId entityId) const
    {
        AZ::Transform currentWorldTransform(AZ::Transform::Identity());
        EBUS_EVENT_ID_RESULT(currentWorldTransform, entityId, AZ::TransformBus, GetWorldTM);
        return AZVec3ToLYVec3(currentWorldTransform.GetPosition());
    }

    Vec3 GetEntityPosition(const IEntity& entity) const
    {
        return entity.GetWorldPos();
    }

public:
    CFlowNode_ActionImpulse(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<Vec3>("Impulse", Vec3(0, 0, 0), _HELP("Impulse vector")),
            InputPortConfig<Vec3>("AngularImpulse", Vec3(0, 0, 0), _HELP("Angular impulse vector")),
            InputPortConfig<Vec3>("Point", Vec3(0, 0, 0), _HELP("Point of application (optional)")),
            InputPortConfig<int>("PartIndex", 0, _HELP("Index of the part that will receive the impulse (optional, 1-based, 0=unspecified)")),
            InputPortConfig<int>("CoordSystem", 1, _HELP("Defines which coordinate system is used for the input vectors"), _HELP("CoordSystem"), _UICONFIG("enum_int:Parent=0,World=1,Local=2")),
            {0}
        };
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.sDescription = _HELP("Applies a physics impulse on an entity.");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        IEntity* attachedEntity = pActInfo->pEntity;
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

        // If there is no attached entity
        if (!attachedEntity)
        {
            // With a legacy or invalid entity id
            if (entityType != FlowEntityType::Component)
            {
                // Do not proceed
                return;
            }
        }

        if (event == eFE_Activate && IsPortActive(pActInfo, InputPorts::Activate))
        {
            pe_action_impulse action;

            int entityPart = GetPortInt(pActInfo, InputPorts::PartIndex);
            if ((entityType == FlowEntityType::Legacy))
            {
                if (entityPart > 0)
                {
                    action.ipart = entityPart - 1;
                }
            }
            else
            {
                action.ipart = entityPart;
            }

            CoordinateSystem coordSys = static_cast<CoordinateSystem>(GetPortInt(pActInfo, InputPorts::CoordSystem));

            bool entityHasParentAttached = false;

            if (coordSys == CoordinateSystem::Parent)
            {
                if (entityType == FlowEntityType::Legacy)
                {
                    entityHasParentAttached = DoesEntityHaveParent(*attachedEntity);
                }
                else if (entityType ==  FlowEntityType::Component)
                {
                    entityHasParentAttached = DoesEntityHaveParent(pActInfo->entityId);
                }

                if (!entityHasParentAttached)
                {
                    coordSys = CoordinateSystem::World;
                }
            }

            // When a "zero point" is set in the node, the value is left undefined and physics assume it is the CM of the object.
            // but when the entity has a parent (is linked), then we have to use a real world coordinate for the point, because we have to apply the impulse to the highest entity
            // on the hierarchy and physics will use the position of that entity instead of the position of the entity assigned to the node
            bool bHaveToUseTransformedZeroPoint = entityHasParentAttached;
            Vec3 transformedZeroPoint;
            Matrix34 transMat;

            switch (coordSys)
            {
            case CoordinateSystem::World:
            default:
            {
                transMat.SetIdentity();

                if (entityType == FlowEntityType::Legacy)
                {
                    transformedZeroPoint = GetEntityPosition(*attachedEntity);
                }
                else if (entityType == FlowEntityType::Component)
                {
                    transformedZeroPoint = GetEntityPosition(pActInfo->entityId);
                }
                break;
            }
            case CoordinateSystem::Parent:
            {
                if (entityType == FlowEntityType::Legacy)
                {
                    IEntity* parentEntity = attachedEntity->GetParent();
                    transMat = parentEntity->GetWorldTM();

                    bHaveToUseTransformedZeroPoint = DoesEntityHaveParent(*parentEntity);

                    transformedZeroPoint = GetEntityPosition(*parentEntity);
                }
                else if (entityType == FlowEntityType::Component)
                {
                    AZ::EntityId parentId;
                    EBUS_EVENT_ID_RESULT(parentId, pActInfo->entityId, AZ::TransformBus, GetParentId);
                    AZ::Transform currentWorldTransform(AZ::Transform::Identity());
                    EBUS_EVENT_ID_RESULT(currentWorldTransform, parentId, AZ::TransformBus, GetWorldTM);
                    transMat = AZTransformToLYTransform(currentWorldTransform);

                    bHaveToUseTransformedZeroPoint = DoesEntityHaveParent(parentId);
                    transformedZeroPoint = GetEntityPosition(parentId);
                }
                break;
            }
            case CoordinateSystem::Local:
            {
                if (entityType == FlowEntityType::Legacy)
                {
                    transMat = attachedEntity->GetWorldTM();
                    transformedZeroPoint = GetEntityPosition(*attachedEntity);
                }
                else if (entityType == FlowEntityType::Component)
                {
                    transformedZeroPoint = GetEntityPosition(pActInfo->entityId);
                }
                break;
            }
            }

            action.impulse = GetPortVec3(pActInfo, InputPorts::Impulse);
            action.impulse = transMat.TransformVector(action.impulse);

            Vec3 angImpulse = GetPortVec3(pActInfo, InputPorts::AngularImpulse);
            if (!angImpulse.IsZero())
            {
                action.angImpulse = transMat.TransformVector(angImpulse);
            }

            Vec3 pointApplication = GetPortVec3(pActInfo, InputPorts::Point);
            if (!pointApplication.IsZero())
            {
                action.point = transMat.TransformPoint(pointApplication);
            }
            else
            {
                if (bHaveToUseTransformedZeroPoint)
                {
                    action.point = transformedZeroPoint;
                }
            }

            // the impulse has to be applied to the highest entity in the hierarchy. This comes from how physics manage linked entities.
            if (entityType == FlowEntityType::Legacy)
            {
                IEntity* pEntityImpulse = attachedEntity;
                while (pEntityImpulse->GetParent())
                {
                    pEntityImpulse = pEntityImpulse->GetParent();
                }

                IPhysicalEntity* pPhysEntity = pEntityImpulse->GetPhysics();
                if (pPhysEntity)
                {
                    pPhysEntity->Action(&action);
                }
            }
            else if (entityType == FlowEntityType::Component)
            {
                if (entityHasParentAttached)
                {
                    AZ::EntityId parentId;
                    EBUS_EVENT_ID_RESULT(parentId, pActInfo->entityId, AZ::TransformBus, GetParentId);

                    AZ::EntityId topLevelEntityId;
                    do
                    {
                        topLevelEntityId.SetInvalid();
                        EBUS_EVENT_ID_RESULT(topLevelEntityId, parentId, AZ::TransformBus, GetParentId);
                        parentId = topLevelEntityId.IsValid() ? topLevelEntityId : parentId;
                    }
                    while (topLevelEntityId.IsValid());

                    EBUS_EVENT_ID(topLevelEntityId, LmbrCentral::CryPhysicsComponentRequestBus, ApplyPhysicsAction, action, true);
                }
                else
                {
                    EBUS_EVENT_ID(pActInfo->entityId, LmbrCentral::CryPhysicsComponentRequestBus, ApplyPhysicsAction, action, true);
                }
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_Raycast
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        Direction,
        MaxLength,
        Position,
        TransformDirection,
    };

    enum OutputPorts
    {
        NoHit,
        Hit,
        RayDirection,
        HitDistance,
        HitPoint,
        HitNormal,
        HitSurfaceType,
        HitEntity,
    };

public:
    CFlowNode_Raycast(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<Vec3>("Direction", Vec3(0, 1, 0), _HELP("Direction of Raycast")),
            InputPortConfig<float>("MaxLength", 10.0f, _HELP("Maximum length of Raycast")),
            InputPortConfig<Vec3>("Position", Vec3(0, 0, 0), _HELP("Ray start position, relative to entity")),
            InputPortConfig<bool>("TransformDirection", true, _HELP("Whether direction is transformed by entity orientation.")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<SFlowSystemVoid>("NoHit", _HELP("Activated if NO object was hit by the raycast")),
            OutputPortConfig<SFlowSystemVoid>("Hit", _HELP("Activated if an object was hit by the raycast")),
            OutputPortConfig<Vec3>("RayDirection", _HELP("Actual direction of the cast ray (possibly transformed by entity rotation, assumes [Hit])")),
            OutputPortConfig<float>("HitDistance", _HELP("Distance to the hit object (assumes [Hit])")),
            OutputPortConfig<Vec3>("HitPoint", _HELP("Position of the hit (assumes [Hit])")),
            OutputPortConfig<Vec3>("HitNormal", _HELP("Normal of the surface at the [HitPoint] (assumes [Hit])")),
            OutputPortConfig<int>("HitSurfaceType", _HELP("Surface type index of the surface hit (assumes [Hit])")),
            OutputPortConfig<FlowEntityId>("HitEntity", _HELP("Entity Id of the entity that was hit (assumes [Hit])")),
            {0}
        };
        config.sDescription = _HELP("Perform a raycast relative to an entity.");
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, InputPorts::Activate))
        {
            IEntity* pEntity = pActInfo->pEntity;
            if (pEntity)
            {
                ray_hit hit;
                IPhysicalEntity* pSkip = pEntity->GetPhysics();
                Vec3 direction = GetPortVec3(pActInfo, InputPorts::Direction).GetNormalized();
                if (GetPortBool(pActInfo, InputPorts::TransformDirection))
                {
                    direction = pEntity->GetWorldTM().TransformVector(GetPortVec3(pActInfo, InputPorts::Direction).GetNormalized());
                }
                IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
                int numHits = pWorld->RayWorldIntersection(
                        pEntity->GetPos() + GetPortVec3(pActInfo, InputPorts::Position),
                        direction * GetPortFloat(pActInfo, InputPorts::MaxLength),
                        ent_all,
                        rwi_stop_at_pierceable | rwi_colltype_any,
                        &hit, 1,
                        &pSkip, 1);

                if (numHits)
                {
                    pEntity = (IEntity*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
                    ActivateOutput(pActInfo, OutputPorts::Hit, true);
                    ActivateOutput(pActInfo, OutputPorts::RayDirection, direction);
                    ActivateOutput(pActInfo, OutputPorts::HitDistance, hit.dist);
                    ActivateOutput(pActInfo, OutputPorts::HitPoint, hit.pt);
                    ActivateOutput(pActInfo, OutputPorts::HitNormal, hit.n);
                    ActivateOutput(pActInfo, OutputPorts::HitSurfaceType, (int)hit.surface_idx);
                    ActivateOutput(pActInfo, OutputPorts::HitEntity, pEntity ? pEntity->GetId() : 0);
                }
                else
                {
                    // lumberyard - shouldn't this be set to true?
                    ActivateOutput(pActInfo, OutputPorts::NoHit, false);
                }
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_RaycastCamera
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        PositionOffset,
        MaxLength,
    };

    enum OutputPorts
    {
        NoHit,
        Hit,
        RayDirection,
        HitDistance,
        HitPoint,
        HitNormal,
        HitSurfaceType,
        PARTID,
        HitEntity,
        HIT_ENTITY_PHID,
    };

public:
    CFlowNode_RaycastCamera(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<Vec3>("PositionOffset", Vec3(0, 0, 0), _HELP("Ray start position, relative to camera")),
            InputPortConfig<float>("MaxLength", 10.0f, _HELP("Maximum length of Raycast")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<SFlowSystemVoid>("NoHit", _HELP("Activated if NO object was hit by the raycast")),
            OutputPortConfig<SFlowSystemVoid>("Hit", _HELP("Activated if an object was hit by the raycast")),
            OutputPortConfig<Vec3>("RayDirection", _HELP("Actual direction of the cast ray (possibly transformed by entity rotation, assumes [Hit])")),
            OutputPortConfig<float>("HitDistance", _HELP("Distance to the hit object (assumes [Hit])")),
            OutputPortConfig<Vec3>("HitPoint", _HELP("Position of the hit (assumes [Hit])")),
            OutputPortConfig<Vec3>("HitNormal", _HELP("Normal of the surface at the [HitPoint] (assumes [Hit])")),
            OutputPortConfig<int>("HitSurfaceType", _HELP("Surface type index of the surface hit (assumes [Hit])")),
            OutputPortConfig<int>("partid", _HELP("Hit part id")),
            OutputPortConfig<FlowEntityId>("HitEntity", _HELP("Entity Id of the entity that was hit (assumes [Hit])")),
            OutputPortConfig<FlowEntityId> ("entityPhysId", _HELP("Id of the physical entity that was hit")),
            {0}
        };
        config.sDescription = _HELP("Perform a raycast relative to the camera.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, InputPorts::Activate))
        {
            IEntity* pEntity = pActInfo->pEntity;
            // if (pEntity)
            {
                ray_hit hit;
                CCamera& cam = GetISystem()->GetViewCamera();
                Vec3 pos = cam.GetPosition() + cam.GetViewdir();
                Vec3 direction = cam.GetViewdir();
                IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
                //IPhysicalEntity* pSkip = nullptr; // pEntity->GetPhysics();
                int numHits = pWorld->RayWorldIntersection(
                        pos + GetPortVec3(pActInfo, InputPorts::PositionOffset),
                        direction * GetPortFloat(pActInfo, InputPorts::MaxLength),
                        ent_all,
                        rwi_stop_at_pierceable | rwi_colltype_any,
                        &hit, 1
                        /* ,&pSkip, 1 */
                        );

                if (numHits)
                {
                    pEntity = (IEntity*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
                    ActivateOutput(pActInfo, OutputPorts::Hit, true);
                    ActivateOutput(pActInfo, OutputPorts::RayDirection, direction);
                    ActivateOutput(pActInfo, OutputPorts::HitDistance, hit.dist);
                    ActivateOutput(pActInfo, OutputPorts::HitPoint, hit.pt);
                    ActivateOutput(pActInfo, OutputPorts::HitNormal, hit.n);
                    ActivateOutput(pActInfo, OutputPorts::HitSurfaceType, (int)hit.surface_idx);
                    ActivateOutput(pActInfo, PARTID, hit.partid);
                    ActivateOutput(pActInfo, OutputPorts::HitEntity, pEntity ? pEntity->GetId() : 0);
                    ActivateOutput(pActInfo, HIT_ENTITY_PHID, gEnv->pPhysicalWorld->GetPhysicalEntityId(hit.pCollider));
                }
                else
                {
                    // lumberyard - shouldn't this be set to true?
                    ActivateOutput(pActInfo, OutputPorts::NoHit, false);
                }
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
// Enable/Disable Physics/AI for an entity
//
// Lumberyard Note: This seems redundant to have distinct Enable/Disable
// ports, and why are there AI and Physics enablers in the same node?
// Could potentially have one input, which is bool, that activates the
// enabled state directly.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_PhysicsEnable
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        EnablePhysics,
        DisablePhysics,
        EnableAI,
        DisableAI,
    };

public:
    CFlowNode_PhysicsEnable(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig_Void("EnablePhysics", _HELP("Enable Physics for target entity")),
            InputPortConfig_Void("DisablePhysics", _HELP("Disable Physics for target entity")),
            InputPortConfig_Void("EnableAI", _HELP("Enable AI for target entity")),
            InputPortConfig_Void("DisableAI", _HELP("Disable AI for target entity")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {   // None!
            { 0 }
        };

        config.sDescription = _HELP("Enables/Disables Physics/AI on an Entity.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!pActInfo->pEntity)
            {
                return;
            }

            if (IsPortActive(pActInfo, InputPorts::EnablePhysics))
            {
                IComponentPhysicsPtr pPhysicsComponent = pActInfo->pEntity->GetComponent<IComponentPhysics>();
                if (pPhysicsComponent)
                {
                    pPhysicsComponent->EnablePhysics(true);
                }
            }
            if (IsPortActive(pActInfo, InputPorts::DisablePhysics))
            {
                IComponentPhysicsPtr pPhysicsComponent = pActInfo->pEntity->GetComponent<IComponentPhysics>();
                if (pPhysicsComponent)
                {
                    pPhysicsComponent->EnablePhysics(false);
                }
            }

            if (IsPortActive(pActInfo, InputPorts::EnableAI))
            {
                if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
                {
                    aiObject->Event(AIEVENT_ENABLE, 0);
                }
            }
            if (IsPortActive(pActInfo, InputPorts::DisableAI))
            {
                if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
                {
                    aiObject->Event(AIEVENT_DISABLE, 0);
                }
            }
            break;
        }

        case eFE_Initialize:
        {
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


class CFlowNode_CameraProxy
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_CREATE,
        IN_ID
    };
    enum EOutputs
    {
        OUT_ID
    };
    CFlowNode_CameraProxy(SActivationInfo* pActInfo) {};
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Create", SFlowSystemVoid(), _HELP("Create the proxy if it doesnt exist yet")),
            InputPortConfig<FlowEntityId>("EntityHost", 0, _HELP("Activate to sync proxy rotation with the current view camera")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("EntityCamera", _HELP("")),
            {0}
        };
        config.sDescription = _HELP("Retrieves or creates a physicalized camera proxy attached to EntityHost");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }
    IEntity* GetCameraEnt(IEntity* pHost, bool bCreate)
    {
        if (!pHost->GetPhysics())
        {
            return bCreate ? pHost : 0;
        }
        pe_params_articulated_body pab;
        IEntityLink* plink = pHost->GetEntityLinks();
        for (; plink && strcmp(plink->name, "CameraProxy"); plink = plink->next)
        {
            ;
        }
        if (plink)
        {
            IEntity* pCam = gEnv->pEntitySystem->GetEntity(plink->entityId);
            if (!pCam)
            {
                pHost->RemoveEntityLink(plink);
                return bCreate ? pHost : 0;
            }
            pab.qHostPivot = !pHost->GetRotation() * Quat(Matrix33(GetISystem()->GetViewCamera().GetMatrix()));
            pCam->GetPhysics()->SetParams(&pab);
            return pCam;
        }
        else if (bCreate)
        {
            CCamera& cam = GetISystem()->GetViewCamera();
            Quat qcam = Quat(Matrix33(cam.GetMatrix()));
            SEntitySpawnParams esp;
            esp.sName = "CameraProxy";
            esp.nFlags = 0;
            esp.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
            IEntity* pCam = gEnv->pEntitySystem->SpawnEntity(esp);
            pCam->SetPos(cam.GetPosition());
            pCam->SetRotation(qcam);
            pHost->AddEntityLink("CameraProxy", pCam->GetId());
            SEntityPhysicalizeParams epp;
            epp.type = PE_ARTICULATED;
            pCam->Physicalize(epp);
            pe_geomparams gp;
            gp.flags = 0;
            gp.flagsCollider = 0;
            primitives::sphere sph;
            sph.r = 0.1f;
            sph.center.zero();
            IGeometry* psph = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &sph);
            phys_geometry* pGeom = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(psph);
            psph->Release();
            pCam->GetPhysics()->AddGeometry(pGeom, &gp);
            pGeom->nRefCount--;
            pe_params_pos pp;
            pp.iSimClass = 2;
            pCam->GetPhysics()->SetParams(&pp);
            pab.pHost = pHost->GetPhysics();
            pab.posHostPivot = (cam.GetPosition() - pHost->GetPos()) * pHost->GetRotation();
            pab.qHostPivot = !pHost->GetRotation() * qcam;
            pCam->GetPhysics()->SetParams(&pab);
            return pCam;
        }
        return 0;
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            bool bCreate = IsPortActive(pActInfo, IN_CREATE);
            if (IEntity* pHost = gEnv->pEntitySystem->GetEntity(GetPortEntityId(pActInfo, IN_ID)))
            {
                if (IEntity* pCam = GetCameraEnt(pHost, bCreate))
                {
                    if (bCreate)
                    {
                        ActivateOutput(pActInfo, OUT_ID, pCam->GetId());
                    }
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowNode_Constraint
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_CREATE,
        IN_BREAK,
        IN_ID,
        IN_ENTITY0,
        IN_PARTID0,
        IN_ENTITY1,
        IN_PARTID1,
        IN_POINT,
        IN_IGNORE_COLLISIONS,
        IN_BREAKABLE,
        IN_FORCE_AWAKE,
        IN_MAX_FORCE,
        IN_MAX_TORQUE,
        IN_MAX_FORCE_REL,
        IN_AXIS,
        IN_MIN_ROT,
        IN_MAX_ROT,
        IN_MAX_BEND,
    };
    enum EOutputs
    {
        OUT_ID,
        OUT_BROKEN,
    };
    CFlowNode_Constraint(SActivationInfo* pActInfo) {};
    ~CFlowNode_Constraint() {}

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_Constraint(pActInfo);
    }

    struct SConstraintRec
    {
        SConstraintRec()
            : next(0)
            , prev(0)
            , pent(0)
            , pNode(0)
            , idConstraint(-1)
            , bBroken(0)
            , minEnergy(0) {}
        ~SConstraintRec() { Free(); }
        void Free()
        {
            if (idConstraint >= 0)
            {
                if (minEnergy > 0)
                {
                    pe_simulation_params sp;
                    sp.minEnergy = minEnergy;
                    pe_params_articulated_body pab;
                    pab.minEnergyLyingMode = minEnergyRagdoll;
                    pent->SetParams(&sp);
                    pent->SetParams(&pab);
                }
                if (pent)
                {
                    pent->Release();
                }
                idConstraint = -1;
                pent = 0;
                (prev ? prev->next : g_pConstrRec0) = next;
                if (next)
                {
                    next->prev = prev;
                }
            }
        }
        SConstraintRec* next, * prev;
        IPhysicalEntity* pent;
        CFlowNode_Constraint* pNode;
        int idConstraint;
        int bBroken;
        float minEnergy, minEnergyRagdoll;
    };
    static SConstraintRec* g_pConstrRec0;

    static int OnConstraintBroken(const EventPhysJointBroken* ejb)
    {
        for (SConstraintRec* pRec = g_pConstrRec0; pRec; pRec = pRec->next)
        {
            if (pRec->pent == ejb->pEntity[0] && pRec->idConstraint == ejb->idJoint)
            {
                pRec->bBroken = 1;
            }
        }
        return 1;
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Create", SFlowSystemVoid(), _HELP("Creates the constraint")),
            InputPortConfig<SFlowSystemVoid>("Break", SFlowSystemVoid(), _HELP("Breaks a constraint Id from EntityA if EntityA is activated, or a previously created one")),
            InputPortConfig<int>("Id", 1000, _HELP("Requested constraint Id; -1 to auto-generate")),
            InputPortConfig<FlowEntityId>("EntityA", 0, _HELP("Constraint owner entity")),
            InputPortConfig<int>("PartIdA", -1, _HELP("Part id to attach to; -1 to use default")),
            InputPortConfig<FlowEntityId>("EntityB", 0, _HELP("Constraint 'buddy' entity")),
            InputPortConfig<int>("PartIdB", -1, _HELP("Part id to attach to; -1 to use default")),
            InputPortConfig<Vec3>("Point", Vec3(ZERO), _HELP("Connection point in world space")),
            InputPortConfig<bool>("IgnoreCollisions", true, _HELP("Disables collisions between constrained entities")),
            InputPortConfig<bool>("Breakable", false, _HELP("Break if force limit is reached")),
            InputPortConfig<bool>("ForceAwake", false, _HELP("Make EntityB always awake; restore previous sleep params on Break")),
            InputPortConfig<float>("MaxForce", 0.0f, _HELP("Force limit")),
            InputPortConfig<float>("MaxTorque", 0.0f, _HELP("Rotational force (torque) limit")),
            InputPortConfig<bool>("MaxForceRelative", true, _HELP("Make limits relative to EntityB's mass")),
            InputPortConfig<Vec3>("TwistAxis", Vec3(0, 0, 1), _HELP("Main rotation axis in world space")),
            InputPortConfig<float>("MinTwist", 0.0f, _HELP("Lower rotation limit around TwistAxis")),
            InputPortConfig<float>("MaxTwist", 0.0f, _HELP("Upper rotation limit around TwistAxis")),
            InputPortConfig<float>("MaxBend", 0.0f, _HELP("Maximum bend of the TwistAxis")),

            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("Id", _HELP("Constraint Id")),
            OutputPortConfig<bool>("Broken", _HELP("Activated when the constraint breaks")),
            {0}
        };
        config.sDescription = _HELP("Creates a physical constraint between EntityA and EntityB");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        SConstraintRec* pRec = 0, * pRecNext;
        if (event == eFE_Update)
        {
            pe_status_pos sp;
            for (pRec = g_pConstrRec0; pRec; pRec = pRecNext)
            {
                pRecNext = pRec->next;
                if (pRec->pNode == this && (pRec->bBroken || pRec->pent->GetStatus(&sp) && sp.iSimClass == 7))
                {
                    ActivateOutput(pActInfo, OUT_BROKEN, true);
                    ActivateOutput(pActInfo, OUT_ID, pRec->idConstraint);
                    delete pRec;
                }
            }
            if (!g_pConstrRec0)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
        }
        else if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, IN_CREATE))
            {
                IEntity* pent[2] = { gEnv->pEntitySystem->GetEntity(GetPortEntityId(pActInfo, IN_ENTITY0)), gEnv->pEntitySystem->GetEntity(GetPortEntityId(pActInfo, IN_ENTITY1)) };
                if (!pent[0] || !pent[0]->GetPhysics())
                {
                    return;
                }
                IPhysicalEntity* pentPhys = pent[0]->GetPhysics();
                pe_action_add_constraint aac;
                int i;
                float f;
                aac.pBuddy = pent[1] ? pent[1]->GetPhysics() : WORLD_ENTITY;
                aac.id = GetPortInt(pActInfo, IN_ID);
                for (int iop = 0; iop < 2; iop++)
                {
                    if ((i = GetPortInt(pActInfo, IN_PARTID0 + iop * 2)) >= 0)
                    {
                        aac.partid[iop] = i;
                    }
                }
                aac.pt[0] = GetPortVec3(pActInfo, IN_POINT);
                aac.flags = world_frames | (GetPortBool(pActInfo, IN_IGNORE_COLLISIONS) ? constraint_ignore_buddy : 0) | (GetPortBool(pActInfo, IN_BREAKABLE) ? 0 : constraint_no_tears);
                pe_status_dynamics sd;
                sd.mass = 1.0f;
                if (GetPortBool(pActInfo, IN_MAX_FORCE_REL))
                {
                    pentPhys->GetStatus(&sd);
                }
                if ((f = GetPortFloat(pActInfo, IN_MAX_FORCE)) > 0)
                {
                    aac.maxPullForce = f * sd.mass;
                }
                if ((f = GetPortFloat(pActInfo, IN_MAX_TORQUE)) > 0)
                {
                    aac.maxBendTorque = f * sd.mass;
                }
                for (int iop = 0; iop < 2; iop++)
                {
                    aac.xlimits[iop] = DEG2RAD(GetPortFloat(pActInfo, IN_MIN_ROT + iop));
                }
                aac.yzlimits[0] = 0;
                aac.yzlimits[1] = DEG2RAD(GetPortFloat(pActInfo, IN_MAX_BEND));
                if (aac.xlimits[1] <= aac.xlimits[0] && aac.yzlimits[1] <= aac.yzlimits[0])
                {
                    aac.flags |= constraint_no_rotation;
                }
                else if (aac.xlimits[0] < gf_PI * -1.01f && aac.xlimits[1] > gf_PI * 1.01f && aac.yzlimits[1] > gf_PI * 0.51f)
                {
                    MARK_UNUSED aac.xlimits[0], aac.xlimits[1], aac.yzlimits[0], aac.yzlimits[1];
                }
                aac.qframe[0] = aac.qframe[1] = Quat::CreateRotationV0V1(Vec3(1, 0, 0), GetPortVec3(pActInfo, IN_AXIS));
                if (GetPortBool(pActInfo, IN_FORCE_AWAKE))
                {
                    pRec = new SConstraintRec;
                    pe_simulation_params sp;
                    sp.minEnergy = 0;
                    pentPhys->GetParams(&sp);
                    pRec->minEnergy = pRec->minEnergyRagdoll = sp.minEnergy;
                    new(&sp)pe_simulation_params;
                    sp.minEnergy = 0;
                    pentPhys->SetParams(&sp);
                    pe_params_articulated_body pab;
                    if (pentPhys->GetParams(&pab))
                    {
                        pRec->minEnergyRagdoll = pab.minEnergyLyingMode;
                        new(&pab)pe_params_articulated_body;
                        pab.minEnergyLyingMode = 0;
                        pentPhys->SetParams(&pab);
                    }
                    pe_action_awake aa;
                    aa.minAwakeTime = 0.1f;
                    pentPhys->Action(&aa);
                    if (aac.pBuddy != WORLD_ENTITY)
                    {
                        aac.pBuddy->Action(&aa);
                    }
                }
                pe_params_flags pf;
                int queued = aac.pBuddy == WORLD_ENTITY ? 0 : aac.pBuddy->SetParams(&pf) - 1, id = pentPhys->Action(&aac, -queued >> 31);
                ActivateOutput(pActInfo, OUT_ID, id);
                if (!is_unused(aac.maxPullForce || !is_unused(aac.maxBendTorque)) && !(aac.flags & constraint_no_tears))
                {
                    if (!pRec)
                    {
                        pRec = new SConstraintRec;
                    }
                    gEnv->pPhysicalWorld->AddEventClient(EventPhysJointBroken::id, (int(*)(const EventPhys*))OnConstraintBroken, 1);
                }
                if (pRec)
                {
                    (pRec->pent = pentPhys)->AddRef();
                    pRec->idConstraint = id;
                    pRec->pNode = this;
                    if (g_pConstrRec0)
                    {
                        g_pConstrRec0->prev = pRec;
                    }
                    pRec->next = g_pConstrRec0;
                    g_pConstrRec0 = pRec;
                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                }
                return;
            }

            if (IsPortActive(pActInfo, IN_BREAK) || IsPortActive(pActInfo, IN_POINT))
            {
                if (IEntity* pent = gEnv->pEntitySystem->GetEntity(GetPortEntityId(pActInfo, IN_ENTITY0)))
                {
                    if (IPhysicalEntity* pentPhys = pent->GetPhysics())
                    {
                        pe_action_update_constraint auc;
                        auc.idConstraint = GetPortInt(pActInfo, IN_ID);
                        if (IsPortActive(pActInfo, IN_BREAK))
                        {
                            auc.bRemove = 1;
                        }
                        if (IsPortActive(pActInfo, IN_POINT))
                        {
                            auc.pt[1] = GetPortVec3(pActInfo, IN_POINT);
                        }
                        pentPhys->Action(&auc);
                        if (auc.bRemove)
                        {
                            for (pRec = g_pConstrRec0; pRec; pRec = pRecNext)
                            {
                                pRecNext = pRec->next;
                                if (pRec->pent == pentPhys && pRec->idConstraint == auc.idConstraint)
                                {
                                    delete pRec;
                                }
                            }
                            ActivateOutput(pActInfo, OUT_BROKEN, true);
                            ActivateOutput(pActInfo, OUT_ID, auc.idConstraint);
                            if (!g_pConstrRec0)
                            {
                                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                            }
                        }
                    }
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

CFlowNode_Constraint::SConstraintRec* CFlowNode_Constraint::g_pConstrRec0 = 0;

//////////////////////////////////////////////////////////////////////////
/// Physics Flow Node Registration
//////////////////////////////////////////////////////////////////////////

class CFlowNode_Collision
    : public CFlowBaseNode<eNCT_Instanced>
{
public:

    enum Inputs
    {
        In_AddListener,
        In_IgnoreSameNode,
        In_RemoveListener,
    };

    enum Outputs
    {
        Out_IdA,
        Out_PartIdA,
        Out_IdB,
        Out_PartIdB,
        Out_Point,
        Out_Normal,
        Out_SurfaceTypeA,
        Out_SurfaceTypeB,
        Out_HitImpulse,
    };

    CFlowNode_Collision(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_Collision(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<FlowEntityId>("AddListener", 0, _HELP("Register entity id as collision listener")),
            InputPortConfig<bool>("IgnoreSameNode", false, _HELP("Suppress events if both colliders are registered via the same node")),
            InputPortConfig<FlowEntityId>("RemoveListener", 0, _HELP("Unregister entity id from collision listeners")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<FlowEntityId>("IdA", _HELP("Id of the first colliding entity (one that is registered as a listener)")),
            OutputPortConfig<int>("PartIdA", _HELP("Part id inside the first colliding entity")),
            OutputPortConfig<FlowEntityId>("IdB", _HELP("Id of the second colliding entity")),
            OutputPortConfig<int>("PartIdB", _HELP("Part id inside the second colliding entity")),
            OutputPortConfig<Vec3>("Point", _HELP("Collision point in world space")),
            OutputPortConfig<Vec3>("Normal", _HELP("Collision normal in world space")),
            OutputPortConfig<string>("SurfacetypeA", _HELP("Name of the surfacetype of the first colliding entity")),
            OutputPortConfig<string>("SurfacetypeB", _HELP("Name of the surfacetype of the second colliding entity")),
            OutputPortConfig<float>("HitImpulse", _HELP("Collision impulse along the normal")),
            { 0 }
        };

        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Set up collision listeners");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, In_AddListener))
            {
                AddEntityId(pActInfo, GetPortEntityId(pActInfo, In_AddListener));
            }

            if (IsPortActive(pActInfo, In_RemoveListener))
            {
                RemoveEntityId(pActInfo, GetPortEntityId(pActInfo, In_RemoveListener));
            }
        }
        else if (event == eFE_Uninitialize)
        {
            while (m_entities.size() > 0)
            {
                RemoveEntityId(pActInfo, *m_entities.begin());
            }
        }
    }

private:

    using NodeListenerMap = AZStd::map < EntityId, AZStd::vector<SActivationInfo> >;

    // Keeping track of what nodes are listening to what entities.
    static NodeListenerMap s_nodeListeners;

    // Tracking all entities that this node is listening to so that we can:
    // -Quickly determine if this node is listening for a particular entity
    // -Clean up any references to this node in the node listeners map when this node uninitializes
    AZStd::set<EntityId> m_entities;

    void AddEntityId(SActivationInfo* pActInfo, EntityId entityId)
    {
        // Early out if we are already tracking this entity
        if (m_entities.find(entityId) != m_entities.end())
        {
            return;
        }

        IEntity* entity = gEnv->pEntitySystem->GetEntity(entityId);

        if (entity == nullptr)
        {
            CRY_ASSERT_MESSAGE(false, "Trying to listen for entity in collision flow node that does not exist");
            return;
        }

        IPhysicalEntity* physEntity = entity->GetPhysics();

        if (physEntity == nullptr)
        {
            CRY_ASSERT_MESSAGE(false, "Trying to listen for entity in collision flow node that does not have any physics");
            return;
        }

        // If we are the first listener, register our callback with the physics system
        if (s_nodeListeners.empty())
        {
            gEnv->pPhysicalWorld->AddEventClient(EventPhysCollision::id, CFlowNode_Collision::OnCollision, 1);
        }

        // Make sure that we are logging collision for this entity
        pe_params_flags pf;
        pf.flagsOR = pef_log_collisions;
        physEntity->SetParams(&pf);

        m_entities.insert(entityId);

        auto nodeListenersIt = s_nodeListeners.find(entityId);

        // If no one is listening to this entity, spin up a new vector
        if (nodeListenersIt == s_nodeListeners.end())
        {
            AZStd::vector< SActivationInfo > nodes = { *pActInfo };
            s_nodeListeners[entityId] = AZStd::move(nodes);
        }
        else
        {
            // Else, if this node is not already in the listeners list, go ahead and add it
            AZStd::vector<SActivationInfo>& nodeListeners = nodeListenersIt->second;

            auto nodeIt = AZStd::find_if(nodeListeners.begin(), nodeListeners.end(), [=](SActivationInfo& info) -> bool
                    {
                        return ActInfosAreSameNode(pActInfo, &info);
                    });

            if (nodeIt == nodeListeners.end())
            {
                nodeListeners.push_back(*pActInfo);
            }
        }
    }

    void RemoveEntityId(SActivationInfo* pActInfo, EntityId entityId)
    {
        // Remove the entity from the node's list of entities
        auto entityIt = m_entities.find(entityId);

        CRY_ASSERT_MESSAGE(entityIt != m_entities.end(), "Removing an entity id from a collision flow node, but that entity id is not being tracked on that node");

        if (entityIt != m_entities.end())
        {
            m_entities.erase(entityIt);
        }

        // Try to find nodes that are listening for this entity
        auto nodeListenersIt = s_nodeListeners.find(entityId);

        CRY_ASSERT_MESSAGE(nodeListenersIt != s_nodeListeners.end(), "Removing an entity id from collision flow node, but no collision listeners found for that entity id");

        if (nodeListenersIt != s_nodeListeners.end())
        {
            // Try to find our particular node within that list of listeners
            AZStd::vector< SActivationInfo >& listeners = nodeListenersIt->second;

            auto nodeIt = AZStd::find_if(listeners.begin(), listeners.end(), [=](SActivationInfo& info) -> bool
                    {
                        return ActInfosAreSameNode(pActInfo, &info);
                    });

            CRY_ASSERT_MESSAGE(nodeIt != listeners.end(), "Removing an entity id from a collision flow node, but that flow node is not registered as a listener for that entity");

            if (nodeIt != listeners.end())
            {
                // If there is only one listener (in which case, we are the only one), we can just clear the map and remove our event listener
                if (listeners.size() == 1)
                {
                    s_nodeListeners.erase(nodeListenersIt);
                }
                else
                {
                    // Else, remove this listener from the vector
                    AZStd::swap(*nodeIt, listeners.back());
                    listeners.pop_back();
                }
            }
        }

        // If there are no longer any listeners at all, remove our callback from the physics system
        if (s_nodeListeners.empty())
        {
            gEnv->pPhysicalWorld->RemoveEventClient(EventPhysCollision::id, OnCollision, 1);
        }
    }

    static bool ActInfosAreSameNode(SActivationInfo* actInfo1, SActivationInfo* actInfo2)
    {
        return actInfo1->pGraph == actInfo2->pGraph &&
               actInfo1->myID == actInfo2->myID;
    }

    static int OnCollision(const EventPhys* event)
    {
        const EventPhysCollision* collision = static_cast<const EventPhysCollision*>(event);

        // Handle collision listeners for collideable A
        HandleOutputPorts(collision, 0, 1);

        // Handle collision listeners for collideable B
        HandleOutputPorts(collision, 1, 0);

        return 1;
    }

    static void HandleOutputPorts(const EventPhysCollision* collision, int index1, int index2)
    {
        if (collision->iForeignData[index1] != PHYS_FOREIGN_ID_ENTITY)
        {
            return;
        }

        IEntity* entity = (IEntity*)collision->pForeignData[index1]; // Note: PhysicsForeignData overrides the cast here and actually does a reinterpret_cast
        EntityId idA = entity->GetId();
        NodeListenerMap::iterator nodeListenersIt = s_nodeListeners.find(idA);

        // If no one is listening for this entity, we can early out now
        if (nodeListenersIt == s_nodeListeners.end())
        {
            return;
        }

        EntityId idB = INVALID_ENTITYID;

        // If we collided with an entity, get that entity's id
        if (collision->iForeignData[index2] == PHYS_FOREIGN_ID_ENTITY)
        {
            IEntity* otherEntity = (IEntity*)collision->pForeignData[index2];
            idB = otherEntity->GetId();
        }

        AZStd::vector<SActivationInfo>& nodes = nodeListenersIt->second;
        float normalMultiplier = (1.0f - static_cast<float>(index1) * 2.0f);

        // Go through all nodes that are listening for collisions on this entity
        for (SActivationInfo& actInfo : nodes)
        {
            // If this flag is set, skip over this node if the other entity is also being listened to by this node
            if (GetPortBool(&actInfo, In_IgnoreSameNode))
            {
                IFlowNodeData* flowNodeData = actInfo.pGraph->GetNodeData(actInfo.myID);
                CFlowNode_Collision* flowNode = static_cast<CFlowNode_Collision*>(flowNodeData->GetNode());

                if (flowNode->m_entities.find(idB) != flowNode->m_entities.end())
                {
                    continue;
                }
            }

            ActivateOutput(&actInfo, Out_PartIdA, collision->partid[index1]);
            ActivateOutput(&actInfo, Out_IdB, idB);
            ActivateOutput(&actInfo, Out_PartIdB, collision->partid[index2]);
            ActivateOutput(&actInfo, Out_Point, collision->pt);
            ActivateOutput(&actInfo, Out_Normal, collision->n * normalMultiplier);
            ActivateOutput(&actInfo, Out_HitImpulse, collision->normImpulse);

            ISurfaceTypeManager* surfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
            ISurfaceType* surface1 = surfaceTypeManager->GetSurfaceType(collision->idmat[index1]);
            ISurfaceType* surface2 = surfaceTypeManager->GetSurfaceType(collision->idmat[index2]);

            if (surface1 != nullptr)
            {
                ActivateOutput(&actInfo, Out_SurfaceTypeA, string(surface1->GetName()));
            }

            if (surface2 != nullptr)
            {
                ActivateOutput(&actInfo, Out_SurfaceTypeB, string(surface2->GetName()));
            }

            ActivateOutput(&actInfo, Out_IdA, idA);
        }
    }
};

CFlowNode_Collision::NodeListenerMap CFlowNode_Collision::s_nodeListeners;

REGISTER_FLOW_NODE("Physics:Dynamics", CFlowNode_Dynamics);
REGISTER_FLOW_NODE("Physics:ActionImpulse", CFlowNode_ActionImpulse);
REGISTER_FLOW_NODE("Physics:RayCast", CFlowNode_Raycast);
REGISTER_FLOW_NODE("Physics:RayCastCamera", CFlowNode_RaycastCamera);
REGISTER_FLOW_NODE("Physics:PhysicsEnable", CFlowNode_PhysicsEnable);
REGISTER_FLOW_NODE("Physics:PhysicsSleepQuery", CFlowNode_PhysicsSleepQuery);
REGISTER_FLOW_NODE("Physics:Constraint", CFlowNode_Constraint);
REGISTER_FLOW_NODE("Physics:CameraProxy", CFlowNode_CameraProxy);
REGISTER_FLOW_NODE("Physics:CollisionListener", CFlowNode_Collision);
