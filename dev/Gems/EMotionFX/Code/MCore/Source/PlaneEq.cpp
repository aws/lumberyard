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

// include required headers
#include "PlaneEq.h"

namespace MCore
{
    // clip points against the plane
    bool PlaneEq::Clip(const Array<Vector3>& pointsIn, Array<Vector3>& pointsOut) const
    {
        int32 numPoints = pointsIn.GetLength();

        MCORE_ASSERT(&pointsIn != &pointsOut);
        MCORE_ASSERT(numPoints >= 2);

        int32       vert1       = numPoints - 1;
        float       firstDist   = CalcDistanceTo(pointsIn[vert1]);
        float       nextDist    = firstDist;
        bool        firstIn     = (firstDist >= 0.0f);
        bool        nextIn      = firstIn;

        pointsOut.Clear();

        if (numPoints == 2)
        {
            numPoints = 1;
        }

        for (int32 vert2 = 0; vert2 < numPoints; vert2++)
        {
            float   dist = nextDist;
            bool    in   = nextIn;

            nextDist = CalcDistanceTo(pointsIn[vert2]);
            nextIn   = (nextDist >= 0.0f);

            if (in)
            {
                pointsOut.Add(pointsIn[vert1]);
            }

            if ((in != nextIn) && (dist != 0.0f) && (nextDist != 0.0f))
            {
                Vector3 dir = (pointsIn[vert2] - pointsIn[vert1]);

                float frac = dist / (dist - nextDist);
                if ((frac > 0.0f) && (frac < 1.0f))
                {
                    pointsOut.Add(pointsIn[vert1] + frac * dir);
                }
            }

            vert1 = vert2;
        }

        //if (numPoints == 1)
        //  return (pointsOut.GetLength() > 1);

        return (pointsOut.GetLength() > 1);
    }


    // clip a set of vectors <points> to this plane
    bool PlaneEq::Clip(Array<Vector3>& points) const
    {
        Array<Vector3> pointsOut;
        if (Clip(points, pointsOut))
        {
            points = pointsOut;
            return true;
        }

        return false;
    }
}   // namespace MCore
