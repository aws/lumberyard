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

// Description : Ruler path agent, to hook in to AI path finder


#ifndef CRYINCLUDE_EDITOR_UTIL_RULERPATHAGENT_H
#define CRYINCLUDE_EDITOR_UTIL_RULERPATHAGENT_H
#pragma once


#include "IAgent.h"
#include "IPathfinder.h"

class CRuler;
class CRulerPoint;

//! Ruler point helper - Defines a point for the ruler
class CRulerPathAgent
    : public IAIPathAgent
{
public:
    CRulerPathAgent();
    virtual ~CRulerPathAgent();

    void Init(CRuler* pRuler);
    void Render(IRenderer* pRenderer, const ColorF& colour);

    void SetType(uint32 type, const string& name);
    uint32 GetType() const { return m_agentType; }
    const string& GetName() const { return m_agentName; }

    // Request a path
    bool RequestPath(const CRulerPoint& startPoint, const CRulerPoint& endPoint);
    void ClearLastRequest();
    bool HasQueuedPaths() const { return m_bPathQueued; }

    // Get path output
    bool GetLastPathSuccess() const { return m_bLastPathSuccess; }
    float GetLastPathDist() const { return m_fLastPathDist; }

    // IAIPathAgent
    virtual IEntity* GetPathAgentEntity() const;
    virtual const char* GetPathAgentName() const;
    virtual unsigned short GetPathAgentType() const;

    virtual float GetPathAgentPassRadius() const;
    virtual Vec3 GetPathAgentPos() const;
    virtual Vec3 GetPathAgentVelocity() const;

    virtual const AgentMovementAbility& GetPathAgentMovementAbility() const;

    virtual void GetPathAgentNavigationBlockers(NavigationBlockers& blockers, const PathfindRequest* pRequest);

    virtual unsigned int GetPathAgentLastNavNode() const;
    virtual void SetPathAgentLastNavNode(unsigned int lastNavNode);

    virtual void SetPathToFollow(const char* pathName){}
    virtual void SetPathAttributeToFollow(bool bSpline){}

    //Path finding avoids blocker type by radius.
    virtual void SetPFBlockerRadius(int blockerType, float radius){}


    //Can path be modified to use request.targetPoint?  Results are cacheded in request.
    virtual ETriState CanTargetPointBeReached(CTargetPointRequest& request){ETriState garbage = eTS_maybe; return garbage; }

    //Is request still valid/use able
    virtual bool UseTargetPointRequest(const CTargetPointRequest& request){return false; }//??

    virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const{return false; }
    virtual bool GetTeleportPosition(Vec3& teleportPos) const{return false; }
    virtual IPathFollower* GetPathFollower() const { return 0; }
    virtual bool IsPointValidForAgent(const Vec3& pos, uint32 flags) const { return true; }
    //~IAIPathAgent

private:
    bool m_bPathQueued;
    bool m_bLastPathSuccess;
    unsigned int m_LastNavNode;
    float m_fLastPathDist;

    Vec3 m_vStartPoint;
    AgentMovementAbility m_AgentMovementAbility;
    uint32 m_agentType;
    string m_agentName;

    PATHPOINTVECTOR m_path; // caches the path locally to allow multiple requests
};

#endif // CRYINCLUDE_EDITOR_UTIL_RULERPATHAGENT_H
