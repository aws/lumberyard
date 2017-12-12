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

#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXMotionCompressionSettingsRule.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class EFXMotionCompressionSettingsRule
                : public DataTypes::IEFXMotionCompressionSettingsRule
            {
            public:
                AZ_RTTI(EFXMotionCompressionSettingsRule, "{2717884D-1F28-4E57-91E2-974FD985C075}", DataTypes::IEFXMotionCompressionSettingsRule);
                AZ_CLASS_ALLOCATOR_DECL

                EFXMotionCompressionSettingsRule();
                ~EFXMotionCompressionSettingsRule() override = default;

                void SetMaxTranslationError(float value);
                void SetMaxRotationError(float value);
                void SetMaxScaleError(float value);

                // IIEFXMotionCompressionSettingsRule overrides
                virtual float GetMaxTranslationError() const;
                virtual float GetMaxRotationError() const;
                virtual float GetMaxScaleError() const;

                static void Reflect(ReflectContext* context);

            protected:
                float m_maxTranslationError;
                float m_maxRotationError;
                float m_maxScaleError;
            };
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED