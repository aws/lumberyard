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

#ifndef CRYINCLUDE_CRYCOMMON_ICLUSTERDETECTOR_H
#define CRYINCLUDE_CRYCOMMON_ICLUSTERDETECTOR_H
#pragma once


typedef uint32 ClusterPointProperties;
typedef uint32 ClusterId;

struct ClusterPoint
{
    ClusterPoint()
        : pos(ZERO)
        , clusterId(~0)
        , pointId(0)
    {
    }

    ClusterPoint(const uint32 _id, const Vec3& _pos)
        : pos(_pos)
        , clusterId(~0)
        , pointId(_id)
    {
    }

    ClusterPoint(const ClusterPoint& _clusterPoint)
        : pos(_clusterPoint.pos)
        , clusterId(_clusterPoint.clusterId)
        , pointId(_clusterPoint.pointId)
    {
    }

    Vec3      pos;
    ClusterId clusterId;
    // This is needed for the requesters to be able
    // to match the point with the entity they have
    uint32    pointId;
};

struct IClusterRequest
{
    typedef Functor1<IClusterRequest*> Callback;
    // <interfuscator:shuffle>
    virtual ~IClusterRequest() {};
    virtual void SetNewPointInRequest(const uint32 pointId, const Vec3& location) = 0;
    virtual void SetCallback(Callback callback) = 0;
    virtual size_t GetNumberOfPoint() const = 0;
    virtual void SetMaximumSqDistanceAllowedPerCluster(float maxDistance) = 0;
    virtual const ClusterPoint* GetPointAt(const size_t pointIndex) const = 0;
    virtual size_t GetTotalClustersNumber() const = 0;
    // </interfuscator:shuffle>
};


struct IClusterDetector
{
    typedef unsigned int ClusterRequestID;
    typedef std::pair<ClusterRequestID, IClusterRequest*> IClusterRequestPair;
    // <interfuscator:shuffle>
    virtual ~IClusterDetector() {}
    virtual IClusterRequestPair CreateNewRequest() = 0;
    virtual void QueueRequest(const ClusterRequestID requestId) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ICLUSTERDETECTOR_H
