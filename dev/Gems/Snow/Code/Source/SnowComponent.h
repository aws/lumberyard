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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "SnowComponentBus.h"

namespace Snow
{
    class SnowOptions
    {
    public:
        friend class SnowConverter;

        AZ_TYPE_INFO(SnowOptions, "{6157B3D7-5A16-4A12-B315-582248A27EED}");

        bool m_disableOcclusion = false;

        //Surface Params
        float m_radius = 50.0f;
        float m_snowAmount = 10.0f;
        float m_frostAmount = 1.0f;
        float m_surfaceFreezing = 1.0f;
    
        //Snowfall Params
        AZ::u32 m_snowFlakeCount = 100;
        float m_snowFlakeSize = 2.5f;
        float m_snowFallBrightness = 3.0f;
        float m_snowFallGravityScale = 0.1f;
        float m_snowFallWindScale = 0.5f;
        float m_snowFallTurbulence = 0.5f;
        float m_snowFallTurbulenceFreq = 0.1f;

        AZStd::function<void()> m_changeCallback;

        void OnChanged()
        {
            if (m_changeCallback)
            {
                m_changeCallback();
            }
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);
    };

    /**
     * Updates the engine's snow settings based on the given
     * entity ID and snow options
     *
     * This lives outside the snow component so that the EditorSnowComponent
     * can easily access the same functionality.
     *
     * @param worldTransform The transform that the snow effect is going to be centered around
     * @param options Snow options
     */
    void UpdateSnowSettings(const AZ::Transform& worldTransform, SnowOptions options);
    
    /**
     * This really just tells the 3D engine to use snow with an amount of 0
     * which is the way to turn off snow. Other snow components may 
     * apply snow settings after this which is expected. 
     */
    void TurnOffSnow();

    class SnowComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
        , private Snow::SnowComponentRequestBus::Handler
    {
    public:
        friend class EditorSnowComponent;
        
        AZ_COMPONENT(SnowComponent, "{F16AD691-091B-44E2-8FBD-2E3AFC2739EF}");

        ~SnowComponent() override;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TransformBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // SnowComponentRequestBus::Handler
        void Enable() override;
        void Disable() override;
        void Toggle() override;

        bool IsEnabled() override
        {
            return m_enabled;
        }

        /*
            Note: If a user needs to call many setters it's best to not use the 
            individual setters. Instead call GetSnowOptions, modify the object
            and then call SetSnowOptions. That way you're not calling UpdateSnow 
            more than once. 
        */

        float GetRadius() override
        {
            return m_snowOptions.m_radius;
        }
        void SetRadius(float disableOcclusion) override;

        float GetSnowAmount() override
        {
            return m_snowOptions.m_snowAmount;
        }
        void SetSnowAmount(float snowAmount) override;

        float GetFrostAmount() override
        {
            return m_snowOptions.m_frostAmount;
        }
        void SetFrostAmount(float frostAmount) override;

        float GetSurfaceFreezing() override
        {
            return m_snowOptions.m_surfaceFreezing;
        }
        void SetSurfaceFreezing(float surfaceFreezing) override;

        AZ::u32 GetSnowFlakeCount() override
        {
            return m_snowOptions.m_snowFlakeCount;
        }
        void SetSnowFlakeCount(AZ::u32 snowFlakeCount) override;

        float GetSnowFlakeSize() override
        {
            return m_snowOptions.m_snowFlakeSize;
        }
        void SetSnowFlakeSize(float snowFlakeSize) override;

        float GetSnowFallBrightness() override
        {
            return m_snowOptions.m_snowFallBrightness;
        }
        void SetSnowFallBrightness(float snowFallBrightness) override;

        float GetSnowFallGravityScale() override
        {
            return m_snowOptions.m_snowFallGravityScale;
        }
        void SetSnowFallGravityScale(float snowFallGravityScale) override;

        float GetSnowFallWindScale() override
        {
            return m_snowOptions.m_snowFallWindScale;
        }
        void SetSnowFallWindScale(float snowFallWindScale) override;

        float GetSnowFallTurbulence() override
        {
            return m_snowOptions.m_snowFallTurbulence;
        }
        void SetSnowFallTurbulence(float snowFallTurbulence) override;

        float GetSnowFallTurbulenceFreq() override
        {
            return m_snowOptions.m_snowFallTurbulenceFreq;
        }
        void SetSnowFallTurbulenceFreq(float snowFallTurbulenceFreq) override;

        SnowOptions GetSnowOptions() override
        {
            return m_snowOptions;
        }
        void SetSnowOptions(SnowOptions snowOptions) override;

        /**
         * Sends this component's snow parameters to the engine
         *
         * If the component is disabled or the "amount" of this snow object is 0
         * the engine will attempt to disable snow. Snow can be re-enabled if another
         * snow component updates and overrides that setting.
         */
        void UpdateSnow() override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SnowService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SnowService"));
        }

        static void Reflect(AZ::ReflectContext* context);

#ifndef NULL_RENDERER
        /**
         * Cache any snow textures
         *
         * Load any textures needed by the snow system and store the returned pointers. 
         * All textures will be released when the component gets destroyed (not deactivated).
         *
         * This is just to make sure that the snow textures are loaded before they're used.
         */
        void PrecacheTextures();
#endif

        // Reflected Data
        bool m_enabled = true;
        SnowOptions m_snowOptions;

        // Unreflected data
        AZ::Transform m_currentWorldTransform;

#ifndef NULL_RENDERER
        AZStd::vector<ITexture*> m_Textures;
#endif
    };
}