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

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

class IPostEffectGroup;

namespace AZ
{
    class SerializeContext;
    class BehaviorContext;
}

namespace GraphicsReflectContext
{
    // Exposes PostEffect functionality to Script Canvas and Lua
    class PostEffects
    {
    public:
        AZ_TYPE_INFO(PostEffects, "{4DF6AE85-E113-4B7C-9BDC-BD07A457B057}")

        static void EnableWaterDroplets(float amount);
        static void DisableWaterDroplets();

        static void EnableDepthOfField(float focusDistance, float FocusRange, float blurAmount, float maxCoC, float centerWeight);
        static void DisableDepthOfField();

        static void EnableBlur(float amount);
        static void DisableBlur();

        static void EnableRadialBlur(float amount, const AZ::Vector2& screenPos, float radius);
        static void DisableRadialBlur();

        static void EnableColorCorrection(float colorC, float colorM, float colorY, float colorK, float brightness, float contrast, float saturation, float colorHue);
        static void DisableColorCorrection();

        static void EnableFrost(float amount, float centerAmount);
        static void DisableFrost();

        static void EnableWaterFlow(float amount);
        static void DisableWaterFlow();

        static void EnableGhosting(float amount);
        static void DisableGhosting();

        static void EnableVolumetricScattering(float amount, float tiling, float speed, const AZ::Color& color);
        static void DisableVolumetricScattering();

        static void EnableRainDrops(float amount, float spawnTimeDistance, float size, float sizeVariation);
        static void DisableRainDrops();

        static void EnableSharpen(float amount);
        static void DisableSharpen();

        static void EnableDirectionalBlur(const AZ::Vector2& direction);
        static void DisableDirectionalBlur();

        static void EnableVisualArtifacts(float vsync, float vsyncFrequency, float interlacing, float interlacingTile, float interlacingRot, float syncWavePhase, float syncWaveFrequency, float syncWaveAmplitude, float chromaShift, float grain, const AZ::Color& colorTint, const AZStd::string& maskTextureName);
        static void DisableVisualArtifacts();

        static void EnableEffectGroup(AZStd::string_view groupName);
        static void DisableEffectGroup(AZStd::string_view groupName);
        static void ApplyEffectGroupAtPosition(AZStd::string_view groupName, const AZ::Vector3& position);

        static void SetColorChart(AZStd::string_view textureName, float fadeTime);

        static void ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext);

    private:
        static IPostEffectGroup* GetGroup(const char* groupName);
    };
}