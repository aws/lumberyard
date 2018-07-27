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
#include <Cry_Math.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzFramework/Math/MathUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>
#include <AzCore/Math/Matrix3x3.h>


//////////////////////////////////////////////////////////////////////////
class CRotateEntityToNode
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        Destination = 0,
        DynamicDestination,
        ValueType,
        Value,
        CoordSystem,
        StartMovement,
        Stop
    };

    enum OutputPorts
    {
        Current = 0,
        CurrentRadians,
        OnStart,
        OnStop,
        Finish,
        Done
    };

    enum EValueType
    {
        Speed = 0,
        Time
    };
    enum ECoordSystem
    {
        Parent = 0,
        World,
        Local
    };


    CTimeValue m_startTime;
    CTimeValue m_endTime;
    CTimeValue m_localStartTime;   // updated every time that the target rotation is dynamically updated in middle of the movement. When there is no update, is equivalent to m_startTime
    Quat m_targetQuat;
    Quat m_sourceQuat;
    ECoordSystem m_coorSys;
    EValueType m_valueType;
    bool m_bIsMoving;

public:
    CRotateEntityToNode(SActivationInfo* pActInfo)
        : m_coorSys(ECoordSystem::World)
        , m_valueType(EValueType::Speed)
        , m_bIsMoving(false)
    {
        m_targetQuat.SetIdentity();
        m_sourceQuat.SetIdentity();
    };

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CRotateEntityToNode(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("isMoving", m_bIsMoving);
        if (m_bIsMoving)
        {
            ser.Value("startTime", m_startTime);
            ser.Value("endTime", m_endTime);
            ser.Value("localStartTime", m_localStartTime);
            ser.Value("sourceQuat", m_sourceQuat);
            ser.Value("targetQuat", m_targetQuat);
        }
        ser.EndGroup();

        if (ser.IsReading())
        {
            m_coorSys = (ECoordSystem)GetPortInt(pActInfo, InputPorts::CoordSystem);
            m_valueType = (EValueType)GetPortInt(pActInfo, InputPorts::ValueType);
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Destination", _HELP("Destination (degrees on each axis)")),
            InputPortConfig<bool>("DynamicUpdate", true, _HELP("If dynamic update of [Destination] is allowed or not"), _HELP("DynamicUpdate")),
            InputPortConfig<int>("ValueType", 0, _HELP("Indicates the type of the [Value] input : Speed = 0,Time = 1 "), _HELP("ValueType"), _UICONFIG("enum_int:speed=0,time=1")),
            InputPortConfig<float>("Value", 0.f, _HELP("Speed (Degrees/sec) or Time (duration in secs), depending on [ValueType] input")),
            InputPortConfig<int>("CoordSys", 0, _HELP("Indicates the co-ordinate system of the [Destination] : Parent = 0, World = 1, Local = 2"), _HELP("CoordSys"), _UICONFIG("enum_int:Parent=0,World=1,Local=2")),
            InputPortConfig_Void("Start", _HELP("Activate to start movement")),
            InputPortConfig_Void("Stop", _HELP("Activate to stop movement")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("CurrentDeg", _HELP("Current Rotation in Degrees")),
            OutputPortConfig<Vec3>("CurrentRad", _HELP("Current Rotation in Radian")),
            OutputPortConfig_Void("OnStart", _HELP("Activated when [Start] or [Reset] inputs are triggered")),
            OutputPortConfig_Void("OnStop", _HELP("Activated when [Stop] input is triggered")),
            OutputPortConfig_Void("Finish", _HELP("Activated when [Destination] is reached")),
            OutputPortConfig_Void("Done", _HELP("Activated when [Destination] rotation is reached or [Stop] is Activated")),
            {0}
        };

        config.sDescription = _HELP("Rotate an entity during a defined period of time or with a defined speed");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void DegToQuat(const Vec3& rotation, Quat& destQuat)
    {
        Ang3 temp;
        temp.x = Snap_s360(rotation.x);
        temp.y = Snap_s360(rotation.y);
        temp.z = Snap_s360(rotation.z);
        const Ang3 angRad = DEG2RAD(temp);
        destQuat.SetRotationXYZ(angRad);
    }

    void SetRawEntityRot(IEntity* pEntity, const Quat& quat)
    {
        if (m_coorSys == ECoordSystem::World && pEntity->GetParent())
        {
            Matrix34 tm(quat);
            tm.SetTranslation(pEntity->GetWorldPos());
            pEntity->SetWorldTM(tm);
        }
        else
        {
            pEntity->SetRotation(quat);
        }
    }

    void UpdateCurrentRotOutputs(SActivationInfo* pActInfo, const Quat& quat)
    {
        Ang3 angles(quat);
        Ang3 anglesDeg = RAD2DEG(angles);
        ActivateOutput(pActInfo, OutputPorts::CurrentRadians, Vec3(angles));
        ActivateOutput(pActInfo, OutputPorts::Current, Vec3(anglesDeg));
    }

    void PhysicStop(IEntity* pEntity, const Quat* forcedOrientation = nullptr)
    {
        if (pEntity->GetPhysics())
        {
            pe_action_set_velocity asv;
            asv.w.zero();
            asv.bRotationAroundPivot = 1;
            pEntity->GetPhysics()->Action(&asv);
            pe_params_pos pos;
            pos.pos = pEntity->GetWorldPos();

            if (forcedOrientation == nullptr)
            {   // This might be one frame behind?
                pos.q = pEntity->GetWorldRotation();
            }
            else
            {
                pos.q = *forcedOrientation;
            }

            pEntity->GetPhysics()->SetParams(&pos);
        }
    }

    void PhysicSetApropiateSpeed(IEntity* pEntity)
    {
        if (pEntity->GetPhysics())
        {
            Quat targetQuat = (m_sourceQuat | m_targetQuat) < 0 ? -m_targetQuat : m_targetQuat;
            Vec3 rotVel = Quat::log(targetQuat * !m_sourceQuat) * (2.0f / max(0.01f, (m_endTime - m_localStartTime).GetSeconds()));
            pe_action_set_velocity asv;
            asv.w = rotVel;
            asv.bRotationAroundPivot = 1;
            pEntity->GetPhysics()->Action(&asv);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_coorSys = (ECoordSystem)GetPortInt(pActInfo, InputPorts::CoordSystem);
            m_valueType = (EValueType)GetPortInt(pActInfo, InputPorts::ValueType);
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            m_bIsMoving = false;
            break;
        }

        case eFE_Activate:
        {
            if (!pActInfo->pEntity)
            {
                break;
            }

            if (IsPortActive(pActInfo, InputPorts::Stop))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OutputPorts::Done, true);
                ActivateOutput(pActInfo, OutputPorts::OnStop, true);
                PhysicStop(pActInfo->pEntity);
                m_bIsMoving = false;
            }
            // when the target is updated in middle of the movement, we can not just continue the interpolation only changing the target because that would cause some snaps.
            // so we do sort of a new start but keeping the initial time into account, so we can still finish at the right time.
            if (IsPortActive(pActInfo, InputPorts::Destination) &&
                GetPortBool(pActInfo, InputPorts::DynamicDestination) == true && m_bIsMoving)
            {
                ReadSourceAndTargetQuats(pActInfo);
                m_localStartTime = gEnv->pTimer->GetFrameStartTime();
                CalcEndTime(pActInfo);
                if (m_valueType == EValueType::Time)
                {
                    PhysicSetApropiateSpeed(pActInfo->pEntity);
                }
            }
            if (IsPortActive(pActInfo, InputPorts::StartMovement))
            {
                bool triggerStartOutput = true;
                Start(pActInfo, triggerStartOutput);
            }
            if (IsPortActive(pActInfo, InputPorts::Value))
            {
                if (m_bIsMoving)
                {
                    bool triggerStartOutput = false;
                    Start(pActInfo, triggerStartOutput);
                }
            }

            // we dont support dynamic change of those inputs
            assert(!IsPortActive(pActInfo, InputPorts::CoordSystem));
            assert(!IsPortActive(pActInfo, InputPorts::ValueType));

            break;
        }

        case eFE_Update:
        {
            if (!pActInfo->pEntity)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                m_bIsMoving = false;
                break;
            }

            CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
            const float fDuration = (m_endTime - m_localStartTime).GetSeconds();
            float fPosition;

            if (fDuration <= 0.0)
            {
                fPosition = 1.0;
            }
            else
            {
                fPosition = (curTime - m_localStartTime).GetSeconds() / fDuration;
                fPosition = clamp_tpl(fPosition, 0.0f, 1.0f);
            }

            if (curTime >= m_endTime)
            {
                if (m_bIsMoving)
                {
                    m_bIsMoving = false;
                    ActivateOutput(pActInfo, OutputPorts::Done, true);
                    ActivateOutput(pActInfo, OutputPorts::Finish, true);
                }

                // By delaying the shutdown of regular updates, we generate
                // the physics/entity stop + snap request multiple times. This seems
                // to fix a weird glitch in Jailbreak whereby doors sometimes seem to
                // overshoot their end orientation (physics components seem to be correct
                // but the render components are off).
                static const float s_Crysis3HacDeactivateDelay = 1.0f;

                if (curTime >= (m_endTime + s_Crysis3HacDeactivateDelay))
                {
                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                }
                SetRawEntityRot(pActInfo->pEntity, m_targetQuat);
                PhysicStop(pActInfo->pEntity, &m_targetQuat);
                UpdateCurrentRotOutputs(pActInfo, m_targetQuat);
            }
            else
            {
                Quat quat;
                quat.SetSlerp(m_sourceQuat, m_targetQuat, fPosition);
                SetRawEntityRot(pActInfo->pEntity, quat);
                UpdateCurrentRotOutputs(pActInfo, quat);
            }
            break;
        }
        }
    }

    void Start(SActivationInfo* pActInfo, bool triggerStartOutput)
    {
        m_bIsMoving = true;

        ReadSourceAndTargetQuats(pActInfo);

        m_startTime = gEnv->pTimer->GetFrameStartTime();
        m_localStartTime = m_startTime;
        CalcEndTime(pActInfo);

        PhysicSetApropiateSpeed(pActInfo->pEntity);

        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
        if (triggerStartOutput)
        {
            ActivateOutput(pActInfo, OutputPorts::OnStart, true);
        }
    }


    void ReadSourceAndTargetQuats(SActivationInfo* pActInfo)
    {
        m_sourceQuat = m_coorSys == ECoordSystem::World ? pActInfo->pEntity->GetWorldRotation() : pActInfo->pEntity->GetRotation();
        m_sourceQuat.Normalize();

        const Vec3& destRotDeg = GetPortVec3(pActInfo, InputPorts::Destination);
        if (m_coorSys == ECoordSystem::Local)
        {
            Quat rotQuat;
            DegToQuat(destRotDeg, rotQuat);
            m_targetQuat = m_sourceQuat * rotQuat;
        }
        else
        {
            DegToQuat(destRotDeg, m_targetQuat);
        }
    }



    void CalcEndTime(SActivationInfo* pActInfo)
    {
        if (m_valueType == EValueType::Time)
        {
            CTimeValue timePast = m_localStartTime - m_startTime;
            float duration = GetPortFloat(pActInfo, InputPorts::Value) - timePast.GetSeconds();
            m_endTime = m_startTime + duration;
        }
        else // when is speed-driven, we just calculate the appropriate endTime and from that on, everything else is the same
        {
            Quat qOriInv = m_sourceQuat.GetInverted();
            Quat qDist = qOriInv * m_targetQuat;
            qDist.NormalizeSafe();

            float angDiff = RAD2DEG(2 * acosf(qDist.w));
            if (angDiff > 180.f)
            {
                angDiff = 360.0f - angDiff;
            }

            float desiredSpeed = fabsf(GetPortFloat(pActInfo, InputPorts::Value));
            if (desiredSpeed < 0.0001f)
            {
                desiredSpeed = 0.0001f;
            }
            m_endTime = m_localStartTime + angDiff / desiredSpeed;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CRotateEntity
    : public CFlowBaseNode < eNCT_Instanced >
{
private:

    enum InputPorts
    {
        Enable = 0,
        Disable,
        Velocity,
        Reference
    };

    enum OutputPorts
    {
        CurrentDegrees = 0,
        CurrentRadians,
    };

    Quat       m_worldRot;
    Quat       m_localRot;
    CTimeValue m_lastTime;
    bool       m_bActive;

public:

    CRotateEntity(SActivationInfo* pActInfo)
        : m_lastTime(0.0f)
        , m_bActive(false)
    {
        m_worldRot.SetIdentity();
        m_localRot.SetIdentity();
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CRotateEntity(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("m_worldRot", m_worldRot);
        ser.Value("m_localRot", m_localRot);
        ser.Value("m_lastTime", m_lastTime);
        ser.Value("m_bActive", m_bActive);
        ser.EndGroup();
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Enable", _HELP("Trigger this port to enable updates")),
            InputPortConfig_Void("Disable", _HELP("Trigger this port to disable updates")),
            InputPortConfig<Vec3>("Velocity", _HELP("Angular velocity (degrees/sec)")),
            InputPortConfig<int>("Reference", 0, _HELP("Indicates the Co-ordinate system for rotation (World = 0,Local = 1)"), _HELP("CoordSys"), _UICONFIG("enum_int:World=0,Local=1")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("CurrentDegrees", _HELP("Current Rotation in Degrees")),
            OutputPortConfig<Vec3>("CurrentRadians", _HELP("Current Rotation in Radian")),
            { 0 }
        };

        config.sDescription = _HELP("Rotates indicated Entity continuously until paused");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            m_lastTime = gEnv->pTimer->GetFrameStartTime();

            // If the Rotation has been enabled
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                m_bActive = true;
            }
            // If the Rotation has been disabled
            else if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ResetVelocity(pActInfo);
                m_bActive = false;
            }

            break;
        }

        case eFE_Initialize:
        {
            if (pActInfo->pEntity)
            {
                m_localRot = pActInfo->pEntity->GetRotation();
            }
            else
            {
                m_localRot.SetIdentity();
            }

            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            m_worldRot.SetIdentity();
            m_lastTime = gEnv->pTimer->GetFrameStartTime();
            break;
        }

        case eFE_Update:
        {
            if (m_bActive)
            {
                CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
                float deltaTime = (currentTime - m_lastTime).GetSeconds();
                m_lastTime = currentTime;

                Vec3 velocity = GetPortVec3(pActInfo, InputPorts::Velocity);
                velocity *= deltaTime;

                Quat deltaRotation(Ang3(DEG2RAD(velocity)));
                Vec3 currentOrientationEulerAngle;
                currentOrientationEulerAngle.zero();

                const bool useWorld = GetPortInt(pActInfo, InputPorts::Reference) == 0;

                if (pActInfo->pEntity)
                {
                    AZ_Assert(IsLegacyEntityId(pActInfo->entityId), "A Component entity ID should never be attached to a legacy entity");

                    IEntity* pEntity = pActInfo->pEntity;
                    Quat finalRotation;

                    if (!useWorld)
                    {
                        finalRotation = pEntity->GetRotation();
                        finalRotation *= deltaRotation;
                    }
                    else
                    {
                        m_worldRot *= deltaRotation;
                        finalRotation = m_worldRot;
                        finalRotation *= m_localRot;
                    }

                    finalRotation.NormalizeSafe();

                    IPhysicalEntity* piPhysEnt = pEntity->GetPhysics();

                    if (piPhysEnt && piPhysEnt->GetType() != PE_STATIC)
                    {
                        if (deltaTime > FLT_EPSILON)
                        {
                            pe_action_set_velocity asv;
                            asv.w = Quat::log(deltaRotation) * (2.f / deltaTime);
                            asv.bRotationAroundPivot = 1;
                            pEntity->GetPhysics()->Action(&asv);
                        }
                    }
                    else
                    {
                        pEntity->SetRotation(finalRotation);
                    }

                    Ang3 currentOrientation = Ang3(finalRotation);
                    currentOrientationEulerAngle = Vec3(currentOrientation);
                }

                if (!IsLegacyEntityId(pActInfo->entityId))
                {
                    AZ::Transform finalTransform;
                    if (useWorld)
                    {
                        AZ::Transform currentTransform;
                        EBUS_EVENT_ID_RESULT(currentTransform, pActInfo->entityId, AZ::TransformBus, GetWorldTM);
                        const AZ::Matrix3x3& currentRotationMatrix = AZ::Matrix3x3::CreateFromTransform(currentTransform);
                        const AZ::Matrix3x3& deltaRotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(LYQuaternionToAZQuaternion(deltaRotation));
                        finalTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(deltaRotationMatrix * currentRotationMatrix, currentTransform.GetTranslation());
                    }
                    else
                    {
                        AZ::Transform currentTransform;
                        EBUS_EVENT_ID_RESULT(currentTransform, pActInfo->entityId, AZ::TransformBus, GetWorldTM);

                        finalTransform = currentTransform * AZ::Transform::CreateFromQuaternion(LYQuaternionToAZQuaternion(deltaRotation));
                    }

                    EBUS_EVENT_ID(pActInfo->entityId, AZ::TransformBus, SetWorldTM, finalTransform);
                    currentOrientationEulerAngle = AZVec3ToLYVec3(AZ::ConvertTransformToEulerDegrees(finalTransform));
                }

                ActivateOutput(pActInfo, OutputPorts::CurrentRadians, currentOrientationEulerAngle);
                currentOrientationEulerAngle = RAD2DEG(currentOrientationEulerAngle);
                ActivateOutput(pActInfo, OutputPorts::CurrentDegrees, currentOrientationEulerAngle);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:

    void ResetVelocity(SActivationInfo* pActInfo)
    {
        IEntity* pGraphEntity = pActInfo->pEntity;
        if (pGraphEntity)
        {
            IPhysicalEntity* pPhysicalEntity = pGraphEntity->GetPhysics();

            if (pPhysicalEntity && pPhysicalEntity->GetType() != PE_STATIC)
            {
                pe_action_set_velocity setVel;
                setVel.v.zero();
                setVel.w.zero();
                pPhysicalEntity->Action(&setVel);
            }
        }
    }
};

REGISTER_FLOW_NODE("Movement:RotateEntityTo", CRotateEntityToNode);
REGISTER_FLOW_NODE("Movement:RotateEntity", CRotateEntity);
