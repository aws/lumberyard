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

// Description : CPathMarker contains the history of a given moving point (i.e. an entity position)
//               and can return a previous position  interpolated  given the distance from the current point
//               computed along the path



#ifndef CRYINCLUDE_CRYAISYSTEM_PATHMARKER_H
#define CRYINCLUDE_CRYAISYSTEM_PATHMARKER_H
#pragma once

#include <IAgent.h>
#include <vector>


struct CSteeringDebugInfo
{
    std::vector<Vec3> pts;
    Vec3 segStart, segEnd;
};

struct pathStep_t
{
    Vec3 vPoint;
    float fDistance;
    bool bPassed;

    pathStep_t()
        : vPoint(ZERO)
        , fDistance(0.f)
        , bPassed(false)
    {
    }
};

class CPathMarker
{
private:
    std::vector<pathStep_t> m_cBuffer; //using vector instead of deque as a FIFO, optimizing memory allocation and speed
    float m_fStep;
    int m_iCurrentPoint;
    int m_iSize;
    int m_iUsed;
    float m_fTotalDistanceRun;
public:
    CPathMarker(float fMaxDistanceNeeded, float fStep);
    void    Init(const Vec3& vInitPoint, const Vec3& vEndPoint);
    Vec3    GetPointAtDistance(const Vec3& vNewPoint, float fDistance) const;
    Vec3    GetPointAtDistanceFromNewestPoint(float fDistanceFromNewestPoint) const;    // returns a point that is at most fDistanceFromNewestPoint meters away from the head (latest) point (clamped to [head, tail] point)
    Vec3    GetDirectionAtDistance(const Vec3& vTargetPoint, float fDesiredDistance) const;
    Vec3    GetDirectionAtDistanceFromNewestPoint(float fDistanceFromNewestPoint) const;
    Vec3    GetMoveDirectionAtDistance(Vec3& vTargetPoint, float fDesiredDistance,
        const Vec3& vUserPos, float falloff,
        float* alignmentWithPath,
        CSteeringDebugInfo* debugInfo);
    float   GetDistanceToPoint(Vec3& vTargetPoint, Vec3& vMyPoint);
    inline float GetTotalDistanceRun() {return m_fTotalDistanceRun; }
    void    Update(const Vec3& vNewPoint, bool b2D = false);
    size_t  GetPointCount() const { return m_cBuffer.size(); }
    void DebugDraw();
    void Serialize(TSerialize ser);
};

#endif // CRYINCLUDE_CRYAISYSTEM_PATHMARKER_H
