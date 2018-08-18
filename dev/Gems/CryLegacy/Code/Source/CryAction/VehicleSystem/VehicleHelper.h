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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEHELPER_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEHELPER_H
#pragma once

class CVehicle;

class CVehicleHelper
    : public IVehicleHelper
{
public:
    CVehicleHelper()
        : m_pParentPart(NULL)
    {
    }

    // IVehicleHelper
    virtual void Release() { delete this; }

    virtual const Matrix34& GetLocalTM() const { return m_localTM; }
    virtual void GetVehicleTM(Matrix34& vehicleTM, bool forced = false) const;
    virtual void GetWorldTM(Matrix34& worldTM) const;
    virtual void GetReflectedWorldTM(Matrix34& reflectedWorldTM) const;

    virtual Vec3 GetLocalSpaceTranslation() const;
    virtual Vec3 GetVehicleSpaceTranslation() const;
    virtual Vec3 GetWorldSpaceTranslation() const;

    virtual IVehiclePart* GetParentPart() const;
    // ~IVehicleHelper

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

protected:
    IVehiclePart* m_pParentPart;

    Matrix34 m_localTM;

    friend class CVehicle;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEHELPER_H
