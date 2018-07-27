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

#include "CryLegacy_precompiled.h"
#include "ClusterDetector.h"

ClusterRequest::ClusterRequest()
    : m_state(eRequestState_Created)
    , m_maxDistanceSq(FLT_MAX)
    , m_totalClustersNumber(0)
{
    m_points.reserve(16);
}

void ClusterRequest::SetNewPointInRequest(const uint32 pointId, const Vec3& location)
{
    m_points.push_back(ClusterPoint(pointId, location));
}

size_t ClusterRequest::GetNumberOfPoint() const
{
    return m_points.size();
}

void ClusterRequest::SetCallback(Callback callback)
{
    m_callback = callback;
}

void ClusterRequest::SetMaximumSqDistanceAllowedPerCluster(float maxDistanceSq)
{
    m_maxDistanceSq = maxDistanceSq;
}

const ClusterPoint* ClusterRequest::GetPointAt(const size_t pointIndex) const
{
    return (pointIndex < m_points.size()) ? &(m_points[pointIndex]) : NULL;
}

size_t ClusterRequest::GetTotalClustersNumber() const
{
    return m_totalClustersNumber;
}
