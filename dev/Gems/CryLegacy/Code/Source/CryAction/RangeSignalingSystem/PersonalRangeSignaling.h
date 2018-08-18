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

// Description : Manager per-actor signals to other actors by range


#ifndef CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_PERSONALRANGESIGNALING_H
#define CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_PERSONALRANGESIGNALING_H
#pragma once

#include "AIProxy.h"

class CRangeSignaling;
class CRange;
class CAngleAlert;

class CPersonalRangeSignaling
    : public IAIProxyListener
{
    typedef std::vector<CRange*>         VecRanges;
    typedef std::vector<CAngleAlert*>    VecAngles;
    typedef std::map<EntityId, VecRanges> MapTargetRanges;

public:
    // Base ----------------------------------------------------------
    CPersonalRangeSignaling(CRangeSignaling* pParent);
    virtual         ~CPersonalRangeSignaling();
    bool            Init(EntityId Id);
    bool            Update(float fElapsedTime, uint32 uDebugOrder = 0);
    void            Reset();
    void            OnProxyReset();
    void            SetEnabled (bool bEnable);
    bool            IsEnabled() const { return m_bEnabled; }

    // Utils ---------------------------------------------------------
    bool            AddRangeSignal(float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    bool            AddTargetRangeSignal(EntityId IdTarget, float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    bool            AddAngleSignal(float fAngle, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    EntityId        GetEntityId() const;
    IEntity*        GetEntity();
    IEntity const*  GetEntity() const;
    IActor*         GetActor();

private:
    CRange const*       GetRangeTo(const Vec3& vPos, const VecRanges& vecRangeList, bool bUseBoundary = false) const;
    CRange*             SearchRange(const char* sSignal, const VecRanges& vecRangeList) const;
    CAngleAlert const*  GetAngleTo(const Vec3& vPos, bool bUseBoundary = false) const;
    CAngleAlert*        SearchAngle(const char* sSignal) const;
    void                SendSignal(IActor* pActor, const string& sSignal, IAISignalExtraData* pData = NULL) const;
    IAISignalExtraData* PrepareSignalData(IAISignalExtraData* pRequestedData) const;
    void                DebugDraw(uint32 uOrder) const;
    void                CheckActorRanges(IActor* pActor);
    void                CheckActorTargetRanges(IActor* pActor);
    void                CheckActorAngles(IActor* pActor);

    // Returns AI object of an entity
    IAIObject const*    GetEntityAI(EntityId entityId) const;

    // Returns AI proxy of an entity
    IAIActorProxy*      GetEntityAIProxy(EntityId entityId) const;

    // IAIProxyListener
    void SetListener(bool bAdd);
    virtual void OnAIProxyEnabled(bool bEnabled);
    // ~IAIProxyListener

private:
    bool                        m_bInit;
    bool                        m_bEnabled;
    IFFont*                     m_pDefaultFont;
    CRangeSignaling*            m_pParent;
    EntityId                    m_EntityId;

    VecRanges m_vecRanges;
    VecAngles m_vecAngles;

    MapTargetRanges m_mapTargetRanges;

private:
    typedef std::map<EntityId, CRange const*>      MapRangeSignals;
    typedef std::map<EntityId, CAngleAlert const*> MapAngleSignals;

    MapRangeSignals m_mapRangeSignalsSent;
    MapRangeSignals m_mapTargetRangeSignalsSent;
    MapAngleSignals m_mapAngleSignalsSent;
};
#endif // CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_PERSONALRANGESIGNALING_H
