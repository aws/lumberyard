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

// Description : Implements a part for vehicles which extends the VehiclePartAnimated
//               but provides for using living entity type  useful for walkers


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDCHAR_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDCHAR_H
#pragma once

#include "VehiclePartAnimated.h"

class CVehiclePartAnimatedChar
    : public CVehiclePartAnimated
{
    IMPLEMENT_VEHICLEOBJECT
public:
    CVehiclePartAnimatedChar();
    virtual ~CVehiclePartAnimatedChar();

    // IVehiclePart
    virtual void Physicalize();
    // ~IVehiclePart
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDCHAR_H
