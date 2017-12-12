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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATEBONE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATEBONE_H
#pragma once

#include "ICryAnimation.h"

struct ISkeletonPose;
struct IAnimatedCharacter;

class CVehicleSeatActionRotateBone
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT

private:

public:

    CVehicleSeatActionRotateBone();
    virtual ~CVehicleSeatActionRotateBone();

    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime){}

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    virtual void GetMemoryUsage(ICrySizer* s) const;

    virtual void PrePhysUpdate(const float dt);

protected:
    IDefaultSkeleton* GetCharacterModelSkeleton() const;
    void UpdateSound(const float dt);

    IVehicle*       m_pVehicle;
    IVehicleSeat*   m_pSeat;

    IAnimationOperatorQueuePtr m_poseModifier;

    int m_MoveBoneId;

    Ang3 m_boneRotation;
    Ang3 m_prevBoneRotation;
    Quat m_boneBaseOrientation;
    Quat m_currentMovementRotation;
    Ang3 m_desiredBoneRotation;
    Ang3 m_rotationSmoothingRate;

    float m_soundRotSpeed;
    float m_soundRotLastAppliedSpeed;
    float m_soundRotSpeedSmoothRate;
    float m_soundRotSpeedSmoothTime;
    float m_soundRotSpeedScalar;
    string m_soundNameFP;
    string m_soundNameTP;
    string m_soundParamRotSpeed;

    float m_pitchLimitMax;
    float m_pitchLimitMin;
    float m_settlePitch;
    float m_settleDelay;
    float m_settleTime;
    float m_noDriverTime;

    float m_networkSluggishness;

    IAnimatedCharacter* m_pAnimatedCharacter;

    float m_autoAimStrengthMultiplier;
    float m_speedMultiplier;
    float m_rotationMultiplier;

    uint8 m_hasDriver : 1;
    uint8 m_driverIsLocal : 1;
    uint8 m_settleBoneOnExit : 1;
    uint8 m_useRotationSounds : 1;
    uint8 m_stopCurrentSound : 1;
    uint8 m_rotationLockCount : 2;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONROTATEBONE_H
