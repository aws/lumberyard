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

#ifndef CRYINCLUDE_CRYAISYSTEM_PATHFOLLOWER_H
#define CRYINCLUDE_CRYAISYSTEM_PATHFOLLOWER_H
#pragma once


#include <IPathfinder.h>

/// This implements path following in a rather simple - and therefore cheap way. One of the
/// results is an ability to generate a pretty accurate prediction of the future entity position,
/// assuming that the entity follows the output of this almost exactly.
class CPathFollower
    : public IPathFollower
{
    /// The control points are attached to the original path, but can be dynamically adjusted
    /// according to the pathRadius. Thus the offsetAmount is scaled by pathRadius when calculating
    /// the actual position
    struct SPathControlPoint
    {
        SPathControlPoint()
            : navType(IAISystem::NAV_UNSET)
            , pos(ZERO)
            , offsetDir(ZERO)
            , offsetAmount(ZERO)
            , customId(0) {}
        SPathControlPoint(IAISystem::ENavigationType navType, const Vec3& pos, const Vec3& offsetDir, float offsetAmount)
            : navType(navType)
            , pos(pos)
            , offsetDir(offsetDir)
            , offsetAmount(offsetAmount)
            , customId(0) {}

        IAISystem::ENavigationType navType;
        Vec3 pos;
        Vec3 offsetDir;
        float offsetAmount;
        uint16 customId;
    };
    typedef std::vector<SPathControlPoint> TPathControlPoints;

    TPathControlPoints m_pathControlPoints;

    /// parameters describing how we follow the path - can be modified as we follow
    PathFollowerParams m_params;

    /// segment that we're on - goes from this point index to the next
    int m_curLASegmentIndex;

    /// position of our lookahead point - the thing we aim towards
    Vec3 m_curLAPos;

    /// used for controlling the acceleration
    float m_lastOutputSpeed;

    INavPath* m_navPath;

    int m_pathVersion;


    Vec3 m_CurPos;
    uint32 m_CurIndex;

    Vec3 GetPathControlPoint(unsigned iPt) const
    {
        return m_pathControlPoints[iPt].pos + m_pathControlPoints[iPt].offsetDir * (m_pathControlPoints[iPt].offsetAmount * m_params.pathRadius);
    }

    /// calculates distance between two points, depending on m_params.use_2D
    float DistancePointPoint(const Vec3 pt1, const Vec3 pt2) const;
    /// calculates distance squared between two points, depending on m_params.use_2D
    float DistancePointPointSq(const Vec3 pt1, const Vec3 pt2) const;

    /// Used internally to create our list of path control points
    void ProcessPath();

    /// calculates a suitable starting state of the lookahead (etc) to match curPos
    void StartFollowing(const Vec3& curPos, const Vec3& curVel);

    /// Used internally to calculate a new lookahead position. const so that it can be used in prediction
    /// where we integrate forwards in time.
    /// curPos and curVel are the position/velocity of the follower
    void GetNewLookAheadPos(Vec3& newLAPos, int& newLASegIndex, const Vec3& curLAPos, int curLASegIndex,
        const Vec3& curPos, const Vec3& curVel, float dt) const;

    /// uses the lookahead and current position to derive a velocity (ultimately using speed control etc) that
    /// should be applied (i.e. passed to physics) to curPos.
    void UseLookAheadPos(Vec3& velocity, bool& reachedEnd, const Vec3 LAPos, const Vec3 curPos, const Vec3 curVel,
        float dt, int curLASegmentIndex, float& lastOutputSpeed) const;

    /// As public version but you pass the current state in
    Vec3 GetPathPointAhead(float dist, float& actualDist, int curLASegmentIndex, Vec3 curLAPos) const;

    /// As public version but you pass the current state in
    float GetDistToEnd(const Vec3* pCurPos, int curLASegmentIndex, Vec3 curLAPos) const;

    // Hiding this virtual method here, so they are only accessible through the IPathFollower interface
    // (i.e. Outside the AISystem module)
    virtual void Release() { delete this; }

    uint32 GetIndex         (const Vec3& pos) const;

public:
    CPathFollower(const PathFollowerParams& params = PathFollowerParams());
    ~CPathFollower();

    virtual void Reset();

    /// This attaches us to a particular path (pass 0 to detach)
    virtual void AttachToPath(INavPath* pNavPath);

    virtual void SetParams(const PathFollowerParams& params) {m_params = params; }

    /// The params may be modified as the path is followed - bear in mind that doing so
    /// will invalidate any predictions
    virtual PathFollowerParams& GetParams() {return m_params; }
    /// Just view the params
    virtual const PathFollowerParams& GetParams() const {return m_params; }

    /// Updates the path following progress, resulting in a move instruction and prediction of future
    /// states if desired.
    virtual bool Update(PathFollowResult& result, const Vec3& curPos, const Vec3& curVel, float dt);

    /// Advances the current state in terms of position - effectively pretending that the follower
    /// has gone further than it has.
    virtual void Advance(float distance);

    /// Returns the distance from the lookahead to the end, plus the distance from the position passed in
    /// to the LA if pCurPos != 0
    virtual float GetDistToEnd(const Vec3* pCurPos) const;

    /// Returns the distance along the path from the current look-ahead position to the
    /// first smart object path segment. If there's no path, or no smart objects on the
    /// path, then std::numeric_limits<float>::max() will be returned
    virtual float GetDistToSmartObject() const;
    virtual float GetDistToNavType(IAISystem::ENavigationType navType) const;
    virtual float GetDistToCustomNav(const TPathControlPoints& controlPoints, uint32 curLASegmentIndex, const Vec3& curLAPos) const;
    /// Returns a point on the path some distance ahead. actualDist is set according to
    /// how far we looked - may be less than dist if we'd reach the end
    virtual Vec3 GetPathPointAhead(float dist, float& actualDist) const;

    virtual void Draw(const Vec3& drawOffset = ZERO) const;


    virtual void Serialize(TSerialize ser);

    // Checks ability to walk along a piecewise linear path starting from the current position
    // (useful for example when animation would like to deviate from the path)
    virtual bool CheckWalkability(const Vec2* path, const size_t length) const;

    // Can the pathfollower cut corners if there is space to do so? (default: true)
    virtual bool GetAllowCuttingCorners() const;

    // Sets whether or not the pathfollower is allowed to cut corners if there is space to do so. (default: true)
    virtual void SetAllowCuttingCorners(const bool allowCuttingCorners);
};

#endif // CRYINCLUDE_CRYAISYSTEM_PATHFOLLOWER_H
