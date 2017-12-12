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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BOIDBIRD_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BOIDBIRD_H
#pragma once

#include "BoidObject.h"
#include "BirdEnum.h"
#include <IScriptSystem.h>
#include <IAISystem.h>

//////////////////////////////////////////////////////////////////////////
class CBoidBird
    : public CBoidObject
{
public:
    CBoidBird(SBoidContext& bc);
    virtual ~CBoidBird();

    virtual void Update(float dt, SBoidContext& bc);
    virtual void UpdatePhysics(float dt, SBoidContext& bc);
    virtual void Kill(const Vec3& hitPoint, const Vec3& force);
    virtual void OnFlockMove(SBoidContext& bc);
    virtual void Physicalize(SBoidContext& bc);
    virtual void Think(float dt, SBoidContext& bc);
    virtual bool ShouldUpdateCollisionInfo(const CTimeValue& t);

    void Land();
    void TakeOff(SBoidContext& bc);
    void SetAttracted(bool bAttracted = true)
    {
        m_attractedToPt = bAttracted;
        m_fAttractionFactor = 0;
    }
    void SetSpawnFromPt(bool bSpawnFromPt = true) { m_spawnFromPt = bSpawnFromPt; }
    bool IsLanding() { return m_status == Bird::LANDING || m_status == Bird::ON_GROUND; }
    static void SetTakeOffAnimLength(float l) { m_TakeOffAnimLength = l; }

    // Parameters for birds spawned from a point
    float m_fNoCenterAttract; // Compensates for attraction to center point
    float m_fNoKeepHeight;    // Compensates for attraction to terrain
    float m_fAttractionFactor;

protected:
    void Landed(SBoidContext& bc);
    void SetStatus(Bird::EStatus status);
    virtual void UpdateAnimationSpeed(SBoidContext& bc);
    void UpdatePitch(float dt, SBoidContext& bc);
    // avoiding virtual functions (calcmovement is called only from CBoidBird)
    void CalcMovementBird(float dt, SBoidContext& bc, bool banking);
    void ThinkWalk(float dt, SBoidContext& bc);
    void UpdateOnGroundAction(float dt, SBoidContext& bc);
    virtual void ClampSpeed(SBoidContext& bc, float dt);

protected:
    static float m_TakeOffAnimLength;

    float m_actionTime;          //!< Time flying/walking/standing after take off.
    float m_maxActionTime;       // Time this bird can be in flight/walk/stand.
    float m_lastThinkTime;       //! Time of last think operation.
    float m_elapsedSlowdownTime; // accumulated time since we started to slow down in order to come to a rest after
                                 // SBoidContext::fWalkToIdleDuration seconds
    float m_desiredHeigh;        // Desired height this birds want to fly at.
    float m_startLandSpeed;
    Vec3 m_birdOriginPos;
    Vec3 m_birdOriginPosTrg;
    Vec3 m_landingPoint;
    Vec3 m_orientation;
    CTimeValue m_takeOffStartTime;
    float m_walkSpeed;

    CBoidCollision m_floorCollisionInfo;

    Bird::EStatus m_status;
    Bird::EOnGroundStatus m_onGroundStatus; // sub-status for when m_status == Bird::ON_GROUND
    // Flags.

    unsigned m_attractedToPt : 1; //! True if bird is attracted to a point
    unsigned m_spawnFromPt : 1;   //! True if bird is spawned from point
    unsigned m_landDecelerating : 1;
    unsigned m_playingTakeOffAnim : 1;
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BOIDBIRD_H
