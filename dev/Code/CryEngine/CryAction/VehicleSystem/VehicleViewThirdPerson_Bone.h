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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_BONE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_BONE_H
#pragma once

#include "VehicleViewBase.h"

class CVehicleViewThirdPersonBone
    : public CVehicleViewBase
{
    IMPLEMENT_VEHICLEOBJECT;
public:
    CVehicleViewThirdPersonBone();
    ~CVehicleViewThirdPersonBone();

    // IVehicleView
    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);

    virtual const char* GetName() { return m_name; }
    virtual bool IsThirdPerson() { return true; }
    virtual bool IsPassengerHidden() { return false; }

    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
    virtual void UpdateView(SViewParams& viewParams, EntityId playerId);

    virtual void OnStartUsing(EntityId passengerId);

    virtual void Update(const float frameTime);

    virtual bool ShootToCrosshair() { return true; }
    // ~IVehicleView

    void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }

protected:
    static const char* m_name;

    int m_directionBoneId;
    Vec3 m_offset;
    float m_distance;
    Vec3 m_position;
    Quat m_rotation;
    Quat m_additionalRotation;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_BONE_H
