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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWERHELPERS_H
#define CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWERHELPERS_H
#pragma once

#include "Cry_Geo.h"
#include "Cry_GeoDistance.h"

namespace FlyHelpers
{
    Path CreatePathSubset(const Path& sourcePath, const PathLocation& start, const PathLocation& end)
    {
        assert(start.segmentIndex <= end.segmentIndex);

        Path result;

        result.AddPoint(start.CalculatePathPosition(sourcePath));
        const size_t pointCount = end.segmentIndex - start.segmentIndex;
        for (size_t i = 0; i < pointCount; ++i)
        {
            const size_t index = start.segmentIndex + i + 1;
            const Vec3& point = sourcePath.GetPoint(index);
            result.AddPoint(point);
        }
        result.AddPoint(end.CalculatePathPosition(sourcePath));

        return result;
    }


    PathLocation FindClosestPathLocation(const Path& path, const Vec3& position)
    {
        PathLocation pathLocationOut;

        float closestDistanceSquared = FLT_MAX;

        const size_t segmentCount = path.GetSegmentCount();
        for (size_t i = 0; i < segmentCount; ++i)
        {
            const Lineseg segment = path.GetSegment(i);

            float deltaSegment = 0;
            const float distanceSquared = Distance::Point_LinesegSq(position, segment, deltaSegment);
            if (distanceSquared < closestDistanceSquared)
            {
                pathLocationOut.segmentIndex = i;
                pathLocationOut.normalizedSegmentPosition = deltaSegment;
                closestDistanceSquared = distanceSquared;
            }
        }

        return pathLocationOut;
    }


    PathLocation TracePathForward(const Path& path, const PathLocation& start, const float distance)
    {
        float remainingDistance = distance;
        remainingDistance -= start.CalculateDistanceToNextPathPoint(path);
        if (remainingDistance <= 0)
        {
            PathLocation result;
            result.segmentIndex = start.segmentIndex;
            const float distanceAlongSegment = distance;
            const float normalizedDistanceDelta = distance / path.GetSegmentLength(start.segmentIndex);
            result.normalizedSegmentPosition = start.normalizedSegmentPosition + normalizedDistanceDelta;

            return result;
        }

        const size_t segmentCount = path.GetSegmentCount();
        size_t currentSegment = start.segmentIndex + 1;
        while (currentSegment < segmentCount)
        {
            const float segmentLength = path.GetSegmentLength(currentSegment);
            if (remainingDistance < segmentLength)
            {
                break;
            }
            remainingDistance -= segmentLength;
            ++currentSegment;
        }

        if (currentSegment < segmentCount)
        {
            PathLocation result;
            result.segmentIndex = currentSegment;
            result.normalizedSegmentPosition = remainingDistance / path.GetSegmentLength(currentSegment);

            return result;
        }
        else
        {
            PathLocation result;
            result.segmentIndex = path.GetSegmentCount() - 1;
            result.normalizedSegmentPosition = 1.0f;

            return result;
        }
    }

    PathLocation TracePathForwardLooping(const Path& path, const PathLocation& start, const float distance)
    {
        float remainingDistance = distance;
        remainingDistance -= start.CalculateDistanceToNextPathPoint(path);
        if (remainingDistance <= 0)
        {
            PathLocation result;
            result.segmentIndex = start.segmentIndex;
            const float distanceAlongSegment = distance;
            const float normalizedDistanceDelta = distance / path.GetSegmentLength(start.segmentIndex);
            result.normalizedSegmentPosition = start.normalizedSegmentPosition + normalizedDistanceDelta;

            return result;
        }

        const size_t pathSegmentCount = path.GetSegmentCount();
        size_t currentSegment = (start.segmentIndex + 1) % pathSegmentCount;
        while (true)
        {
            const float segmentLength = path.GetSegmentLength(currentSegment);
            if (remainingDistance < segmentLength)
            {
                break;
            }
            remainingDistance -= segmentLength;
            currentSegment = (currentSegment + 1) % pathSegmentCount;
        }

        PathLocation result;
        result.segmentIndex = currentSegment;
        result.normalizedSegmentPosition = remainingDistance / path.GetSegmentLength(currentSegment);

        return result;
    }


    PathLocation GetReversePathLocation(const Path& path, const PathLocation& pathLocation)
    {
        PathLocation reversedPathLocation;
        const size_t pathSegmentCount = path.GetSegmentCount();
        reversedPathLocation.segmentIndex = pathSegmentCount - 1 - pathLocation.segmentIndex;
        reversedPathLocation.normalizedSegmentPosition = (1.0f - pathLocation.normalizedSegmentPosition);

        return reversedPathLocation;
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWERHELPERS_H
