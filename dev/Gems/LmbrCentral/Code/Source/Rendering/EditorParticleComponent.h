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

#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include <LmbrCentral/Rendering/ParticleAsset.h>

#include "ParticleComponent.h"

namespace LmbrCentral
{
    /*!
    * In-editor particle component.
    * Loads available emitters from a specified particle effect library.
    */
    class EditorParticleComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public RenderNodeRequestBus::Handler
        , public EditorParticleComponentRequestBus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::Data::AssetBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    public:

        friend class EditorParticleComponentFactory;

        AZ_COMPONENT(EditorParticleComponent, "{0F35739E-1B40-4497-860D-D6FF5D87A9D9}", AzToolsFramework::Components::EditorComponentBase);

        EditorParticleComponent();
        ~EditorParticleComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorVisibilityNotificationBus interface implementation
        void OnEntityVisibilityChanged(bool visibility) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus interface implementation
        void DisplayEntity(bool& handled) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorParticleComponentRequestBus interface implementation
        void SetEmitter(const AZStd::string& emitterName, const AZStd::string& libPath) override;

        void SetVisibility(bool visible) override;
        void Enable(bool enable) override;
        void SetColorTint(const AZ::Color& tint) override;
        void SetCountScale(float scale) override;
        void SetTimeScale(float scale) override;
        void SetSpeedScale(float scale) override;
        void SetGlobalSizeScale(float scale) override;
        void SetParticleSizeScaleX(float scale) override;
        void SetParticleSizeScaleY(float scale) override;
        void SetParticleSizeScaleZ(float scale) override;
        bool GetVisibility() override;
        bool GetEnable() override;
        AZ::Color GetColorTint() override;
        float GetCountScale() override;
        float GetTimeScale() override;
        float GetSpeedScale() override;
        float GetGlobalSizeScale() override;
        float GetParticleSizeScaleX() override;
        float GetParticleSizeScaleY() override;
        float GetParticleSizeScaleZ() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetEventHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogEventBus
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        //////////////////////////////////////////////////////////////////////////

        //! Called during export for the game-side entity creation.
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        //! Invoked in the editor when the user assigns a new asset.
        void OnAssetChanged();

        //! Invoked in the editor when the user selects an emitter from the combo box.
        void OnEmitterSelected();
        
        //! Invoked in the editor when the user adjusts any properties.
        void OnSpawnParamChanged();

        //! Used to populate the emitter combo box.
        AZStd::vector<AZStd::string> GetEmitterNames() const { return m_emitterNames; }
        
    protected:

        //! Particle libraries may be assets or part of the level.
        enum class LibrarySource
        {
            Level,
            File
        };

        LibrarySource m_librarySource = LibrarySource::File;
        
        void ResolveLevelEmitter();

        //! Select a specific emitter. The default emitter will be selected if the emitter doesn't exist
        void SelectEmitter(const AZStd::string& emitterFullName);

        AZ::u32 OnVisibilityChanged();

        //! Invoked when audio settings changed
        void OnAudioChanged();

        //! Invoked when enable chagned
        AZ::u32 OnEnableChanged();

        //! Get a copy of settings appropriate for use with the emitter
        ParticleEmitterSettings GetSettingsForEmitter() const;

        //! Emitter visibility
        bool m_visible;
        
        //! whether particle component is active
        bool m_enable;

        //! Particle library asset.
        AZ::Data::Asset<ParticleAsset> m_asset;

        //! Emitter to use.
        AZStd::string m_selectedEmitter;

        //! Selected library name. Combine library name and emitter name will be emitter's full name.
        AZStd::string m_libName;

        //! The full file path of the library to be loaded when set a emitter programmatically. 
        AZStd::string m_libNameToLoad;

        //! The emitter to be selected when seting a emitter programmatically.It may not be in the m_emitterNames list until the asset was processed."
        AZStd::string m_emitterFullNameToSelect;

        //! Non serialized list (editor only)
        AZStd::vector<AZStd::string> m_emitterNames;

        //! Audio setting. to achieve the layout we have to have duplicate ones here.
        bool m_enableAudio;
        AzToolsFramework::CReflectedVarAudioControl m_rtpc;

        ParticleEmitterSettings m_settings;
        ParticleEmitter m_emitter;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ParticleComponent::GetProvidedServices(provided);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            ParticleComponent::GetIncompatibleServices(incompatible);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            ParticleComponent::GetRequiredServices(required);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            ParticleComponent::GetDependentServices(dependent);
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace LmbrCentral
