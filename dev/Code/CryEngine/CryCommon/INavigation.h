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

// Description : AI Navigation Interface.


#ifndef CRYINCLUDE_CRYCOMMON_INAVIGATION_H
#define CRYINCLUDE_CRYCOMMON_INAVIGATION_H
#pragma once


#include <IAISystem.h> // <> required for Interfuscator

struct IAIObject;

struct INavigation
{
    enum EIFMode
    {
        IF_AREASBOUNDARIES, IF_AREAS, IF_BOUNDARIES
    };

    // <interfuscator:shuffle>
    virtual ~INavigation() {}

    virtual uint32 GetPath(const char* szPathName, Vec3* points, uint32 maxpoints) const = 0;
    virtual float GetNearestPointOnPath(const char* szPathName, const Vec3& vPos, Vec3& vResult, bool& bLoopPath, float& totalLength) const = 0;
    virtual void GetPointOnPathBySegNo(const char* szPathName, Vec3& vResult, float segNo) const = 0;
    virtual bool IsSegmentValid(IAISystem::tNavCapMask navCap, float rad, const Vec3& posFrom, Vec3& posTo, IAISystem::ENavigationType& navTypeFrom) const = 0;

    /// if there's intersection vClosestPoint indicates the intersection point, and the edge normal
    /// is optionally returned. If bForceNormalOutwards is set then in the case of forbidden
    /// boundaries this normal is chosen to point (partly) towards vStart.
    /// nameToSkip can optionally point to a string indicating a forbidden area area to not check
    /// mode indicates if areas and/or boundaries should be checked
    virtual bool IntersectsForbidden(const Vec3& vStart, const Vec3& vEnd, Vec3& vClosestPoint, const string* nameToSkip = 0, Vec3* pNormal = NULL, INavigation::EIFMode mode = INavigation::IF_AREASBOUNDARIES, bool bForceNormalOutwards = false) const = 0;

    /// Returns true if it is impossible (assuming path finding is OK) to get from start
    /// to end without crossing a forbidden boundary (except for moving out of a
    /// forbidden region).
    virtual bool IsPathForbidden(const Vec3& start, const Vec3& end) const = 0;

    /// Returns true if a point is inside forbidden boundary/area edge or close to its edge
    virtual bool IsPointForbidden(const Vec3& pos, float tol, Vec3* pNormal = 0) const = 0;

    /// Get the best point outside any forbidden region given the input point,
    /// and optionally a start position to stay close to
    virtual Vec3 GetPointOutsideForbidden(Vec3& pos, float distance, const Vec3* startPos = 0) const = 0;

    /// Returns nearest designer created path/shape.
    /// The devalue parameter specifies how long the path will be unusable by others after the query.
    /// If useStartNode is true the start point of the path is used to select nearest path instead of the nearest point on path.
    virtual const char* GetNearestPathOfTypeInRange(IAIObject* requester, const Vec3& pos, float range, int type, float devalue, bool useStartNode) = 0;

    /// Modifies the additional cost multiplier of a named cost nav modifier. If factor < 0 then
    /// the cost is made infinite. If >= 0 then the cost is multiplied by 1 + factor.
    /// The original value gets reset when leaving/entering game mode etc.
    virtual void ModifyNavCostFactor(const char* navModifierName, float factor) = 0;

    /// returns the names of the region files generated during volume generation
    virtual void GetVolumeRegionFiles(const char* szLevel, const char* szMission, DynArray<CryStringT<char> >& filenames) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_INAVIGATION_H
