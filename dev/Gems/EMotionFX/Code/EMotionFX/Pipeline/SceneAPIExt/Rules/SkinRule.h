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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXSkinRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class EFXSkinRule
                : public DataTypes::IEFXSkinRule
            {
            public:
                AZ_RTTI(EFXSkinRule, "{B26E7FC9-86A1-4711-8415-8BE4861C08BA}", DataTypes::IEFXSkinRule);
                AZ_CLASS_ALLOCATOR_DECL

                EFXSkinRule();
                ~EFXSkinRule() override = default;

                uint32_t GetMaxWeightsPerVertex() const override;
                float GetWeightThreshold() const override;

                static void Reflect(ReflectContext* context);

            protected:
                uint32_t m_maxWeightsPerVertex;
                float m_weightThreshold;
            };
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED