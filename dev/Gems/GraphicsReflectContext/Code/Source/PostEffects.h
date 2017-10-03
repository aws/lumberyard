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

namespace AZ
{
    class SerializeContext;
    class BehaviorContext;
}

namespace GraphicsReflectContext
{
    //classes for post effects parameters
    class PostEffects
    {
    public:
        AZ_TYPE_INFO(PostEffects, "{4DF6AE85-E113-4B7C-9BDC-BD07A457B057}")

        static void SetWaterDropletsParams(bool enable, float amount);
        static void SetDepthOfFieldParams(bool Active, float focusDistance, float FocusRange, float blurAmount, float maxCoC, float centerWeight);
        static void SetFilterBlurParams(bool enable, float amount);
        static void SetColorCorrectionParams(bool enable, float colorC, float colorM, float colorY, float colorK, float brightness, float contrast, float saturation, float colorHue);

        static void ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext);
    };
}