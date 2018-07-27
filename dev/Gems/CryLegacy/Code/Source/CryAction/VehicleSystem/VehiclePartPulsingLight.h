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

//  Description: A vehicle light which pulses over time

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPULSINGLIGHT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPULSINGLIGHT_H
#pragma once

#include "VehiclePartLight.h"

class CVehiclePartPulsingLight
    : public CVehiclePartLight
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartPulsingLight();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void OnEvent(const SVehiclePartEvent& event);
    // ~IVehiclePart

    virtual void ToggleLight(bool enable);

protected:

    virtual void UpdateLight(const float frameTime);

    float m_colorMult;
    float m_minColorMult;
    float m_currentColorMultSpeed;

    //Data from XML
    float m_colorMultSpeed;
    float m_toggleOnMinDamageRatio;

    float m_colorMultSpeedStageTwo;
    float m_toggleStageTwoMinDamageRatio;

    float m_colorChangeTimer;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPULSINGLIGHT_H
