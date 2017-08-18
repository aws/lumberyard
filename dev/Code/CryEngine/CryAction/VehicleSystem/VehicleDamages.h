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

// Description : Implements the base of the vehicle damages


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGES_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGES_H
#pragma once

class CVehicle;
class CVehicleDamagesGroup;

class CVehicleDamages
{
public:

    CVehicleDamages()
        : m_pVehicle(NULL) {}

    struct SDamageMultiplier
    {
        float mult;
        float splash;

        SDamageMultiplier()
            : mult(1.f)
            , splash(1.f) {}

        void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
    };

    enum
    {
        DEFAULT_HIT_TYPE = 0
    };

    void InitDamages(CVehicle* pVehicle, const CVehicleParams& table);
    void ReleaseDamages();
    void ResetDamages();
    void UpdateDamages(float frameTime);

    void GetDamagesMemoryStatistics(ICrySizer* pSizer) const;

    bool ProcessHit(float& damage, const HitInfo& hitInfo, bool splash);
    CVehicleDamagesGroup* GetDamagesGroup(const char* groupName);

    typedef std::map <int, SDamageMultiplier> TDamageMultipliers;
    static void ParseDamageMultipliers(TDamageMultipliers& multipliersByHitType, TDamageMultipliers& multipliersByProjectile, const CVehicleParams& table);

protected:
    CVehicle* m_pVehicle;

    typedef std::vector <CVehicleDamagesGroup*> TVehicleDamagesGroupVector;
    TVehicleDamagesGroupVector m_damagesGroups;

    SVehicleDamageParams m_damageParams;

    TDamageMultipliers m_damageMultipliersByHitType;
    TDamageMultipliers m_damageMultipliersByProjectile;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGES_H
