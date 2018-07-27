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
#include <ISystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>

namespace
{
    /**
    * Fetches the World position of an entity indicated by the given activation info
    * @param The activation information which has the entity whose world position is to be fetched
    * @return An outcome that , upon success has a Vec3 representing the World Position of the entity
    */
    AZ::Outcome<Vec3> GetEntityWorldPosition(const IFlowNode::SActivationInfo* const activationInfo)
    {
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);
        Vec3 position(0);

        if (activationInfo->pEntity)
        {
            AZ_Assert(entityType == FlowEntityType::Legacy,
                "A Component or Invalid entity ID should never be attached to a legacy entity");

            position = activationInfo->pEntity->GetWorldPos();
        }
        else if (entityType == FlowEntityType::Component)
        {
            AZ::Transform currentTransform(AZ::Transform::Identity());
            EBUS_EVENT_ID_RESULT(currentTransform, activationInfo->entityId, AZ::TransformBus, GetWorldTM);
            position = AZVec3ToLYVec3(currentTransform.GetPosition());
        }
        else
        {
            return AZ::Failure();
        }

        return AZ::Success(position);
    }

    /**
    * Fetches the Transform matrix of the parent of the entity indicated by the Activation info
    * @param The activation info that holds the entity in question
    * @param Out param that holds the Transform Matrix for the parent of the entity in question
    */
    void GetParentTransform(const IFlowNode::SActivationInfo* const activationInfo, Matrix34& parentTransformMatrix)
    {
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);

        if (entityType == FlowEntityType::Legacy)
        {
            if (activationInfo->pEntity)
            {
                IEntity* pParent = activationInfo->pEntity->GetParent();
                if (pParent)
                {
                    parentTransformMatrix = pParent->GetWorldTM();
                }
            }
        }
        else if (entityType == FlowEntityType::Component)
        {
            AZ::EntityId parentId;
            EBUS_EVENT_ID_RESULT(parentId, activationInfo->entityId, AZ::TransformBus, GetParentId);

            if (parentId.IsValid())
            {
                AZ::Transform parentTransform(AZ::Transform::Identity());
                EBUS_EVENT_ID_RESULT(parentTransform, parentId, AZ::TransformBus, GetWorldTM);
                parentTransformMatrix = AZTransformToLYTransform(parentTransform);
            }
        }
    }

    /**
    * Fetches the Transform matrix of the entity indicated by the Activation info
    * @param The activation info that holds the entity in question
    * @param Out param that holds the Transform Matrix of the entity in question
    */
    void GetWorldTransform(const IFlowNode::SActivationInfo* const pActInfo, Matrix34& worldTransformMatrix)
    {
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);
        if (entityType == FlowEntityType::Legacy)
        {
            if (pActInfo->pEntity)
            {
                worldTransformMatrix = pActInfo->pEntity->GetWorldTM();
            }
        }
        else if (entityType == FlowEntityType::Component)
        {
            AZ::Transform currentTransform(AZ::Transform::Identity());
            EBUS_EVENT_ID_RESULT(currentTransform, pActInfo->entityId, AZ::TransformBus, GetWorldTM);
            worldTransformMatrix = AZTransformToLYTransform(currentTransform);
        }
    }
}

class CMoveEntityTo
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Destination = 0,
        DynamicDestination,
        ValueType,
        Value,
        EaseIn,
        EaseOut,
        CoordSystem,
        InputStart,
        InputStop
    };

    enum EValueType
    {
        Speed = 0,
        Time
    };
    enum CoordSys
    {
        Parent = 0,
        World,
        Local
    };

    enum OutputPorts
    {
        Current = 0,
        OutStart,
        OutStop,
        Finish,
        Done
    };


    Vec3 m_position;
    Vec3 m_destination;
    Vec3 m_startPos;   // position the entity had when Start was last triggered
    float m_lastFrameTime;
    float m_topSpeed;
    float m_easeOutDistance;
    float m_easeInDistance;
    float m_startTime;
    CoordSys m_coorSys;
    EValueType m_valueType;
    bool m_bActive;

    // TODO: im not sure about the whole "stopping" thing. why just 1 frame? it probably could be even more than 1 in some situations, or 0. not changing it now, but keep an eye on it.
    bool m_stopping; // this is used as a workaround for a physics problem: when the velocity is set to 0, the object still can move, for 1 more frame, with the previous velocity.
    // calling Action() with threadSafe=1 does not solve that.
    // TODO: im not sure about the whole "stopping" thing. why just 1 frame? it probably could be even more than 1 in some situations, or 0. not changing it now, but keep an eye on it.
    bool m_bForceFinishAsTooNear;  // force finish when the target is too near. This avoids a problem with rapidly moving target position that is updated on every frame, when a fixed time is specified for the movement
    bool m_bUsePhysicUpdate;
    static const float EASE_MARGIN_FACTOR;

