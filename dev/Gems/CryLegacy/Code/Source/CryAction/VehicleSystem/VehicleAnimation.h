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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEANIMATION_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEANIMATION_H
#pragma once

#include <IVehicleSystem.h>

class CVehiclePartAnimated;

class CVehicleAnimation
    : public IVehicleAnimation
{
public:

    CVehicleAnimation();
    virtual ~CVehicleAnimation() {}

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual bool StartAnimation();
    virtual void StopAnimation();

    virtual bool ChangeState(TVehicleAnimStateId stateId);
    virtual TVehicleAnimStateId GetState();

    virtual string GetStateName(TVehicleAnimStateId stateId);
    virtual TVehicleAnimStateId GetStateId(const string& name);

    virtual void SetSpeed(float speed);

    virtual void ToggleManualUpdate(bool isEnabled);
    virtual void SetTime(float time, bool force = false);

    virtual float GetAnimTime(bool raw = false);
    virtual bool IsUsingManualUpdates();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_animationStates);
    }
protected:

    struct SAnimationStateMaterial
    {
        string  material, setting;
        float       _min, _max;
        bool        invertValue;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(material);
            pSizer->AddObject(setting);
        }
    };

    typedef std::vector <SAnimationStateMaterial> TAnimationStateMaterialVector;

    struct SAnimationState
    {
        string name;
        string animation;

        string sound;
        IVehicleHelper* pSoundHelper;

        float speedDefault;
        float speedMin;
        float speedMax;
        bool isLooped;
        bool isLoopedEx;

        TAnimationStateMaterialVector materials;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
            pSizer->AddObject(animation);
            pSizer->AddObject(sound);
            pSizer->AddObject(pSoundHelper);
            pSizer->AddObject(materials);
        }
    };

    typedef std::vector <SAnimationState> TAnimationStateVector;

protected:

    bool ParseState(const CVehicleParams& table, IVehicle* pVehicle);
    _smart_ptr<IMaterial> FindMaterial(const SAnimationStateMaterial& animStateMaterial, _smart_ptr<IMaterial> pMaterial);

    CVehiclePartAnimated* m_pPartAnimated;
    int m_layerId;

    TAnimationStateVector m_animationStates;
    TVehicleAnimStateId m_currentStateId;

    bool m_currentAnimIsWaiting;
};


#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEANIMATION_H
