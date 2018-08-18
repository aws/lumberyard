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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATH_H
#define CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATH_H
#pragma once

#include "Cry_Geo.h"

namespace FlyHelpers
{
    class Path
    {
    public:
        Path();
        ~Path();

        void Clear();

        void AddPoint(const Vec3& point);

        const Vec3& GetPoint(const size_t pointIndex) const;

        Lineseg GetSegment(const size_t segmentIndex) const;
        float GetSegmentLength(const size_t segmentIndex) const;

        size_t GetPointCount() const;
        size_t GetSegmentCount() const;

        void MakeLooping();

        float GetPathDistanceToPoint(const size_t pointIndex) const;
        float GetTotalPathDistance() const;

    private:
        std::vector< Vec3 > m_points;
        std::vector< float > m_segmentDistances;
        std::vector< float > m_pathLengthsToPoint;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATH_H