public:
    CMoveEntityTo(SActivationInfo* pActInfo)
        : m_position(ZERO)
        , m_destination(ZERO)
        , m_startPos(ZERO)
        , m_lastFrameTime(0.0f)
        , m_topSpeed(0.0f)
        , m_easeOutDistance(0.0f)
        , m_easeInDistance(0.0f)
        , m_coorSys(CoordSys::World)
        , m_valueType(EValueType::Speed)
        , m_startTime(0.f)
        , m_bActive(false)
        , m_stopping(false)
        , m_bForceFinishAsTooNear(false)
        , m_bUsePhysicUpdate(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CMoveEntityTo(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("Active", m_bActive);
        if (m_bActive)
        {
            ser.Value("position", m_position);
            ser.Value("destination", m_destination);
            ser.Value("startPos", m_startPos);
            ser.Value("lastFrameTime", m_lastFrameTime);
            ser.Value("topSpeed", m_topSpeed);
            ser.Value("easeInDistance", m_easeInDistance);
            ser.Value("easeOutDistance", m_easeOutDistance);
            ser.Value("stopping", m_stopping);
            ser.Value("startTime", m_startTime);
            ser.Value("bForceFinishAsTooNear", m_bForceFinishAsTooNear);
        }

        ser.EndGroup();

        if (ser.IsReading())
        {
            m_coorSys = (CoordSys) GetPortInt(pActInfo, InputPorts::CoordSystem);
            m_valueType = (EValueType) GetPortInt(pActInfo, InputPorts::ValueType);
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Destination", _HELP("Destination position")),
            InputPortConfig<bool>("DynamicDestination", true, _HELP("Indicates if Destination position is to be followed if it moves"), _HELP("DynamicUpdate")),
            InputPortConfig<int>("ValueType", 0, _HELP("Indicates the type of the [Value] input : Speed = 0,Time = 1 "), _HELP("ValueType"), _UICONFIG("enum_int:Speed=0,Time=1")),
            InputPortConfig<float>("Value", 0.f, _HELP("Speed (m/sec) or Time (duration in secs), depending on [ValueType] input."), _HELP("Value")),
            InputPortConfig<float>("EaseInDistance", 0.f, _HELP("Distance from [Destination] at which the moving entity starts slowing down (0=no slowing down)"), _HELP("EaseInDistance")),
            InputPortConfig<float>("EaseOutDistance", 0.f, _HELP("Distance from start position at which the moving entity starts speeding up (0=no slowing down)")),
            InputPortConfig<int>("CoordSys", 0, _HELP("Indicates the co-ordinate system of the [Destination] : Parent = 0, World = 1, Local = 2\n(Warning: Parent space doesn't work if your parent is a brush)"), _HELP("CoordSys"), _UICONFIG("enum_int:Parent=0,World=1,Local=2")),
            InputPortConfig_Void("Start", _HELP("Activate to start movement")),
            InputPortConfig_Void("Stop", _HELP("Activate to stop movement")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Current", _HELP("Current position")),
            OutputPortConfig_Void("OnStart", _HELP("Activated when [Start] is triggered")),
            OutputPortConfig_Void("OnStop", _HELP("Activated when [Stop] is triggered")),
            OutputPortConfig_Void("Finish", _HELP("Activated when destination is reached")),
            OutputPortConfig_Void("DoneTrigger", _HELP("Activated when destination is reached or [Stop] is triggered"), _HELP("Done")),
            {0}
        };

        config.sDescription = _HELP("Moves an entity to a destination position at the Indicated speed or Time interval");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            OnActivate(pActInfo);
            break;
        }
        case eFE_Initialize:
        {
            OnInitialize(pActInfo);
            break;
        }

        case eFE_Update:
        {
            OnUpdate(pActInfo);
            break;
        }
        }
        ;
    };

    //////////////////////////////////////////////////////////////////////////
    void OnActivate(SActivationInfo* pActInfo)
    {
        // update destination only if dynamic update is enabled. otherwise destination is read on Start/Reset only
        if (m_bActive && IsPortActive(pActInfo, InputPorts::Destination) &&
            GetPortBool(pActInfo, InputPorts::DynamicDestination))
        {
            ReadDestinationPosFromInput(pActInfo);
            if (m_valueType == EValueType::Time)
            {
                CalcSpeedFromTimeInput(pActInfo);
            }
        }

        if (m_bActive && IsPortActive(pActInfo, InputPorts::Value))
        {
            ReadSpeedFromInput(pActInfo);
        }

        if (IsPortActive(pActInfo, InputPorts::InputStart))
        {
            Start(pActInfo);
        }

        if (IsPortActive(pActInfo, InputPorts::InputStop))
        {
            Stop(pActInfo);
        }

        // we dont support dynamic change of those inputs
        // Todo Ly : These asserts seem overzealous
        assert(!IsPortActive(pActInfo, InputPorts::CoordSystem));
        assert(!IsPortActive(pActInfo, InputPorts::ValueType));
    }

    //////////////////////////////////////////////////////////////////////////
    void OnInitialize(SActivationInfo* pActInfo)
    {
        m_bActive = false;
        m_position = ZERO;
        m_coorSys = (CoordSys) GetPortInt(pActInfo, InputPorts::CoordSystem);
        m_valueType = (EValueType) GetPortInt(pActInfo, InputPorts::ValueType);

        // Disabling regular updates until "Start"
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
    }

    //////////////////////////////////////////////////////////////////////////
    void OnUpdate(SActivationInfo* pActInfo)
    {
        IEntity* pEnt = pActInfo->pEntity;
        IPhysicalEntity* physicalEntity = nullptr;

        // If there is no attached entity
        if (!pEnt)
        {
            // With a legacy entity id
            if (GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId) == FlowEntityType::Legacy)
            {
                // Do not regularly update
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                return;
            }
        }
        else
        {
            physicalEntity = m_bUsePhysicUpdate ? pEnt->GetPhysics() : nullptr;
        }

        if (m_stopping)
        {
            m_stopping = false;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);

            if (physicalEntity == nullptr)
            {
                SetPosition(pActInfo, m_destination);
            }

            m_bActive = false;
            return;
        }

        if (!m_bActive)
        {
            return;
        }

        float time = gEnv->pTimer->GetFrameStartTime().GetSeconds();
        float timeDifference = time - m_lastFrameTime;
        m_lastFrameTime = time;

        // note - if there's no physics then this will be the same, but if the platform is moved through physics, then
        //        we have to get back the actual movement - this maybe framerate dependent.
        AZ::Outcome<Vec3> entityWorldPosition = GetEntityWorldPosition(pActInfo);

        if (entityWorldPosition.IsSuccess())
        {
            m_position = entityWorldPosition.GetValue();
        }

        // computing the movement vector
        Vec3 oldPosition = m_position;

        if (m_bForceFinishAsTooNear || m_position.IsEquivalent(m_destination, 0.01f))
        {
            m_position = m_destination;
            oldPosition = m_destination;
            ActivateOutput(pActInfo, OutputPorts::Done, true);
            ActivateOutput(pActInfo, OutputPorts::Finish, true);
            SetPosition(pActInfo, m_position);   // for finishing we have to make a manual setpos even if there is physics involved, to make sure the entity will be where it should.

            if (physicalEntity)
            {
                pe_action_set_velocity setVel;
                setVel.v = ZERO;
                physicalEntity->Action(&setVel);
                m_stopping = true;
            }
            else
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                m_bActive = false;
            }
        }
        else
        {
            Vec3 direction = m_destination - m_position;
            float distance = direction.GetLength();
            Vec3 directionAndSpeed = direction.normalized();

            // ease-area calcs
            float distanceForEaseOutCalc = distance + m_easeOutDistance * EASE_MARGIN_FACTOR;
            if (distanceForEaseOutCalc < m_easeOutDistance)     // takes care of m_easeOutDistance being 0
            {
                directionAndSpeed *= distanceForEaseOutCalc / m_easeOutDistance;
            }
            else  // init code makes sure both eases dont overlap, when the movement is time defined. when it is speed defined, only ease out is applied if they overlap.
            {
                if (m_easeInDistance > 0.f)
                {
                    Vec3 vectorFromStart = m_position - m_startPos;
                    float distanceFromStart = vectorFromStart.GetLength();
                    float distanceForEaseInCalc = distanceFromStart + m_easeInDistance * EASE_MARGIN_FACTOR;
                    if (distanceForEaseInCalc < m_easeInDistance)
                    {
                        directionAndSpeed *= distanceForEaseInCalc / m_easeInDistance;
                    }
                }
            }

            directionAndSpeed *= (m_topSpeed * timeDifference);

            if (direction.GetLength() < directionAndSpeed.GetLength())
            {
                m_position = m_destination;
                m_bForceFinishAsTooNear = true;
            }
            else
            {
                m_position += directionAndSpeed;
            }
        }

        ActivateOutput(pActInfo, OutputPorts::Current, m_position);

        if (physicalEntity == nullptr)
        {
            SetPosition(pActInfo, m_position);
        }
        else
        {
            pe_action_set_velocity setVel;
            float rTimeStep = timeDifference > 0.000001f ? 1.f / timeDifference : 0.0f;
            setVel.v = (m_position - oldPosition) * rTimeStep;
            physicalEntity->Action(&setVel);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool CanUsePhysicsUpdate(IEntity* pEntity)
    {
        // Do not use physics when
        // a) there is no physics geometry
        // b) the entity is a child
        // c) the entity is not rigid
        if (pEntity)
        {
            if (IStatObj* pStatObj = pEntity->GetStatObj(ENTITY_SLOT_ACTUAL))
            {
                phys_geometry* pPhysGeom = pStatObj->GetPhysGeom();
                if (!pPhysGeom)
                {
                    return false;
                }
            }
            IPhysicalEntity* pPhysEnt = pEntity->GetPhysics();

            if (!pPhysEnt)
            {
                return false;
            }
            else
            {
                if (pEntity->GetParent() || pPhysEnt->GetType() == PE_STATIC)
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void SetPosition(SActivationInfo* pActInfo, const Vec3& vPos)
    {
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

        if (pActInfo->pEntity)
        {
            if (pActInfo->pEntity->GetParent())
            {
                Matrix34 tm = pActInfo->pEntity->GetWorldTM();
                tm.SetTranslation(vPos);
                pActInfo->pEntity->SetWorldTM(tm);
            }
            else
            {
                pActInfo->pEntity->SetPos(vPos);
            }
        }
        else if (entityType == FlowEntityType::Component)
        {
            AZ::Transform currentTransform(AZ::Transform::Identity());
            EBUS_EVENT_ID_RESULT(currentTransform, pActInfo->entityId, AZ::TransformBus, GetWorldTM);

            currentTransform.SetTranslation(LYVec3ToAZVec3(vPos));
            EBUS_EVENT_ID(pActInfo->entityId, AZ::TransformBus, SetWorldTM, currentTransform);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void ReadDestinationPosFromInput(SActivationInfo* pActInfo)
    {
        m_destination = GetPortVec3(pActInfo, InputPorts::Destination);
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

        Matrix34 finalDestinationTransformMatrix = Matrix34::CreateIdentity();

        switch (m_coorSys)
        {
        case CoordSys::Parent:
        {
            GetParentTransform(pActInfo, finalDestinationTransformMatrix);
            break;
        }
        case CoordSys::Local:
        {
            GetWorldTransform(pActInfo, finalDestinationTransformMatrix);
            break;
        }
        case CoordSys::World:
        default:
        {
            break;
        }
        }

        m_destination = finalDestinationTransformMatrix.TransformPoint(m_destination);
    }


    //////////////////////////////////////////////////////////////////////////
    void ReadSpeedFromInput(SActivationInfo* pActInfo)
    {
        if (m_valueType == EValueType::Time)
        {
            CalcSpeedFromTimeInput(pActInfo);
        }
        else
        {
            m_topSpeed = GetPortFloat(pActInfo, InputPorts::Value);

            if (m_topSpeed < 0.0f)
            {
                m_topSpeed = 0.0f;
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // when the inputs are set to work with "time" instead of speed, we just calculate speed from that time value
    void CalcSpeedFromTimeInput(SActivationInfo* pActInfo)
    {
        assert(GetPortInt(pActInfo, InputPorts::ValueType) == EValueType::Time);
        float movementDuration = GetPortFloat(pActInfo, InputPorts::Value);
        float timePassed = gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_startTime;
        float movementDurationLeft = movementDuration - timePassed;

        if (movementDurationLeft < 0.0001f)
        {
            movementDurationLeft = 0.0001f;
        }

        Vec3 movement = m_destination - m_position;
        float distance = movement.len();

        if (m_easeInDistance + m_easeOutDistance > distance)   // make sure they dont overlap
        {
            m_easeInDistance = distance - m_easeOutDistance;
        }

        float distanceNoEase = distance - (m_easeInDistance + m_easeOutDistance);

        const float factorAverageSpeedEase = 0.5f; // not real because is not lineal, but is good enough for this
        float realEaseInDistance = max(m_easeInDistance - m_easeInDistance * EASE_MARGIN_FACTOR, 0.f);
        float realEaseOutDistance = max(m_easeOutDistance - m_easeOutDistance * EASE_MARGIN_FACTOR, 0.f);

        m_topSpeed = ((realEaseInDistance / factorAverageSpeedEase) +
                      distanceNoEase + (realEaseOutDistance / factorAverageSpeedEase)) / movementDurationLeft; // this comes just from V = S / T;
    }

    //////////////////////////////////////////////////////////////////////////
    void Start(SActivationInfo* pActInfo)
    {
        IEntity* pEnt = pActInfo->pEntity;
        FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

        if (!pEnt && entityType != FlowEntityType::Component)
        {
            return;
        }

        m_bForceFinishAsTooNear = false;

        AZ::Outcome<Vec3> worldPosition = GetEntityWorldPosition(pActInfo);
        if (worldPosition.IsSuccess())
        {
            m_position = worldPosition.GetValue();
        }

        ActivateOutput(pActInfo, OutputPorts::Current, m_position);
        m_startPos = m_position;

        m_lastFrameTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
        m_startTime = m_lastFrameTime;

        ReadDestinationPosFromInput(pActInfo);

        m_easeOutDistance = GetPortFloat(pActInfo, InputPorts::EaseOut);
        m_easeInDistance = GetPortFloat(pActInfo, InputPorts::EaseIn);
        ReadSpeedFromInput(pActInfo);

        if (m_easeOutDistance < 0.0f)
        {
            m_easeOutDistance = 0.0f;
        }

        m_stopping = false;
        m_bActive = true;
        m_bUsePhysicUpdate = CanUsePhysicsUpdate(pEnt);

        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);

        ActivateOutput(pActInfo, OutputPorts::OutStart, true);
    }

    //////////////////////////////////////////////////////////////////////////
    void Stop(SActivationInfo* pActInfo)
    {
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);

        if (m_bActive)
        {
            m_bActive = false;
            ActivateOutput(pActInfo, OutputPorts::Done, true);

            if (m_bUsePhysicUpdate)
            {
                if (IEntity* pEntity = pActInfo->pEntity)
                {
                    if (IPhysicalEntity* pPhysEntity = pEntity->GetPhysics())
                    {
                        pe_action_set_velocity setVel;
                        setVel.v = ZERO;
                        pPhysEntity->Action(&setVel);
                    }
                }
            }
        }

        ActivateOutput(pActInfo, OutputPorts::OutStop, true);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

const float CMoveEntityTo::EASE_MARGIN_FACTOR = 0.2f;  // the eases don't go to/from speed 0, they go to/from this fraction of the max speed.

REGISTER_FLOW_NODE("Movement:MoveEntityTo", CMoveEntityTo);
