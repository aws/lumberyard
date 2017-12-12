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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphicsReflectContext
{
    //! Provides reflection for shadow controls
    class Shadows
    {
    public:
        AZ_TYPE_INFO(Shadows, "{E18D6F42-E39C-4769-9286-0941611B7010}");
        static void Reflect(AZ::ReflectContext* context);

    private:
        static void RecomputeStaticShadows(const AZ::Aabb& bounds, float nextCascadeScale);
    };
}
