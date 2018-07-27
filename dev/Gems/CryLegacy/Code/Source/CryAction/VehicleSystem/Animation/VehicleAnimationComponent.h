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

// Description : Implements vehicle animation using Mannequin

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLEANIMATIONCOMPONENT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLEANIMATIONCOMPONENT_H
#pragma once

#include <ICryMannequinDefs.h>

class CVehicle;
class IAnimationDatabase;
class IActionController;
struct SAnimationContext;

class CVehicleAnimationComponent
    : public IVehicleAnimationComponent
{
public:
    CVehicleAnimationComponent();
    ~CVehicleAnimationComponent();

    void Initialise(CVehicle& vehicle, const CVehicleParams& mannequinTable);
    void Reset();
    void DeleteActionController();

    void Update(float timePassed);

    IActionController* GetActionController() {return m_pActionController; }
    void AttachPassengerScope(CVehicleSeat* seat, EntityId ent, bool attach);

    virtual CTagState* GetTagState()
    {
        return m_pAnimContext ? &m_pAnimContext->state : NULL;
    }

    ILINE bool IsEnabled() const
    {
        return (m_pActionController != NULL);
    }

private:
    CVehicle* m_vehicle;
    const IAnimationDatabase* m_pADBVehicle;
    const IAnimationDatabase* m_pADBPassenger;
    SAnimationContext* m_pAnimContext;
    IActionController* m_pActionController;
    TagID                                         m_typeTag;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLEANIMATIONCOMPONENT_H
