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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Math/Color.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace LmbrCentral
{
    /*!
    *  Particle emitter settings, these are used by both the in-game and editor components.
    */
    class ParticleEmitterSettings
    {
    public:

        AZ_TYPE_INFO(ParticleEmitterSettings, "{A1E34557-30DB-4716-B4CE-39D52A113D0C}")

        ParticleEmitterSettings()
            : m_visible(true)
            , m_enable(true)
            , m_prime(false)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f)
            , m_countScale(1.f)
            , m_timeScale(1.f)
            , m_speedScale(1.f)
            , m_sizeScale(1.f)
            , m_pulsePeriod(0.f)
            , m_particleSizeScaleX(1.0f)
            , m_particleSizeScaleY(1.0f)
            , m_particleSizeScaleZ(1.0f)
            , m_particleSizeScaleRandom(0.0f)
            , m_strength(-1.f)
            , m_ignoreRotation(false)
            , m_notAttached(false)
            , m_registerByBBox(false)
            , m_useLOD(true)
            , m_enableAudio(false)
            , m_viewDistMultiplier(1.0f)
            , m_useVisAreas(true)
        {}

        bool                        m_visible;                  //!< Controls the emitter's visibility.
        bool                        m_enable;                   //!< Controls whether emitter is active.
        AZStd::string               m_selectedEmitter;          //!< Name of the particle emitter to use.
        AZ::EntityId                m_targetEntity;             //!< Target Entity to be used for emitters with Target Attraction or similar features enabled.
        bool                        m_prime;                    //!< "Pre-Roll". Set emitter as though it's been running indefinitely.
        AZ::Color                   m_color;                    //!< An additional tint color
        float                       m_countScale;               //!< Multiple for particle count (on top of m_countPerUnit if set).
        float                       m_timeScale;                //!< Multiple for emitter time evolution.
        float                       m_speedScale;               //!< Multiple for particle emission speed.
        float                       m_pulsePeriod;              //!< How often to restart emitter.
        float                       m_particleSizeScaleX;       //!< Scaling on particle size X
        float                       m_particleSizeScaleY;       //!< Scaling on particle size Y
        float                       m_particleSizeScaleZ;       //!< Scaling on particle size Z; only for geometry particle
        float                       m_particleSizeScaleRandom;  //!< Random Scaling on partle size scale
        float                       m_strength;                 //!< Controls the affect of Strength Over Emitter lifetime curve, negative value applies strength over emitter lifetime curve.\
                                                                //zero removes the affect of the curve. positive value applies the affect uniformly over emitter lifetime
        float                       m_sizeScale;                //!< Multiple for all effect sizes.
        bool                        m_ignoreRotation;           //!< The entity rotation is ignored.
        bool                        m_notAttached;              //!< The entity's position is ignore (i.e. emitter does not follow its entity).
        bool                        m_registerByBBox;           //!< Use the Bounding Box instead of Position to Register in VisArea
        bool                        m_useLOD;                   //!< Activates LOD if they exist on emitter
        bool                        m_enableAudio;              //!< Used by particle effect instances to indicate whether audio should be updated or not.
        AZStd::string               m_audioRTPC;                //!< Indicates what audio RTPC this particle effect instance drives.

        //! For IRenderNode
        float                       m_viewDistMultiplier;       
        bool                        m_useVisAreas;              //!< Indicates whether it allows VisAreas to control this component's visibility
                
        static void Reflect(AZ::ReflectContext* context);

        const static float MaxCountScale;
        const static float MaxTimeScale;
        const static float MaxSpeedScale;
        const static float MaxSizeScale;
        const static float MaxLifttimeStrength;
        const static float MinLifttimeStrength;
        
    private:

        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);
    };


    /*!
     * ParticleComponentRequestBus
     * Messages serviced by the Particle component.
     */
    class ParticleComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~ParticleComponentRequests() {}

        // Shows the particle component.
        virtual void Show() = 0;
        
        // Hides the particle component.
        virtual void Hide() = 0;

        // Specifies the visibility of the particle component.
        virtual void SetVisibility(bool visible) = 0;

        // Get the visibility of the particle component.
        virtual bool GetVisibility() = 0;
                
        // Enable/disable current emitter.
        virtual void Enable(bool enable) = 0;

        // Get whehther current emitter is enabled.
        virtual bool GetEnable() = 0;

        // Set pre-roll
        virtual void EnablePreRoll(bool enable) = 0;
        
        // Set color tint
        virtual void SetColorTint(const AZ::Color& tint) = 0;

        // Get color tint
        virtual AZ::Color GetColorTint() = 0;

        // Set count scale
        virtual void SetCountScale(float scale) = 0;

        // Get count scale
        virtual float GetCountScale() = 0;

        // Set time scale
        virtual void SetTimeScale(float scale) = 0;

        // Get time scale
        virtual float GetTimeScale() = 0;

        // Set speed scale
        virtual void SetSpeedScale(float scale) = 0;

        // Get speed scale
        virtual float GetSpeedScale() = 0;

        // Set global size scale
        virtual void SetGlobalSizeScale(float scale) = 0;

        // Get global size scale
        virtual float GetGlobalSizeScale() = 0;
        
        // Set particle size scale
        virtual void SetParticleSizeScaleX(float scale) = 0;
        virtual void SetParticleSizeScaleY(float scale) = 0;
        virtual void SetParticleSizeScaleZ(float scale) = 0;

        // Get particle size scale
        virtual float GetParticleSizeScaleX() = 0;
        virtual float GetParticleSizeScaleY() = 0;
        virtual float GetParticleSizeScaleZ() = 0;
        
        // Set pulse period
        virtual void SetPulsePeriod(float pulse) = 0;

        // Get pulse period
        virtual float GetPulsePeriod() = 0;

        // Set lifetime strength
        virtual void SetLifetimeStrength(float strenth) = 0;

        // Enable audio
        virtual void EnableAudio(bool enable) = 0;

        // Set audio RTPC
        virtual void SetRTPC(const AZStd::string& rtpc) = 0;

        // Set view distance multiplier
        virtual void SetViewDistMultiplier(float multiplier) = 0;

        // Set using VisArea
        virtual void SetUseVisArea(bool enable) = 0;
        
        // get emitter setting
        virtual ParticleEmitterSettings GetEmitterSettings() = 0;

        // Sets up an effect emitter by name and params.
        virtual void SetupEmitter(const AZStd::string& emitterName, const ParticleEmitterSettings& settings) {}

        // Restarts the emitter.
        virtual void Restart() { }
    };

    using ParticleComponentRequestBus = AZ::EBus <ParticleComponentRequests>;

    /*!
     * ParticleComponentEvents
     * Events dispatched by the Particle component.
     */
    class ParticleComponentEvents
        : public AZ::ComponentBus
    {
    public:

        using Bus = AZ::EBus<ParticleComponentEvents>;
    };


    /*!
    * EditorParticleComponentRequestBus
    * Messages serviced by EditorParticleComponent.
    */
    class EditorParticleComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~EditorParticleComponentRequests() {}

        // Specifies the visibility of the particle component.
        virtual void SetVisibility(bool visible) = 0;

        // Get the visibility of the particle component.
        virtual bool GetVisibility() = 0;

        // Enable/disable current emitter.
        virtual void Enable(bool enable) = 0;

        // Get whether current emitter is enabled.
        virtual bool GetEnable() = 0;

        // Set color tint
        virtual void SetColorTint(const AZ::Color& tint) = 0;

        // Get color tint
        virtual AZ::Color GetColorTint() = 0;

        // Set count scale
        virtual void SetCountScale(float scale) = 0;

        // Get count scale
        virtual float GetCountScale() = 0;

        // Set time scale
        virtual void SetTimeScale(float scale) = 0;

        // Get time scale
        virtual float GetTimeScale() = 0;

        // Set speed scale
        virtual void SetSpeedScale(float scale) = 0;

        // Get speed scale
        virtual float GetSpeedScale() = 0;

        // Set global size scale
        virtual void SetGlobalSizeScale(float scale) = 0;

        // Get global size scale
        virtual float GetGlobalSizeScale() = 0;

        // Set particle size scale
        virtual void SetParticleSizeScaleX(float scale) = 0;
        virtual void SetParticleSizeScaleY(float scale) = 0;
        virtual void SetParticleSizeScaleZ(float scale) = 0;

        // Get particle size scale
        virtual float GetParticleSizeScaleX() = 0;
        virtual float GetParticleSizeScaleY() = 0;
        virtual float GetParticleSizeScaleZ() = 0;

        // Sets up an effect emitter by name
        virtual void SetEmitter(const AZStd::string& emitterName, const AZStd::string& libPath) = 0;
    };

    using EditorParticleComponentRequestBus = AZ::EBus <EditorParticleComponentRequests>;
} // namespace LmbrCentral
