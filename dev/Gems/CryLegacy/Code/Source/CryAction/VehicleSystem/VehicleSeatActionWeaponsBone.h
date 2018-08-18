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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONSBONE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONSBONE_H
#pragma once

#include "VehicleSeatActionWeapons.h"
#include "ICryAnimation.h"

struct ISkeletonPose;

class CVehicleSeatActionWeaponsBone
    : public CVehicleSeatActionWeapons
{
public:
    CVehicleSeatActionWeaponsBone();
    virtual ~CVehicleSeatActionWeaponsBone();

    // IVehicleSeatAction
    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    // ~IVehicleSeatAction

    // IWeaponFiringLocator
    virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit);
    virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos);
    virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
    // ~IWeaponFiringLocator

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void UpdateWeaponTM(SVehicleWeapon& weapon);

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

protected:

    bool CalcFiringPosDir(Vec3& rPos, Vec3* pDir = NULL, const IFireMode* pFireMode = NULL) const;


    Quat m_currentMovementRotation;
    ISkeletonPose* m_pSkeletonPose;
    int m_positionBoneId;
    IAnimationOperatorQueuePtr m_poseModifier;
    float m_forwardOffset;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONSBONE_H
