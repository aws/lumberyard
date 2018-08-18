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

// Description : Subclass of VehiclePartEntity that can be asked to detach at a random point in the future


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITYDELAYEDDETACH_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITYDELAYEDDETACH_H
#pragma once

//Base Class include
#include "VehiclePartEntity.h"

class CVehiclePartEntityDelayedDetach
    : public CVehiclePartEntity
{
public:
    IMPLEMENT_VEHICLEOBJECT

    CVehiclePartEntityDelayedDetach();
    virtual ~CVehiclePartEntityDelayedDetach();

    virtual void Update(const float frameTime);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

private:
    float                       m_detachTimer;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITYDELAYEDDETACH_H
