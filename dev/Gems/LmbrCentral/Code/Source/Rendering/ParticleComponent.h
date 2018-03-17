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

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityBus.h>

#include <IParticles.h>
#include <Cry_Geo.h>

#include <LmbrCentral/Rendering/ParticleComponentBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////


    /*!
    *  Particle emitter, this is a wrapper around IParticleEmitter that provides access to an effect's emitter from a specified particle library.
    */
    class ParticleEmitter
        : public AZ::TransformNotificationBus::Handler
    {
    public:

        ParticleEmitter();

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        //! Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        //! Attaches the component's entity to this emitter.
        void AttachToEntity(AZ::EntityId id);

        //! Will find and load the effect associated with the provided emitter name and apply the ParticleEmitterSettings
        void Set(const AZStd::string& emmitterName, const ParticleEmitterSettings& settings);
        
        //! spawn a new emitter with the spawn setting
        void SpawnEmitter(const ParticleEmitterSettings& settings);

        //! apply the spawn setting to existing emitter
        void ApplyEmitterSetting(const ParticleEmitterSettings& settings);

        //! apply render flags from particle emitter settins
        void ApplyRenderFlags(const ParticleEmitterSettings& settings);

        //! Allows the user to specify the visibility of the emitter.
        void SetVisibility(bool visible);

        //! Marks this emitter as visible
        void Show();

        //! Hides this emitter
        void Hide();

        //! Removes the effect and the emitter.
        void Clear();

        //! Enable/disable the emitter
        void Enable(bool enable);

        //! True if the emitter has been created.
        bool IsCreated() const;

        //! Restarts the emitter.
        void Restart();

        IRenderNode* GetRenderNode();
        
    protected:

        //! Constructs a SpawnParams struct from the component's properties
        SpawnParams GetPropertiesAsSpawnParams(const ParticleEmitterSettings& settings) const;

        //! A handler that tracks updates to the Emitter's TargetEntity, if any.
        class TargetEntityHandler 
            : public AZ::TransformNotificationBus::Handler
        {
        public:
            //! Initializes the TargetEntityHandler to watch a new target.
            //! \param emitter Emitter that is targeting the entity
            //! \param targetEntity ID of the target entity. Invalid will disable targeting for the particle system.
            void Setup(_smart_ptr<IParticleEmitter> emitter, AZ::EntityId targetEntity);

            //! Notified when the target entity moves, to update the Emitter.
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        private:
            //! Update the Emitter's target settings, including position, velocity, and radius.
            //! \param targetEntityTransform TargetEntity's transform is passed in as an optimization since OnTransformChanged() is fed this value already.
            void UpdateTargetPos(const AZ::Transform& targetEntityTransform);

            _smart_ptr<IParticleEmitter> m_emitter;
            AZ::EntityId m_targetEntity;
        } m_targetEntityHandler;


        AZ::EntityId m_attachedToEntityId;

        _smart_ptr<IParticleEffect> m_effect;
        _smart_ptr<IParticleEmitter> m_emitter;
    };

    //////////////////////////////////////////////////////////////////////////

    /*!
    * In-game particle component.
    */
    class ParticleComponent
        : public AZ::Component
        , public ParticleComponentRequestBus::Handler
        , public RenderNodeRequestBus::Handler
    {
    public:

        friend class EditorParticleComponent;

        AZ_COMPONENT(ParticleComponent, "{65BC817A-ABF6-440F-AD4F-581C40F92795}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ParticleComponentRequestBus
        void Show() override;
        void Hide() override;
        void SetVisibility(bool visible) override;
        void SetupEmitter(const AZStd::string& emitterName, const ParticleEmitterSettings& settings) override;
        void Enable(bool enable) override;
        void EnablePreRoll(bool enable) override;
        void SetColorTint(const AZ::Color& tint) override;
        void SetCountScale(float scale) override;
        void SetTimeScale(float scale) override;
        void SetSpeedScale(float scale) override;
        void SetGlobalSizeScale(float scale) override;
        void SetParticleSizeScaleX(float scale) override;
        void SetParticleSizeScaleY(float scale) override;
        void SetPulsePeriod(float pulse) override;
        bool GetVisibility() override;
        bool GetEnable() override;
        AZ::Color GetColorTint() override;
        float GetCountScale() override;
        float GetTimeScale() override;
        float GetSpeedScale() override;
        float GetGlobalSizeScale() override;
        float GetParticleSizeScaleX() override;
        float GetParticleSizeScaleY() override;
        float GetPulsePeriod() override;
        void SetLifetimeStrength(float strenth) override;
        void EnableAudio(bool enable) override;
        void SetRTPC(const AZStd::string& rtpc) override;
        void SetViewDistMultiplier(float multiplier) override;
        void SetUseVisArea(bool enable) override;
        ParticleEmitterSettings GetEmitterSettings() override;
        void Restart() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        static const float s_renderNodeRequestBusOrder;
        //////////////////////////////////////////////////////////////////////////

    private:

        ParticleEmitter m_emitter;
        ParticleEmitterSettings m_settings;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ParticleService", 0x725d4a5d));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ParticleService", 0x725d4a5d));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace LmbrCentral
