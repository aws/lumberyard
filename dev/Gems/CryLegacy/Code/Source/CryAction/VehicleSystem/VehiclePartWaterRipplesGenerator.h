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

// Description : Implements water ripple generation for boats and similar vehicles

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTWATERRIPPLESGENERATOR_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTWATERRIPPLESGENERATOR_H
#pragma once

#include "VehiclePartBase.h"

class CVehicle;

class CVehiclePartWaterRipplesGenerator
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT

public:

    CVehiclePartWaterRipplesGenerator();
    virtual ~CVehiclePartWaterRipplesGenerator();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Update(const float frameTime);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        CVehiclePartBase::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePart

private:

    IVehicle*       m_pVehicle;

    Vec3    m_localOffset;
    float m_waterRipplesScale;
    float m_waterRipplesStrength;
    float m_minMovementSpeed;
    bool  m_onlyMovingForward;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTWATERRIPPLESGENERATOR_H
