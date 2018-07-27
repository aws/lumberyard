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

// Description : VehicleSystem CVars


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLECVARS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLECVARS_H
#pragma once


class CVehicleCVars
{
public:
#if ENABLE_VEHICLE_DEBUG
    // debug draw
    int v_debugdraw;
    int v_draw_components;
    int v_draw_helpers;
    int v_draw_seats;
    int v_draw_tm;
    int v_draw_passengers;
    int v_debug_mem;

    int v_debug_flip_over;
    int v_debug_reorient;

    int v_debugView;
    int v_debugViewDetach;
    int v_debugViewAbove;
    float v_debugViewAboveH;
    int v_debugCollisionDamage;
#endif

    // dev vars
    int v_transitionAnimations;
    int v_playerTransitions;
    int v_autoDisable;
    int v_lights;
    int v_lights_enable_always;
    int v_set_passenger_tm;
    int v_disable_hull;
    int v_ragdollPassengers;
    int v_goliathMode;
    int v_show_all;
    float v_treadUpdateTime;
    float v_tpvDist;
    float v_tpvHeight;
    int v_debugSuspensionIK;
    int v_serverControlled;
    int v_clientPredict;
    int v_clientPredictSmoothing;
    int v_testClientPredict;
    float v_clientPredictSmoothingConst;
    float v_clientPredictAdditionalTime;
    float v_clientPredictMaxTime;

    int v_enableMannequin;

    // tweaking
    float v_slipSlopeFront;
    float v_slipSlopeRear;
    float v_slipFrictionModFront;
    float v_slipFrictionModRear;

    float v_FlippedExplosionTimeToExplode;
    float v_FlippedExplosionPlayerMinDistance;
    int v_FlippedExplosionRetryTimeMS;

    int v_vehicle_quality;
    int v_driverControlledMountedGuns;
    int v_independentMountedGuns;

    int v_disableEntry;

    static __inline CVehicleCVars& Get()
    {
        assert (s_pThis != 0);
        return *s_pThis;
    }

private:
    friend class CVehicleSystem; // Our only creator

    CVehicleCVars(); // singleton stuff
    ~CVehicleCVars();
    CVehicleCVars(const CVehicleCVars&);
    CVehicleCVars& operator= (const CVehicleCVars&);

    static CVehicleCVars* s_pThis;
};

ILINE const CVehicleCVars& VehicleCVars() { return CVehicleCVars::Get(); }


#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLECVARS_H
