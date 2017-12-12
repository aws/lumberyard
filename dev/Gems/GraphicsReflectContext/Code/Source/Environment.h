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

#include <LmbrCentral/Rendering/MaterialHandle.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphicsReflectContext
{
    //! Provides reflection for environment controls
    class Environment
    {
    public:
        AZ_TYPE_INFO(Environment, "{FBFF1A05-BD9C-4196-982B-A6F28ADBAE3C}");

        static void Reflect(AZ::ReflectContext* context);

    private:
        static void SetSkyboxMaterial(LmbrCentral::MaterialHandle material);
        static void SetSkyboxAngle(float angle);
        static void SetSkyboxStretch(float amount);
    };
}
