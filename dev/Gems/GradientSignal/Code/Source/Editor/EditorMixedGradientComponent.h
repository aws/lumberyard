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

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/MixedGradientComponent.h>

namespace GradientSignal
{
    template<>
    struct HasCustomSetSamplerOwner<MixedGradientConfig>
        : AZStd::true_type
    {
    };

    template<typename T>
    bool ValidateGradientEntityIds(T& configuration, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        bool validated = true;

        for (auto& layer : configuration.m_layers)
        {
            validated &= layer.m_gradientSampler.ValidateGradientEntityId();
        }

        return validated;
    }

    template<typename T>
    void SetSamplerOwnerEntity(T& configuration, AZ::EntityId entityId, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        for (auto& layer : configuration.m_layers)
        {
            layer.m_gradientSampler.m_ownerEntityId = entityId;
        }
    }

    template<typename T>
    AzToolsFramework::EntityIdList GetSamplerGradientEntities(T& configuration, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        AzToolsFramework::EntityIdList gradientIds;
        for (auto& layer : configuration.m_layers)
        {
            gradientIds.push_back(layer.m_gradientSampler.m_gradientId);
        }

        return gradientIds;
    }

    template<typename T>
    void SetSamplerGradientEntities(T& configuration, AzToolsFramework::EntityIdList gradientIds, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        size_t numLayers = configuration.GetNumLayers();
        if (numLayers != gradientIds.size())
        {
            return;
        }

        for (int i = 0; i < numLayers; ++i)
        {
            configuration.m_layers[i].m_gradientSampler.m_gradientId = gradientIds[i];
        }
    }

    class EditorMixedGradientComponent
        : public EditorGradientComponentBase<MixedGradientComponent, MixedGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<MixedGradientComponent, MixedGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorMixedGradientComponent, "{90642402-6C5F-4C4D-B1F0-8C0F242A2716}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Gradient Mixer";
        static constexpr const char* const s_componentDescription = "Generates a new gradient by combining other gradients";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradientmodifiers/gradient-mixer";

    protected:

        AZ::u32 ConfigurationChanged() override;
    };
}
