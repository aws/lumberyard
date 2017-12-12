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

#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXMotionScaleRule.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class EFXMotionScaleRule
                : public DataTypes::IEFXMotionScaleRule
            {
            public:
                AZ_RTTI(EFXMotionScaleRule, "{5C0B0CD3-5CC8-42D0-99EC-FD5744B11B95}", DataTypes::IEFXMotionScaleRule);
                AZ_CLASS_ALLOCATOR_DECL

                EFXMotionScaleRule();
                ~EFXMotionScaleRule() override = default;

                void SetScaleFactor(float value);

                // IIEFXMotionScaleRule overrides
                virtual float GetScaleFactor() const;

                static void Reflect(ReflectContext* context);

            protected:
                float m_scaleFactor;
            };
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED