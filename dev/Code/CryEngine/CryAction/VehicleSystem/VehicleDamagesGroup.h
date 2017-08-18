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

// Description : Implements the base of the vehicle damages group


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESGROUP_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESGROUP_H
#pragma once

class CVehicle;

class CVehicleDamagesGroup
    : public IVehicleDamagesGroup
{
public:

    virtual ~CVehicleDamagesGroup();

    bool Init(CVehicle* pVehicle, const CVehicleParams& table);
    void Release() { delete this; }
    void Reset();
    void Serialize(TSerialize ser, EEntityAspects aspects);

    virtual bool ParseDamagesGroup(const CVehicleParams& table);

    const string& GetName() { return m_name; }
    bool IsPotentiallyFatal();

    void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);
    void Update(float frameTime);

    void GetMemoryUsage(ICrySizer* pSizer) const;

protected:

    typedef std::vector <IVehicleDamageBehavior*> TVehicleDamageBehaviorVector;

    typedef uint16 TDamagesSubGroupId;
    struct SDamagesSubGroup
    {
        TDamagesSubGroupId id;
        float m_randomness;
        TVehicleDamageBehaviorVector m_damageBehaviors;
        float m_delay;
        bool m_isAlreadyInProcess;

        SDamagesSubGroup()
        {
            id = ~0;
            m_randomness = 0.f;
            m_delay = 0.f;
            m_isAlreadyInProcess = false;
        }
        void GetMemoryUsage(ICrySizer* pSizer) const{}
    };

    typedef std::vector <SDamagesSubGroup> TDamagesSubGroupVector;
    TDamagesSubGroupVector m_damageSubGroups;

protected:

    IVehicleDamageBehavior* ParseDamageBehavior(const CVehicleParams& table);

    CVehicle* m_pVehicle;
    string m_name;

    struct SDelayedDamagesSubGroupInfo
    {
        float delay;
        TDamagesSubGroupId subGroupId;
        SVehicleDamageBehaviorEventParams behaviorParams;
    };

    typedef std::list<SDelayedDamagesSubGroupInfo> TDelayedDamagesSubGroupList;
    TDelayedDamagesSubGroupList m_delayedSubGroups;

    friend class CVehicleDamagesTemplateRegistry;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESGROUP_H
