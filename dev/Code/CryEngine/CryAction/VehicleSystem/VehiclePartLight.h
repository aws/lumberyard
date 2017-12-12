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

// Description : Implements a light part


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTLIGHT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTLIGHT_H
#pragma once

#include "VehiclePartBase.h"

class CVehiclePartLight
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartLight();
    ~CVehiclePartLight();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Reset();

    virtual void OnEvent(const SVehiclePartEvent& event);

    virtual void Physicalize() {}

    virtual void Update(const float frameTime);

    virtual void Serialize(TSerialize serialize, EEntityAspects);
    virtual void RegisterSerializer(IGameObjectExtension* gameObjectExt) {}
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        s->AddObject(m_components);
        s->AddObject(m_pHelper);
        CVehiclePartBase::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePart

    virtual void ToggleLight(bool enable);
    bool IsEnabled() { return m_enabled; }
    const string& GetLightType() const { return m_lightType; }

protected:

    virtual void UpdateLight(const float frameTime);

    string  m_lightType;
    CDLight m_light;
    float   m_lightViewDistanceMultiplier;
    _smart_ptr<IMaterial> m_pMaterial;

    std::vector<IVehicleComponent*> m_components;

    IVehicleHelper* m_pHelper;

    float m_diffuseMult[2];
    Vec3  m_diffuseCol;

    bool m_enabled;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTLIGHT_H
