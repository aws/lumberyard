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

// Description : Implements a seat action to head/spot lights


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONLIGHTS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONLIGHTS_H
#pragma once

#include "SharedParams/ISharedParams.h"

class CVehiclePartLight;

class CVehicleSeatActionLights
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT

public:
    ~CVehicleSeatActionLights();

    // IVehicleSeatAction

    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }
    virtual void StartUsing(EntityId passengerId) {}
    virtual void ForceUsage() {};
    virtual void StopUsing() {}
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    // ~IVehicleSeatAction

    // IVehicleObject

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize() {}
    virtual void Update(const float deltaTime) {}

    // IVehicleObject

    // IVehicleEventListener

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    // ~IVehicleEventListener

protected:

    enum ELightActivation
    {
        eLA_Toggle = 0,
        eLA_Brake,
        eLA_Reversing
    };

    BEGIN_SHARED_PARAMS(SSharedParams)

    TVehicleSeatId      seatId;

    ELightActivation    activation;

    string                      onSound, offSound;

    END_SHARED_PARAMS

    struct SLightPart
    {
        inline SLightPart()
            : pPart(NULL)
        {
        }

        inline SLightPart(CVehiclePartLight* part)
            : pPart(part)
        {
        }

        CVehiclePartLight* pPart;

        void GetMemoryUsage(ICrySizer* pSizer) const {}
    };

    typedef std::vector<SLightPart> TVehiclePartLightVector;

    void ToggleLights(bool enable);
    void PlaySound(const string& name);

    IVehicle* m_pVehicle;

    SSharedParamsConstPtr       m_pSharedParams;

    TVehiclePartLightVector m_lightParts;

    bool                                        m_enabled;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONLIGHTS_H
