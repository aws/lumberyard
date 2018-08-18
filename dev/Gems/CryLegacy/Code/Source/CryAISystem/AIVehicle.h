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

#ifndef CRYINCLUDE_CRYAISYSTEM_AIVEHICLE_H
#define CRYINCLUDE_CRYAISYSTEM_AIVEHICLE_H
#pragma once

#include "Puppet.h"

class CAIVehicle
    : public CPuppet
{
public:
    CAIVehicle();
    ~CAIVehicle(void);

    void Update(EObjectUpdate type);
    void UpdateDisabled(EObjectUpdate type);
    void Navigate(CAIObject* pTarget);
    void Event(unsigned short eType, SAIEVENT* pEvent);

    void Reset(EObjectResetType type);
    void ParseParameters(const AIObjectParams& params, bool bParseMovementParams = true);
    virtual EntityId GetPerceivedEntityID() const;

    void AlertPuppets(void);
    virtual void Serialize(TSerialize ser);

    bool HandleVerticalMovement(const Vec3& targetPos);

    bool IsDriverInside() const;
    bool IsPlayerInside();

    EntityId GetDriverEntity() const;
    CAIActor* GetDriver() const;

    virtual bool IsTargetable() const { return IsActive(); }
    virtual bool IsActive() const { return m_bEnabled || IsDriverInside(); }

    virtual void GetPathFollowerParams(struct PathFollowerParams& outParams) const;

protected:
    void    FireCommand(void);
    bool    GetEnemyTarget(int objectType, Vec3& hitPosition, float fDamageRadius2, CAIObject** pTarget);
    void    OnDriverChanged(bool bEntered);

private:

    // local functions for firecommand()
    Vec3 PredictMovingTarget(const CAIObject* pTarget, const Vec3& vTargetPos, const Vec3& vFirePos, const float duration, const float distpred);
    bool CheckExplosion(const Vec3& vTargetPos, const Vec3& vFirePos, const Vec3& vActuallFireDir, const float fDamageRadius);
    Vec3 GetError(const Vec3& vTargetPos, const Vec3& vFirePos, const float fAccuracy);
    float RecalculateAccuracy(void);

    bool        m_bPoweredUp;

    CTimeValue  m_fNextFiringTime;          //To put an interval for the next firing.
    CTimeValue  m_fFiringPauseTime;         //Put a pause time after the trurret rotation has completed.
    CTimeValue  m_fFiringStartTime;         //the time that the firecommand started.

    Vec3        m_vDeltaTarget;             //To add a vector to the target position for errors and moving prediction

    int         m_ShootPhase;
    mutable int         m_driverInsideCheck;
    int         m_playerInsideCheck;
    bool        m_bDriverInside;
};

ILINE const CAIVehicle* CastToCAIVehicleSafe(const IAIObject* pAI) { return pAI ? pAI->CastToCAIVehicle() : 0; }
ILINE CAIVehicle* CastToCAIVehicleSafe(IAIObject* pAI) { return pAI ? pAI->CastToCAIVehicle() : 0; }

#endif // CRYINCLUDE_CRYAISYSTEM_AIVEHICLE_H

