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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVPATH_H
#define CRYINCLUDE_CRYAISYSTEM_NAVPATH_H
#pragma once

#include "IAgent.h"
#include "SerializeFwd.h"
#include "AILog.h"
#include "PathObstacles.h"

#include "Cry_Color.h"

#include <list>

#include <IPathfinder.h>

#include <AzCore/std/smart_ptr/make_shared.h>

//====================================================================
// CNavPath
//====================================================================
class CNavPath
    : public INavPath
{
public:
    CNavPath();
    virtual ~CNavPath();

    NavigationMeshID GetMeshID() const;

    /// returns the path version - this is incremented every time the
    /// path is modified (wrapping around is unlikely to be a problem!)
    int GetVersion() const {return m_version.v; }

    /// Set the path version, should not normally be used.
    virtual void SetVersion(int v) override {m_version.v = v; }

    /// The original parameters used to generate the path will be stored
    virtual void SetParams(const SNavPathParams& params) override {m_params = params; }
    virtual const SNavPathParams& GetParams() const override {return m_params; }
    virtual SNavPathParams& GetParams() override { return m_params; }

    void Draw(const Vec3& drawOffset = Vec3_Zero) const;
    void Dump(const char* name) const;
    // gets distance from the current path point to the end of the path - needs to
    // be told if this path should be traversed as if it's in 2D
    virtual float GetPathLength(bool b2D) const override;

    /// Add a point to the front of the path
    void PushFront(const PathPointDescriptor& newPathPoint, bool force = false);

    /// Add a point to the back of the path
    void PushBack(const PathPointDescriptor& newPathPoint, bool force = false);

    /// Clear all the path. dbgString can be used to indicate the calling context
    /// If this path is owned by an object that might need to free smart-object
    /// resources when the path is cleared, then clearing the path should be done
    /// via CPipeUser::ClearPath()
    void    Clear(const char* dbgString);

    /// removes the current previous point (shifting all points) and sets nextPathPoint to the
    /// new next path point (if it's available). If there are no points remaining, then returns false,
    /// and the path will be left with just the path end
    bool Advance(PathPointDescriptor& nextPathPoint);

    /// Indicates if this path finishes at approx the location that was requested, or
    /// if it uses some other end position (e.g. from a partial path)
    bool GetPathEndIsAsRequested() const {return m_pathEndIsAsRequested; }
    void SetPathEndIsAsRequested(bool val) {m_pathEndIsAsRequested = val; }

    /// Returns true if the path contains so few points it's not useable (returns false if 2 or more)
    bool    Empty() const;
    /// end of the path - returns 0 if empty
    const PathPointDescriptor* GetLastPathPoint() const { return m_pathPoints.empty() ? 0 : &m_pathPoints.back(); }
    /// previous path point - returns 0 if not available
    const PathPointDescriptor* GetPrevPathPoint() const { return m_pathPoints.empty() ? 0 : &m_pathPoints.front(); }
    /// next path point - returns 0 if not available
    const PathPointDescriptor* GetNextPathPoint() const { return m_pathPoints.size() > 1 ? &(*(++m_pathPoints.begin())) : 0; }
    /// next but 1 path point - returns 0 if not available
    const PathPointDescriptor* GetNextNextPathPoint() const { return m_pathPoints.size() > 2 ? &(*(++(++m_pathPoints.begin()))) : 0; }

    /// Gets the position of next path point. Returns defaultPos if the path is empty
    const Vec3& GetNextPathPos(const Vec3& defaultPos = Vec3_Zero) const {const PathPointDescriptor* ppd = GetNextPathPoint(); return ppd ? ppd->vPos : defaultPos; }
    /// Gets the end of the path. Returns defaultPos if the path is empty
    const Vec3& GetLastPathPos(const Vec3& defaultPos = Vec3_Zero) const {const PathPointDescriptor* ppd = GetLastPathPoint(); return ppd ? ppd->vPos : defaultPos; }

    /// Returns the point that is dist ahead on the path, extrapolating if necessary.
    /// Fills nextPointType with the nav type of the next point in the path from the given distance.
    /// Returns false if there is no path points at all
    bool GetPosAlongPath(Vec3& posOut, float dist = 0.f, bool b2D = false, bool bExtrapolateBeyondEnd = false, IAISystem::ENavigationType* pNextPointType = NULL) const;

    /// Returns the distance from pos to the closest point on the path, looking a maximum of dist ahead.
    /// Also the closest point, and the distance it is along the path are returned.
    /// Returns -1 if there's no path.
    /// Note that this starts from the start of the path, NOT the current position
    float GetDistToPath(Vec3& pathPosOut, float& distAlongPathOut, const Vec3& pos, float dist, bool twoD) const;

    /// Returns the closest direction to the current path and a given point.
    void GetDirectionToPathFromPoint(const Vec3& point, Vec3& dirOut) const;

    /// Returns the distance along the path from the current look-ahead position to the
    /// first smart object path segment. If there's no path, or no smart objects on the
    /// path, then std::numeric_limits<float>::max() will be returned
    float GetDistToSmartObject(bool b2D) const;

    /// Returns a pointer to a smartobject nav data if the last path point exists and
    /// its traversal method is one of the animation methods.
    PathPointDescriptor::SmartObjectNavDataPtr  GetLastPathPointAnimNavSOData() const;
    const PathPointDescriptor::OffMeshLinkData* GetLastPathPointMNNSOData() const;
    /// Sets the value of the previous path point, if it exists
    void SetPreviousPoint(const PathPointDescriptor& previousPoint);

    const TPathPoints& GetPath() const { return m_pathPoints; }

    virtual void SetPathPoints(const TPathPoints& points) override { m_pathPoints = points; ++m_version.v; }

    /// Returns the max distance we can find from the start position and any point in the path
    float GetMaxDistanceFromStart() const;

    /// Returns AABB containing all the path between the current position and dist ahead
    AABB GetAABB(float dist) const;

    /// Calculates the path position and direction and (approx) radius of curvature distAhead beyond the current iterator
    /// position. If the this position would be beyond the path end then it extrapolates.
    /// If scaleOutputWithDist is set then it scales the lowestPathDotOut according to how far along the
    /// path it is compared to distAhead
    /// Returns false if there's not enough path to do the calculation - true if all is
    /// well.
    bool GetPathPropertiesAhead(float distAhead, bool twoD, Vec3& posOut, Vec3& dirOut,
        float* invROut, float& lowestPathDotOut, bool scaleOutputWithDist) const;

    /// Serialise the current request
    void Serialize(TSerialize ser);

    const Vec3& GetEndDir() const {return m_endDir; }
    void    SetEndDir(const Vec3& endDir) {m_endDir = endDir; }

    /// Obtains a new target direction and approximate distance to the path end (for speed control). Also updates
    /// the current position along the path.
    ///
    /// isResolvingSticking indicates if the algorithm is trying to resolve agent sticking.
    /// distToPathOut indicates the approximate perpendicular distance the agent is from the path.
    /// pathDirOut, pathAheadPosOut and pathAheadDirOut are the path directions/position at the current
    /// and look-ahead positions
    /// currentPos/Vel are the tracer's current position/velocity.
    /// lookAhead is the look-ahead distance for smoothing the path.
    /// pathRadius is the radius of the core part of the path - the narrower the
    /// path the more constrained the lookahead will be at corners
    /// dt is the time since last update (mainly used to detect the tracer
    /// getting stuck).
    /// resolveSticking indicates if getting stuck should be handled - handling is
    /// done by limiting the lookahead which can confuse vehicles that use
    /// manouevering.
    /// twoD indicates if this should work in 2D
    /// Return value indicates if we're still tracing the path - false means we reached the end
    bool UpdateAndSteerAlongPath(Vec3& dirOut, float& distToEndOut, float& distToPathOut, bool& isResolvingSticking,
        Vec3& pathDirOut, Vec3& pathAheadDirOut, Vec3& pathAheadPosOut, Vec3 currentPos, const Vec3& currentVel,
        float lookAhead, float pathRadius, float dt, bool resolveSticking, bool twoD);

    /// Iterates over the path and fills the navigation methods for the path segments.
    /// Cuts the path at the path point where the next navSO animation should be played.
    /// The trace goalop will follow the path and play the animation at the target location
    /// and then regenerate the path to the target.
    void PrepareNavigationalSmartObjectsForMNM(IAIPathAgent* pAgent);
    /// Reinstates the path removed during the last PrepareNavigationalSmartObjects()
    void ResurrectRemainingPath();

    /// trim the path to specified length.
    void TrimPath(float length, bool twoD);

    /// after the path is being cut on smart object this function
    /// can tell the length of the discarded path section
    float GetDiscardedPathLength() const { return m_fDiscardedPathLength; }

    /// Adjusts the path (or what's left of it) to steer it around the obstacles passed in.
    /// These obstacles should already have been expanded if necessary to allow for pass
    /// radius. Returns false if there was no way to adjust the path (in which case the path
    /// may be partially adjusted)
    bool AdjustPathAroundObstacles(const CPathObstacles& obstacles, IAISystem::tNavCapMask navCapMask);
    bool AdjustPathAroundObstacles(const Vec3& currentpos, const AgentMovementAbility& movementAbility);

    /// Advances path state until it represents agentPos. If allowPathToFinish = false then
    /// it won't allow the path to finish
    /// Returns the remaining path length
    float UpdatePathPosition(Vec3 agentPos, float pathLookahead, bool twoD, bool allowPathToFinish);

    /// Calculates a target position which is lookAhead along the path,
    Vec3 CalculateTargetPos(Vec3 agentPos, float lookAhead, float minLookAheadAlongPath, float pathRadius, bool twoD) const;
    /// Returns true if the path can be modified to use request.targetPoint, and byproducts
    /// of the test are cached in request.
    ETriState CanTargetPointBeReached(CTargetPointRequest& request, const CAIActor* pAIActor, bool twoD) const;
    /// Returns true if the request is still valid/can be used, false otherwise.
    bool UseTargetPointRequest(const CTargetPointRequest& request, CAIActor* pAIActor, bool twoD);

private:
    // Hiding these virtual methods here, so they are only accessible through the INavPath interface
    // (i.e. Outside the AISystem module)
    virtual void Release() { delete this; };
    virtual void CopyTo(INavPath* pRecipient) const
    {
        *static_cast<CNavPath*>(pRecipient) = *this;
    }
    virtual INavPathPtr Clone() const
    {
        return AZStd::make_shared<CNavPath>(*this);
    };

    /// Makes the path avoid a single obstacle - return false if this is impossible.
    /// It only works on m_pathPoints.
    bool AdjustPathAroundObstacle(const CPathObstacle& obstacle, IAISystem::tNavCapMask navCapMask);
    bool AdjustPathAroundObstacleShape2D(const SPathObstacleShape2D& obstacle, IAISystem::tNavCapMask navCapMask);
    bool CheckPath(const TPathPoints& pathList, float radius) const;
    void MovePathEndsOutOfObstacles(const CPathObstacles& obstacles);

    float GetPathDeviationDistance(Vec3& deviationOut, float criticalDeviation, bool twoD);

    TPathPoints m_pathPoints;
    Vec3        m_endDir;

    struct SDebugLine
    {
        Vec3 P0, P1;
        ColorF  col;
    };
    void DebugLine(const Vec3& P0, const Vec3& P1, const ColorF& col) const;
    mutable std::list<SDebugLine>           m_debugLines;

    struct SDebugSphere
    {
        Vec3    pos;
        float r;
        ColorF    col;
    };
    void DebugSphere(const Vec3& P, float r, const ColorF& col) const;
    mutable std::list<SDebugSphere> m_debugSpheres;

    /// the current position on the path between previous and current point
    float m_currentFrac;

    /// How long we've been stuck for - used to reduce the lookAhead
    float m_stuckTime;

    /// Indicates if this path finishes at approx the location that was requested, or
    /// if it uses some other end position (e.g. from a partial path)
    bool m_pathEndIsAsRequested;

    /// parameters used to generate the request - in case the requester wants to regenerate
    /// it
    SNavPathParams m_params;

    /// the length of the path segment discarded after a smart object
    float m_fDiscardedPathLength;
    TPathPoints m_remainingPathPoints;  // Path points that were discarded (saved to avoid regenerating the path)

    CPathObstacles m_obstacles; // Marcio: Crysis2

    /// Lets other things track when we've changed
    /// also we change version when copied!
    struct SVersion
    {
        int v;
        void operator=(const SVersion& other) {v = other.v + 1; }
        SVersion()
            : v(-1) {}
        void Serialize(TSerialize ser);
    };
    SVersion m_version;
};


#endif // CRYINCLUDE_CRYAISYSTEM_NAVPATH_H
