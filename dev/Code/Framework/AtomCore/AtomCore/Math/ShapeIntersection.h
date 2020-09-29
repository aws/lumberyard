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
#pragma once

#include <AtomCore/Math/Sphere.h>
#include <AtomCore/Math/Frustum.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace ShapeIntersection
    {
        inline bool IntersectThreePlanes(AZ::Plane p1, AZ::Plane p2, AZ::Plane p3, AZ::Vector3& outP)
        {
            //From Foundations of Game Development Vol 1 by Eric Lengyel (Ch. 3)
            AZ::Vector3 n1CrossN2 = p1.GetNormal().Cross(p2.GetNormal());
            float det = n1CrossN2.Dot(p3.GetNormal());
            if (AZ::GetAbs(det) > std::numeric_limits<float>::min())
            {
                AZ::Vector3 n3CrossN2 = p3.GetNormal().Cross(p2.GetNormal());
                AZ::Vector3 n1CrossN3 = p1.GetNormal().Cross(p3.GetNormal());

                outP = (n3CrossN2 * p1.GetDistance() + n1CrossN3 * p2.GetDistance() - n1CrossN2 * p3.GetDistance()) / det;
                return true;
            }

            return false;
        }

        enum PlaneClassification
        {
            InFront,
            Intersects,
            Behind
        };
        // Tests to see which halfspace Arg2 is in relative to Arg1
        PlaneClassification Classify(Plane plane, Sphere sphere);
        PlaneClassification Classify(Plane plane, Obb obb);

        enum FrustumClassification
        {
            Inside,
            Touches,
            Outside
        };
        // Tests to see how Arg2 relates to frustum Arg1
        FrustumClassification Classify(Frustum frustum, Sphere sphere);

        // Tests to see if Arg1 overlaps Arg2. Symmetric.
        bool Overlaps(Sphere sphere, Aabb aabb);
        bool Overlaps(Sphere sphere, Frustum frustum);
        bool Overlaps(Sphere sphere, Plane plane);
        bool Overlaps(Sphere sphere1, Sphere sphere2);
        bool Overlaps(Frustum frustum, Aabb aabb);
        bool Overlaps(Frustum frustum, Obb obb);

        // Tests to see if Arg1 contains Arg2. Non Symmetric.
        bool Contains(Sphere sphere, Aabb aabb);
        bool Contains(Sphere sphere, Vector3 point);
        bool Contains(Sphere sphere1, Sphere sphere2);

        bool Contains(Frustum frustum, Aabb aabb);
        bool Contains(Frustum frustum, Sphere sphere);
        bool Contains(Frustum frustum, Vector3 point);

    }//ShapeIntersection
}//AZ
