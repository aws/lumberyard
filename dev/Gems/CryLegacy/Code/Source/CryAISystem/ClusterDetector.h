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

// Description : This is the actual class that implements the detection of
//               the different clusters in the world space
//               This suystem only knows about the presence of points into
//               the space and he group them into clusters


#ifndef CRYINCLUDE_CRYAISYSTEM_CLUSTERDETECTOR_H
#define CRYINCLUDE_CRYAISYSTEM_CLUSTERDETECTOR_H
#pragma once

#include <IClusterDetector.h>

struct Cluster
{
    typedef float DistanceSqFromCenter;
    typedef std::pair<DistanceSqFromCenter, Vec3> DistancePointPair;

    Cluster()
        : centerPos(ZERO)
        , numberOfElements(0)
        , pointsWeight(1.0f)
    {
        pointAtMaxDistanceSqFromCenter = DistancePointPair(FLT_MIN, Vec3(ZERO));
    }

    Cluster(const Vec3& _pos)
        : centerPos(_pos)
        , numberOfElements(0)
        , pointsWeight(1.0f)
    {
        pointAtMaxDistanceSqFromCenter = DistancePointPair(FLT_MIN, Vec3(ZERO));
    }

    Vec3  centerPos;
    uint8 numberOfElements;
    float pointsWeight;
    DistancePointPair pointAtMaxDistanceSqFromCenter;
};

struct ClusterRequest
    : public IClusterRequest
{
    enum EClusterRequestState
    {
        eRequestState_Created,
        eRequestState_ReadyToBeProcessed,
    };

    ClusterRequest();

    virtual void SetNewPointInRequest(const uint32 pointId, const Vec3& location);
    virtual size_t GetNumberOfPoint() const;
    virtual void SetMaximumSqDistanceAllowedPerCluster(float maxDistanceSq);
    virtual void SetCallback(Callback callback);
    virtual const ClusterPoint* GetPointAt(const size_t pointIndex) const;
    virtual size_t GetTotalClustersNumber() const;

    typedef std::vector<ClusterPoint> PointsList;
    PointsList                m_points;
    Callback                  m_callback;
    EClusterRequestState      m_state;
    float                     m_maxDistanceSq;
    size_t                    m_totalClustersNumber;
};


class ClusterDetector
    : public IClusterDetector
{
    typedef std::vector<Cluster> ClustersArray;

    struct CurrentRequestState
    {
        CurrentRequestState()
            : pCurrentRequest(NULL)
            , currentRequestId(~0)
            , aabbCenter(ZERO)
            , spawningPositionForNextCluster(ZERO)
        {
        }

        void Reset();

        ClusterRequest*  pCurrentRequest;
        ClusterRequestID currentRequestId;
        ClustersArray    clusters;
        Vec3             aabbCenter;
        Vec3             spawningPositionForNextCluster;
    };

public:

    enum EStatus
    {
        eStatus_Waiting = 0,
        eStatus_ClusteringInProgress,
        eStatus_ClusteringCompleted,
    };

    ClusterDetector();

    virtual IClusterRequestPair CreateNewRequest();
    virtual void QueueRequest(const ClusterRequestID requestId);

    void Update(float frameDeltaTime);
    void Reset();


    typedef std::pair<ClusterRequestID, ClusterRequest> ClusterRequestPair;

private:

    bool InitializeNextRequest();

    ClusterRequestID GenerateUniqueClusterRequestId();
    ClusterRequestPair* ChooseRequestToServe();
    size_t CalculateRuleOfThumbClusterSize(const size_t numberOfPoints);
    void InitializeClusters();
    Vec3 CalculateInitialClusterPosition(const size_t clusterIndex, const float spreadAngleUnit) const;

    // Steps
    bool ExecuteClusteringStep();
    void UpdateClustersCenter();
    void ResetClustersElements();
    bool AddNewCluster();
    bool IsNewClusterNeeded();

private:
    typedef std::list<ClusterRequestPair> RequestsList;
    CurrentRequestState m_internalState;
    ClusterRequestID    m_nextUniqueRequestID;
    RequestsList        m_requests;
    EStatus             m_status;
};

#endif // CRYINCLUDE_CRYAISYSTEM_CLUSTERDETECTOR_H
