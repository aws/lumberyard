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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BIRDSFLOCK_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BIRDSFLOCK_H
#pragma once

#include "Flock.h"
#include "BirdEnum.h"

struct SLandPoint
    : Vec3
{
    bool bTaken;
    SLandPoint(const Vec3& p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        bTaken = false;
    }
};

//////////////////////////////////////////////////////////////////////////
// Specialized flocks for birds
//////////////////////////////////////////////////////////////////////////
class CBirdsFlock
    : public CFlock
{
public:
    CBirdsFlock(IEntity* pEntity);
    virtual void CreateBoids(SBoidsCreateContext& ctx);
    virtual void SetEnabled(bool bEnabled);
    virtual void Update(CCamera* pCamera);
    virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender);
    virtual void Reset();

    void SetAttractionPoint(const Vec3& point);
    bool GetAttractOutput() const { return m_bAttractOutput; }
    void SetAttractOutput(bool bAO) { m_bAttractOutput = bAO; }
    void Land();
    void TakeOff();
    void NotifyBirdLanded();
    Vec3 GetLandingPoint(Vec3& fromPos);
    void LeaveLandingPoint(Vec3& point);
    inline bool IsPlayerNearOrigin() const { return m_isPlayerNearOrigin; }

protected:
    Vec3& FindLandSpot();
    //  void UpdateAvgBoidPos(float dt);
    float GetFlightDuration();
    int GetNumAliveBirds();
    void UpdateLandingPoints();
    void LandCollisionCallback(const QueuedRayID& rayID, const RayCastResult& result);
    bool IsPlayerInProximity(const Vec3& pos) const;

private:
    typedef std::vector<SLandPoint> TLandingPoints;

    bool m_bAttractOutput; // Set to true when the AttractTo flownode output has been activated
    Vec3 m_defaultLandSpot;
    CTimeValue m_flightStartTime;
    Bird::EStatus m_status;
    float m_flightDuration;
    int m_birdsOnGround;
    TLandingPoints m_LandingPoints;
    int m_terrainPoints;
    bool m_hasLanded;
    bool m_isPlayerNearOrigin;
    CBoidCollision m_landCollisionInfo;
    GlobalRayCaster::ResultCallback m_landCollisionCallback;
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BIRDSFLOCK_H
