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

// Description : Implements a seat action to rotate a turret


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATETURRET_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATETURRET_H
#pragma once

#include "InterpolationHelpers.h"

class CVehiclePartBase;
class CVehicle;
class CVehicleSeat;

// holds information for each of the rotation types
struct SVehiclePartRotationParameters
{
    SVehiclePartRotationParameters()
        : m_prevWorldQuat(IDENTITY)
    {
        m_pPart = NULL;
        m_action = 0.0f;
        m_aimAssist = 0.f;
        m_currentValue = 0.0f;
        m_speed = 0.0f;
        m_acceleration = 0.0f;
        m_minLimitF = 0.0f;
        m_minLimitB = 0.0f;
        m_maxLimit = 0.0f;
        m_relSpeed = 0.0f;
        m_worldSpace = false;
        m_orientation = InterpolatedQuat(Quat::CreateIdentity(), 0.75f, 3.0f);
        m_turnSoundId = InvalidSoundEventId;
        m_damageSoundId = InvalidSoundEventId;
    }

    InterpolatedQuat m_orientation; //  Interpolated orientation for this part

    CVehiclePartBase* m_pPart;  // TODO: IVehiclePart*?

    float m_action;                         // what the user input is requesting
    float m_aimAssist;                  // what the aim assist is requesting
    float m_currentValue;               // current rotation
    float m_speed;                          // speed of rotation (from vehicle xml)
    float m_acceleration;               // acceleration of rotation (from vehicle xml)

    float m_minLimitF;                  // smallest permissable value (from xml), when facing forwards
    float m_minLimitB;                  // smallest permissable value (from xml), when facing backwards
    float m_maxLimit;                       // largest permissable value (from xml)

    float m_relSpeed;                       // used to interpolate the rotation speed (for sound)

    Quat m_prevWorldQuat;               // previous world-space rotation

    TVehicleSoundEventId m_turnSoundId;
    TVehicleSoundEventId m_damageSoundId;

    bool m_worldSpace;                  // rotation should be calculated in world space
};


class CVehicleSeatActionRotateTurret
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT
public:

    // IVehicleSeatAction
    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime);

    virtual void UpdateFromPassenger(const float deltaTime, EntityId playerId);

    virtual void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }
    // ~IVehicleSeatAction

    void SetAimGoal(Vec3 aimPos, int priority = 0);
    bool GetRemainingAnglesToAimGoalInDegrees(float& pitch, float& yaw);    // FIXME: should be const, but IVehiclePart::GetLocalTM() is not const

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}
    virtual bool GetRotationLimits(int axis, float& min, float& max);

protected:

    enum EVehicleTurretRotationType
    {
        eVTRT_Pitch = 0,
        eVTRT_Yaw,
        eVTRT_NumRotationTypes,
    };

    void DoUpdate(const float deltaTime);

    void UpdateAimGoal();
    void MaintainPartRotationWorldSpace(EVehicleTurretRotationType eType);
    void UpdatePartRotation(EVehicleTurretRotationType eType, float frameTime);
    float GetDamageSpeedMul(CVehiclePartBase* pPart);

    bool InitRotation(IVehicle* pVehicle, const CVehicleParams& rotationTable, EVehicleTurretRotationType eType);
    bool InitRotationSounds(const CVehicleParams& rotationParams, EVehicleTurretRotationType eType);
    void UpdateRotationSound(EVehicleTurretRotationType eType, float speed, float deltaTime);

    CVehicle* m_pVehicle;
    IEntity* m_pUserEntity;
    CVehicleSeat* m_pSeat;

    Vec3 m_aimGoal;
    int m_aimGoalPriority;

    SVehiclePartRotationParameters m_rotations[eVTRT_NumRotationTypes];

    IVehicleHelper* m_rotTestHelpers[2];
    float m_rotTestRadius;

    friend class CVehiclePartBase;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATETURRET_H
