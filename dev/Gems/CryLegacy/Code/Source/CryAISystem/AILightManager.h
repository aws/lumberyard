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

// Description : Keeps track of the logical light level in the game world and
//               provides services to make light levels queries


#ifndef CRYINCLUDE_CRYAISYSTEM_AILIGHTMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_AILIGHTMANAGER_H
#pragma once

#include "StlUtils.h"


class CAILightManager
{
public:
    CAILightManager();

    void Reset();
    void Update(bool forceUpdate);
    void DebugDraw();
    void Serialize(TSerialize ser);

    void DynOmniLightEvent(const Vec3& pos, float radius, EAILightEventType type, CAIActor* pShooter, float time);
    void DynSpotLightEvent(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightEventType type, CAIActor* pShooter, float time);

    // Returns light level at specified location
    EAILightLevel   GetLightLevelAt(const Vec3& pos, const CAIActor* pAgent = 0, bool* outUsingCombatLight = 0);

private:

    void DebugDrawArea(const ListPositions& poly, float zmin, float zmax, ColorB color);
    void UpdateTimeOfDay();
    void UpdateLights();

    struct SAIDynLightSource
    {
        SAIDynLightSource(const Vec3& _pos, const Vec3& _dir, float _radius, float _fov, EAILightLevel _level, EAILightEventType _type, CWeakRef<CAIActor> _refShooter, CCountedRef<CAIObject> _refAttrib, float _t)
            : pos(_pos)
            , dir(_dir)
            , radius(_radius)
            , fov(_fov)
            , level(_level)
            , type(_type)
            , refShooter(_refShooter)
            , refAttrib(_refAttrib)
            , t(0)
            , tmax((int)(_t * 1000.0f)) {}
        SAIDynLightSource()
            : pos(ZERO)
            , dir(Vec3_OneY)
            , radius(0)
            , fov(0)
            , level(AILL_NONE)
            , type(AILE_GENERIC)
            , t(0)
            , tmax((int)(t * 1000.0f)) {}
        SAIDynLightSource(const SAIDynLightSource& that)
            : pos(that.pos)
            , dir(that.dir)
            , radius(that.radius)
            , fov(that.fov)
            , level(that.level)
            , type(that.type)
            , t(that.t)
            , tmax(that.tmax)
            , refShooter(that.refShooter)
            , refAttrib(that.refAttrib) {}

        void Serialize(TSerialize ser);

        Vec3 pos;
        Vec3 dir;
        float radius;
        float fov;
        EAILightLevel level;
        CWeakRef<CAIActor> refShooter;
        CCountedRef<CAIObject> refAttrib;
        EAILightEventType type;

        int t;
        int tmax;
    };
    std::vector<SAIDynLightSource> m_dynLights;

    struct SAILightSource
    {
        SAILightSource(const Vec3& _pos, float _radius, EAILightLevel _level)
            : pos(_pos)
            , radius(_radius)
            , level(_level)
        {
            // Empty
        }

        Vec3 pos;
        float radius;
        EAILightLevel level;
    };

    std::vector<SAILightSource> m_lights;
    bool m_updated;
    EAILightLevel   m_ambientLight;

    CTimeValue  m_lastUpdateTime;
};


#endif // CRYINCLUDE_CRYAISYSTEM_AILIGHTMANAGER_H
