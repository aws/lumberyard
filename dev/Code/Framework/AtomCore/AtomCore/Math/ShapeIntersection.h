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
        /// Tests to see if Arg1 overlaps Arg2. Symmetric.
        bool Overlaps(Sphere sphere, Aabb aabb);
        bool Overlaps(Sphere sphere, Frustum frustum);
        bool Overlaps(Sphere sphere, Obb obb);
        bool Overlaps(Sphere sphere, Plane plane);
        bool Overlaps(Sphere sphere1, Sphere sphere2);
        bool Overlaps(Frustum frustum, Plane plane);
        bool Overlaps(Frustum frustum, Aabb aabb);
        bool Overlaps(Frustum frustum1, Frustum frustum2);


        /// Tests to see if Arg1 contains Arg2. Non Symmetric.
        bool Contains(Sphere sphere, Aabb aabb);
        bool Contains(Sphere sphere, Vector3 point);
        bool Contains(Sphere sphere, Frustum frustum);
        bool Contains(Sphere sphere, Obb obb);
        bool Contains(Sphere sphere1, Sphere sphere2);

        bool Contains(Frustum frustum, Aabb aabb);
        bool Contains(Frustum frustum, Sphere sphere);
        bool Contains(Frustum frustum, Obb obb);
        bool Contains(Frustum frustum, Vector3 point);
        bool Contains(Frustum frustum1, Frustum frustum2);

    }//ShapeIntersection
}//AZ