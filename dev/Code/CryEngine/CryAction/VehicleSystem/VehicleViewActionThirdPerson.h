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

// Description: Implements a third person view for vehicles tweaked for action gameplay

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWACTIONTHIRDPERSON_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWACTIONTHIRDPERSON_H
#pragma once

#include "VehicleViewBase.h"

class CVehicleViewActionThirdPerson
    : public CVehicleViewBase
{
    IMPLEMENT_VEHICLEOBJECT;
public:

    CVehicleViewActionThirdPerson();
    ~CVehicleViewActionThirdPerson();

    // IVehicleView
    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void ResetPosition()
    {
        m_worldCameraPos = m_pVehicle->GetEntity()->GetWorldPos();
        m_worldViewPos = m_worldCameraPos;
    }

    virtual const char* GetName() { return m_name; }
    virtual bool IsThirdPerson() { return true; }
    virtual bool IsPassengerHidden() { return false; }

    virtual void OnStartUsing(EntityId passengerId);

    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
    virtual void UpdateView(SViewParams& viewParams, EntityId playerId);

    virtual void Update(const float frameTime);
    virtual void Serialize(TSerialize serialize, EEntityAspects);

    virtual void GetMemoryUsage(ICrySizer* s) const  {s->Add(*this); }
    // ~IVehicleView

protected:

    Matrix34 GetCameraTM(const Matrix34& worldTM, const Vec3& aimPos);
    Vec3 GetAimPos(const Matrix34& worldTM);

    Vec3 m_worldViewPos;
    Vec3 m_worldCameraPos;
    Vec3 m_worldCameraAim;

    IVehiclePart* m_pAimPart;

    Vec3 m_localCameraPos;
    Vec3 m_localCameraAim;
    Vec3 m_velocityMult;
    float m_lagSpeed;

    Vec3 m_extraLag;

    float m_zoom;
    float m_zoomTarget;
    float m_actionZoom;
    float m_verticalFilter;
    float m_verticalFilterOffset;

    float m_heightAboveWater;

    Vec3 m_cameraOffset;

    static const char* m_name;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWACTIONTHIRDPERSON_H
