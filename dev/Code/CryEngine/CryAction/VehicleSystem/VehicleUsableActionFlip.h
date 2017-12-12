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

// Description : Implements a usable action to recover a vehicle by flipping it over


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEUSABLEACTIONFLIP_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEUSABLEACTIONFLIP_H
#pragma once

class CVehicleUsableActionFlip
    : public IVehicleAction
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleUsableActionFlip();
    ~CVehicleUsableActionFlip() {}

    // IVehicleAction
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual int OnEvent(int eventType, SVehicleEventParams& eventParams);
    void GetMemoryStatistics(ICrySizer* s);
    // ~IVehicleAction

    // IVehicleObject
    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
    virtual void Update(const float deltaTime);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}
    // ~IVehicleObject

protected:

    CVehicle* m_pVehicle;
    float m_timer;
    float m_postReorientedTimer;
    Vec3 m_localAngVel;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEUSABLEACTIONFLIP_H
