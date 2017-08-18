#pragma once

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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXActorScaleRule.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class EFXActorScaleRule
                : public DataTypes::IEFXActorScaleRule
            {
            public:
                AZ_RTTI(EFXActorScaleRule, "{29A7688B-45DA-449E-9862-8ADD99645F69}", DataTypes::IEFXActorScaleRule);
                AZ_CLASS_ALLOCATOR_DECL

                EFXActorScaleRule();
                ~EFXActorScaleRule() override = default;

                void SetScaleFactor(float value);

                // IIEFXActorScaleRule overrides
                virtual float GetScaleFactor() const;

                static void Reflect(ReflectContext* context);

            protected:
                float m_scaleFactor;
            };
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED