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

// Description : Implements a seat action for shaking a list of parts based on
//               vehicle movement, speed and acceleration


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONSHAKEPARTS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONSHAKEPARTS_H
#pragma once

#include <SharedParams/ISharedParams.h>
#include "VehicleNoiseGenerator.h"

class CVehicleSeatActionShakeParts
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT

public:

    CVehicleSeatActionShakeParts();

    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset() {}
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime);

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:

    BEGIN_SHARED_PARAMS(SSharedParams)

    struct SPartInfo
    {
        unsigned int   partIndex;
        float          amplitudeUpDown;
        float          amplitudeRot;
        float          freq;
        float          suspensionAmp;
        float          suspensionResponse;
        float          suspensionSharpness;
    };
    typedef std::vector<SPartInfo>  TPartInfos;
    typedef const TPartInfos        TPartInfosConst;
    TPartInfos                      partInfos;

    END_SHARED_PARAMS

    struct SPartInstance
    {
        // Updated at runtime
        CVehicleNoiseValue  noiseUpDown;
        CVehicleNoiseValue  noiseRot;
        float               zpos;
    };

    typedef std::vector<SPartInstance>  TParts;
    IVehicle*                           m_pVehicle;
    TParts                              m_controlledParts;
    SSharedParamsConstPtr               m_pSharedParams;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONSHAKEPARTS_H
