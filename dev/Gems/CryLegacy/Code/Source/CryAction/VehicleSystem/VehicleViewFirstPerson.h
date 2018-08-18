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

// Description : Implements the first person pit view for vehicles

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_H
#pragma once

#include "VehicleViewBase.h"

class CVehicleViewFirstPerson
    : public CVehicleViewBase
{
    IMPLEMENT_VEHICLEOBJECT;
public:

    CVehicleViewFirstPerson();
    virtual ~CVehicleViewFirstPerson() {}

    // IVehicleView
    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();

    virtual const char* GetName() { return m_name; }
    virtual bool IsThirdPerson() { return false; }

    virtual void OnStartUsing(EntityId playerId);
    virtual void OnStopUsing();

    virtual void Update(const float frameTime);
    virtual void UpdateView(SViewParams& viewParams, EntityId playerId = 0);

    void GetMemoryUsage(ICrySizer* s) const;
    // ~IVehicleView

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // ~IVehicleEventListener

protected:

    virtual Vec3 GetWorldPosGoal();
    virtual Quat GetWorldRotGoal();
    Quat GetVehicleRotGoal();

    void HideEntitySlots(IEntity* pEnt, bool hide);

    static const char* m_name;

    IVehicleHelper* m_pHelper;
    string m_sCharacterBoneName;
    Vec3 m_offset;
    float m_fov;
    float m_relToHorizon;
    bool m_hideVehicle;
    int m_frameSlot;
    Matrix34 m_invFrame;
    Vec3 m_frameObjectOffset;

    typedef std::multimap<EntityId, int> TSlots;
    TSlots m_slotFlags;

    Vec3 m_viewPosition;
    Quat m_viewRotation;

    float m_speedRot;

    EntityId m_passengerId;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_H

