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

// Description : Implements a third person view for vehicles


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_H
#pragma once

#include "VehicleViewBase.h"

class CVehicleViewThirdPerson
    : public CVehicleViewBase
{
    IMPLEMENT_VEHICLEOBJECT;
public:

    CVehicleViewThirdPerson();
    ~CVehicleViewThirdPerson();

    // IVehicleView
    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void ResetPosition()
    {
        m_position = m_pVehicle->GetEntity()->GetWorldPos();
    }

    virtual const char* GetName() { return m_name; }
    virtual bool IsThirdPerson() { return true; }
    virtual bool IsPassengerHidden() { return false; }

    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
    virtual void UpdateView(SViewParams& viewParams, EntityId playerId);

    virtual void OnStartUsing(EntityId passengerId);

    virtual void Update(const float frameTime);
    virtual void Serialize(TSerialize serialize, EEntityAspects);

    virtual bool ShootToCrosshair() { return false; }
    // ~IVehicleView

    bool Init(CVehicleSeat* pSeat);

    //! sets default view distance. if 0, the distance from the vehicle is used
    static void SetDefaultDistance(float dist);

    //! sets default height offset. if 0, the heightOffset from the vehicle is used
    static void SetDefaultHeight(float height);

    void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }

protected:

    I3DEngine* m_p3DEngine;
    IEntitySystem* m_pEntitySystem;

    EntityId m_targetEntityId;
    IVehiclePart* m_pAimPart;

    float m_distance;
    float m_heightOffset;

    float m_height;
    Vec3 m_vehicleCenter;

    static const char* m_name;

    Vec3 m_position;
    Vec3 m_worldPos;
    float m_rot;

    float m_interpolationSpeed;
    float m_boundSwitchAngle;
    float m_zoom;
    float m_zoomMult;

    static float m_defaultDistance;
    static float m_defaultHeight;

    float m_actionZoom;
    float m_actionZoomSpeed;

    bool m_isUpdatingPos;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWTHIRDPERSON_H
