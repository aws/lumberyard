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
#include "LmbrCentral_precompiled.h"

#include "EditorParticleComponent.h"

#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Include/IEditorParticleManager.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace LmbrCentral
{

    void EditorParticleComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorParticleComponent, EditorComponentBase>()->
                Version(2)->
                Field("Visible", &EditorParticleComponent::m_visible)->
                Field("Enable", &EditorParticleComponent::m_enable)->
                Field("ParticleLibrary", &EditorParticleComponent::m_asset)->
                Field("SelectedEmitter", &EditorParticleComponent::m_selectedEmitter)->
                Field("LibrarySource", &EditorParticleComponent::m_librarySource)->
                Field("Particle", &EditorParticleComponent::m_settings)->
                Field("EnableAudio", &EditorParticleComponent::m_enableAudio)->
                Field("RTPC", &EditorParticleComponent::m_rtpc)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorParticleComponent>("Particle", "The Particle component allows the placement of a single particle emitter on an entity (Entities can have more than one Particle component)")->

                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(AZ::Edit::Attributes::Category, "Rendering")->
                    Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Particle.png")->
                    Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<LmbrCentral::ParticleAsset>::Uuid())->
                    Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Particle.png")->
                    Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                    Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-particle.html")->
                    Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))->

                    DataElement(0, &EditorParticleComponent::m_visible, "Visible", "Whether or not the particle emitter is currently visible")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnVisibilityChanged)->

                    DataElement(0, &EditorParticleComponent::m_enable, "Enable", "Whether or not the particle component is active.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnEnableChanged)->

                    DataElement(0, &EditorParticleComponent::m_asset, "Particle effect library", "The library of particle effects.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnAssetChanged)->

                    DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorParticleComponent::m_selectedEmitter, "Emitters", "List of emitters in this library.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnEmitterSelected)->
                    Attribute(AZ::Edit::Attributes::StringList, &EditorParticleComponent::GetEmitterNames)->
                    
                    DataElement(0, &EditorParticleComponent::m_settings, "Spawn Settings", "")->
                    Attribute(AZ::Edit::Attributes::AutoExpand, false)->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnSpawnParamChanged)->

                    // add audio settings here so we can have proper layout
                    ClassElement(AZ::Edit::ClassElements::Group, "Audio Settings")->
                    Attribute(AZ::Edit::Attributes::AutoExpand, false)->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorParticleComponent::m_enableAudio, "Enable audio", "Used by particle effect instances to indicate whether audio should be updated or not.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnAudioChanged)->
                    DataElement("AudioControl", &EditorParticleComponent::m_rtpc, "Audio RTPC", "Indicates what audio RTPC this particle effect instance drives.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorParticleComponent::OnAudioChanged)
                ;

                editContext->Class<ParticleEmitterSettings>("ParticleSettings", "")->
                    
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_prime, "Pre-Roll", "(CPU Only) Set emitter as though it's been running indefinitely.")->

                    DataElement(AZ::Edit::UIHandlers::Color, &ParticleEmitterSettings::m_color, "Color tint", "Particle color tint")->

                    DataElement(0, &ParticleEmitterSettings::m_countScale, "Count scale", "Multiple for particle count")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxCountScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_timeScale, "Time scale", "Multiple for emitter time evolution.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxTimeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_pulsePeriod, "Pulse period", "How often to restart emitter.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->

                    DataElement(0, &ParticleEmitterSettings::m_speedScale, "Speed scale", "Multiple for particle emission speed.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSpeedScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &ParticleEmitterSettings::m_strength, "Strength Curve Time", 
                        "Controls all \"Strength Over Emitter Life\" curves.\n\
The curves will use this Strength Curve Time instead of the actual Emitter life time.\n\
Negative values will be ignored.\n")->

                    Attribute(AZ::Edit::Attributes::Min, ParticleEmitterSettings::MinLifttimeStrength)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxLifttimeStrength)->
                    Attribute(AZ::Edit::Attributes::Step, 0.05f)->

                    DataElement(0, &ParticleEmitterSettings::m_sizeScale, "Global size scale", "Multiple for all effect sizes.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleX, "Particle size scale x", "Particle size scale x")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleY, "Particle size scale y", "Particle size scale y")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleZ, "Particle size scale z", "Particle size scale z for geometry particle")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &ParticleEmitterSettings::m_particleSizeScaleRandom, "Particle size scale random", "(CPU only) Randomize particle size scale")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, 1.f)->
                    Attribute(AZ::Edit::Attributes::Step, 0.05f)->

                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_ignoreRotation, "Ignore rotation", "Ignored the entity's rotation.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_notAttached, "Not attached", "If selected, the entity's position is ignored. Emitter does not follow its entity.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_registerByBBox, "Register by bounding box", "Use the Bounding Box instead of Position to Register in VisArea.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_useLOD, "Use LOD", "Activates LOD if they exist on emitter.")->

                    DataElement(AZ::Edit::UIHandlers::Default, &ParticleEmitterSettings::m_targetEntity, "Target Entity", "Target Entity to be used for emitters with Target Attraction or similar features enabled.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_viewDistMultiplier, "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default. Set to 100 for infinite visibility ")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, static_cast<float>(IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX))->

                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_useVisAreas, "Use VisAreas", "Allow VisAreas to control this component's visibility.")
                ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<EditorParticleComponent>()
                ->RequestBus("EditorParticleComponentRequestBus");

            behaviorContext->EBus<EditorParticleComponentRequestBus>("EditorParticleComponentRequestBus")
                ->Event("SetVisibility", &EditorParticleComponentRequestBus::Events::SetVisibility)
                ->Event("GetVisibility", &EditorParticleComponentRequestBus::Events::GetVisibility)
                ->VirtualProperty("Visible", "GetVisibility", "SetVisibility")
                ->Event("Enable", &EditorParticleComponentRequestBus::Events::Enable)
                ->Event("GetEnable", &EditorParticleComponentRequestBus::Events::GetEnable)
                ->VirtualProperty("Enable", "GetEnable", "Enable")
                ->Event("SetColorTint", &EditorParticleComponentRequestBus::Events::SetColorTint)
                ->Event("GetColorTint", &EditorParticleComponentRequestBus::Events::GetColorTint)
                ->VirtualProperty("ColorTint", "GetColorTint", "SetColorTint")
                ->Event("SetCountScale", &EditorParticleComponentRequestBus::Events::SetCountScale)
                ->Event("GetCountScale", &EditorParticleComponentRequestBus::Events::GetCountScale)
                ->VirtualProperty("CountScale", "GetCountScale", "SetCountScale")
                ->Event("SetTimeScale", &EditorParticleComponentRequestBus::Events::SetTimeScale)
                ->Event("GetTimeScale", &EditorParticleComponentRequestBus::Events::GetTimeScale)
                ->VirtualProperty("TimeScale", "GetTimeScale", "SetTimeScale")
                ->Event("SetSpeedScale", &EditorParticleComponentRequestBus::Events::SetSpeedScale)
                ->Event("GetSpeedScale", &EditorParticleComponentRequestBus::Events::GetSpeedScale)
                ->VirtualProperty("SpeedScale", "GetSpeedScale", "SetSpeedScale")
                ->Event("SetGlobalSizeScale", &EditorParticleComponentRequestBus::Events::SetGlobalSizeScale)
                ->Event("GetGlobalSizeScale", &EditorParticleComponentRequestBus::Events::GetGlobalSizeScale)
                ->VirtualProperty("GlobalSizeScale", "GetGlobalSizeScale", "SetGlobalSizeScale")
                ->Event("SetParticleSizeScaleX", &EditorParticleComponentRequestBus::Events::SetParticleSizeScaleX)
                ->Event("GetParticleSizeScaleX", &EditorParticleComponentRequestBus::Events::GetParticleSizeScaleX)
                ->VirtualProperty("ParticleSizeScaleX", "GetParticleSizeScaleX", "SetParticleSizeScaleX")
                ->Event("SetParticleSizeScaleY", &EditorParticleComponentRequestBus::Events::SetParticleSizeScaleY)
                ->Event("GetParticleSizeScaleY", &EditorParticleComponentRequestBus::Events::GetParticleSizeScaleY)
                ->VirtualProperty("ParticleSizeScaleY", "GetParticleSizeScaleY", "SetParticleSizeScaleY")
                ->Event("SetParticleSizeScaleZ", &EditorParticleComponentRequestBus::Events::SetParticleSizeScaleZ)
                ->Event("GetParticleSizeScaleZ", &EditorParticleComponentRequestBus::Events::GetParticleSizeScaleZ)
                ->VirtualProperty("ParticleSizeScaleZ", "GetParticleSizeScaleZ", "SetParticleSizeScaleZ")
                ;
        }
    }

    EditorParticleComponent::EditorParticleComponent()
    {
        m_visible = true;
        m_enable = true;
        m_enableAudio = false;
        m_rtpc.m_propertyType = AzToolsFramework::AudioPropertyType::Rtpc;
    }

    EditorParticleComponent::~EditorParticleComponent()
    {
    }
    
    void EditorParticleComponent::Init()
    {
        EditorComponentBase::Init();
    }

    void EditorParticleComponent::Activate()
    {
        EditorComponentBase::Activate();

        //attach entity to emitter so the emitter spawning will get correct entity transformation
        m_emitter.AttachToEntity(m_entity->GetId());

        //set selected emitter
        if (m_librarySource == LibrarySource::File)
        {
            m_emitterFullNameToSelect = m_settings.m_selectedEmitter;

            // Populates the emitter dropdown.
            OnAssetChanged();
        }
        else if (m_librarySource == LibrarySource::Level)
        {
            //Populates level's emitter list and showing selected 
            ResolveLevelEmitter();
        }

        SetDirty();
                
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());

        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EditorParticleComponentRequestBus::Handler::BusConnect(m_entity->GetId());
    }

    void EditorParticleComponent::Deactivate()
    {
        EditorParticleComponentRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        if (!m_libNameToLoad.empty())
        {
            m_libNameToLoad.clear();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }

        m_emitter.Clear();
        m_emitter.AttachToEntity(AZ::EntityId());

        EditorComponentBase::Deactivate();
    }

    IRenderNode* EditorParticleComponent::GetRenderNode()
    {
        return m_emitter.GetRenderNode();
    }

    float EditorParticleComponent::GetRenderNodeRequestBusOrder() const
    {
        return ParticleComponent::s_renderNodeRequestBusOrder;
    }

    void EditorParticleComponent::DisplayEntity(bool& handled)
    {
        // Don't draw extra visualization unless selected and there is not an emitter
        if (!IsSelected() || m_emitter.IsCreated())
        {
            handled = true;
        }
    }

    void EditorParticleComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        ParticleComponent* component = gameEntity->CreateComponent<ParticleComponent>();

        if (component)
        {
            // Copy the editor-side settings to the game entity to be exported.
            component->m_settings = m_settings;
        }
    }

    void EditorParticleComponent::OnEntityVisibilityChanged(bool /*visibility*/)
    {
        m_emitter.ApplyEmitterSetting(GetSettingsForEmitter());
    }

    void EditorParticleComponent::OnEmitterSelected()
    {
        SetDirty();

        if (m_libName.empty())
        {
            m_settings.m_selectedEmitter = m_selectedEmitter;
        }
        else
        {
            m_settings.m_selectedEmitter = m_libName + "." + m_selectedEmitter;
        }
        m_emitter.Set(m_settings.m_selectedEmitter, GetSettingsForEmitter());
    }

    void EditorParticleComponent::OnSpawnParamChanged()
    {
        m_emitter.ApplyEmitterSetting(GetSettingsForEmitter());
    }

    void EditorParticleComponent::OnAudioChanged()
    {
        m_settings.m_enableAudio = m_enableAudio;
        m_settings.m_audioRTPC = m_rtpc.m_controlName;
        m_emitter.ApplyEmitterSetting(GetSettingsForEmitter());
    }

    void EditorParticleComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_asset.GetId())
        {
            m_librarySource = LibrarySource::File;

            // Grab emitter list from asset and refresh properties.
            m_emitterNames = static_cast<ParticleAsset*>(asset.Get())->m_emitterNames;

            //get lib name
            m_libName = "";
            if (m_emitterNames.size() > 0)
            {
                m_libName = m_emitterNames[0].substr(0, m_emitterNames[0].find_first_of('.'));
            }

            //remove lib name from from emitter names
            for (AZStd::string& emitterName : m_emitterNames)
            {
                emitterName = emitterName.substr(emitterName.find_first_of('.')+1);
            }

            //select a emitter since a emitter list changed
            if (m_emitterFullNameToSelect.empty())
            {
                SelectEmitter(m_settings.m_selectedEmitter);
            }
            else
            {
                SelectEmitter(m_emitterFullNameToSelect);
                if (m_emitterFullNameToSelect == m_settings.m_selectedEmitter)
                {
                    m_emitterFullNameToSelect.clear();
                }
            }
            
            //refresh the selected emitter UI
            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
    }

    void EditorParticleComponent::SelectEmitter(const AZStd::string& emitterFullName)
    {
        //format the emitter name. 
        AZStd::string selEmitterName = emitterFullName;
        AZStd::string libName;
        int pos = emitterFullName.find_first_of('.');
        if (pos >= 0)
        {
            selEmitterName = emitterFullName.substr(pos + 1);
            libName = emitterFullName.substr(0, pos);
        }

        if (!m_emitterNames.empty())
        {
            //check if the selected emitter still exist
            if (m_libName == libName)
            {
                for (AZStd::string emitterName : m_emitterNames)
                {
                    if (selEmitterName == emitterName)
                    {
                        m_selectedEmitter = emitterName;
                        OnEmitterSelected();
                        return;
                    }
                }
            }

            m_selectedEmitter = m_emitterNames[0];
            OnEmitterSelected();
        }
        else
        {
            m_selectedEmitter.clear();
            OnEmitterSelected();
        }
    }

    void EditorParticleComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        /*
         *There are 2 scenarios when the asset will be reloaded
         *1. The library is renamed/deleted
         *2. There is an emitter being renamed/deleted
         *Thus it's safer to clear the cache and rely on OnAssetReady to do all the assigning.
         *If the library is deleted in particle editor/locally, the asset would be invalid, need to have check here.
         */

        m_emitterNames.clear();
        m_emitter.Clear();
        m_selectedEmitter.clear();
        m_libName.clear();

        m_asset = asset;

        if (m_asset.GetId().IsValid())
        {
            OnAssetReady(asset);
        }
        else
        {
            m_emitterFullNameToSelect.clear();
            m_settings.m_selectedEmitter.clear();
            SetDirty();
        }
    }

    void EditorParticleComponent::OnAssetChanged()
    {
        m_emitterNames.clear();
        m_emitter.Clear();
        m_selectedEmitter.clear();
        m_libName.clear();

        if (AZ::Data::AssetBus::Handler::BusIsConnected())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        if (m_asset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
            m_asset.QueueLoad();
        }
        else
        {
            m_emitterFullNameToSelect.clear();
            m_settings.m_selectedEmitter.clear();
            SetDirty();
        }

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorParticleComponent::SetPrimaryAsset(const AZ::Data::AssetId& id)
    {
        m_asset.Create(id, true);
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, GetEntityId());
        OnAssetChanged();
    }
    
    void EditorParticleComponent::ResolveLevelEmitter()
    {
        if (m_librarySource == LibrarySource::Level)
        {
            m_emitterNames.clear();

            IEditor* editor = nullptr;
            EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
            IEditorParticleManager* pPartManager = editor->GetParticleManager();
            if (pPartManager)
            {
                IDataBaseLibrary* lib = pPartManager->GetLevelLibrary();
                if (lib)
                {
                    for (int i = 0; i < lib->GetItemCount(); ++i)
                    {
                        IDataBaseItem* item = lib->GetItem(i);
                        auto itemName = item->GetName();
                        AZStd::string effectName = AZStd::string::format("Level.%s", itemName.toUtf8().data());
                        m_emitterNames.push_back(effectName);
                    }
                    m_libName = "";
                }

                if (!m_emitterNames.empty())
                {
                    SelectEmitter(m_settings.m_selectedEmitter);
                }
            }
        }
    }

    AZ::u32 EditorParticleComponent::OnVisibilityChanged()
    {
        m_settings.m_visible = m_visible;

        if (m_emitter.IsCreated())
        {
            OnSpawnParamChanged();
        }

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 EditorParticleComponent::OnEnableChanged()
    {
        m_settings.m_enable = m_enable;

        if (m_emitter.IsCreated())
        {
            OnSpawnParamChanged();
        }

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }
    
    ParticleEmitterSettings EditorParticleComponent::GetSettingsForEmitter() const
    {
        ParticleEmitterSettings settingsForEmitter = m_settings;

        // take the entity's visibility into account
        bool entityVisibility = true;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(entityVisibility, GetEntityId(), &AzToolsFramework::EditorVisibilityRequestBus::Events::GetCurrentVisibility);
        settingsForEmitter.m_visible &= entityVisibility;

        return settingsForEmitter;
    }

    void EditorParticleComponent::SetEmitter(const AZStd::string& emitterFullName, const AZStd::string& libPath)
    {      
        m_librarySource = LibrarySource::File;

        //set selected emitter
        m_emitterFullNameToSelect = emitterFullName;        
        
        //find the asset
        AZ::Data::AssetId cmAssetId;
        EBUS_EVENT_RESULT(cmAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, libPath.c_str(), azrtti_typeid<ParticleAsset>(), false);
        if (!cmAssetId.IsValid())
        {   //asset does not exist, listen to the asset add event from AssetCatalogEventBus
            m_libNameToLoad = libPath;
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }
        else
        {
            //assign to the asset
            m_asset.Create(cmAssetId, true);
            //Populates the emitter list and create emitter from selected effect.
            OnAssetChanged();
        }
    }

    void EditorParticleComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetId cmAssetId;
        EBUS_EVENT_RESULT(cmAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, m_libNameToLoad.c_str(), azrtti_typeid<ParticleAsset>(), false);

        if (cmAssetId == assetId)
        {
            m_libNameToLoad.clear();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            m_asset.Create(assetId, true);
            OnAssetChanged();
        }
    }

    void EditorParticleComponent::Enable(bool enable)
    {
        m_settings.m_enable = enable;
        m_emitter.Enable(enable);
    }

    void EditorParticleComponent::SetVisibility(bool visible)
    {
        m_settings.m_visible = visible;
        m_emitter.SetVisibility(visible);
    }

    bool EditorParticleComponent::GetVisibility()
    {
        return m_settings.m_visible;
    }

    void EditorParticleComponent::SetColorTint(const AZ::Color& tint)
    {
        m_settings.m_color = tint;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetCountScale(float scale)
    {
        if (ParticleEmitterSettings::MaxCountScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_countScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetTimeScale(float scale)
    {
        if (ParticleEmitterSettings::MaxTimeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_timeScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);

    }

    void EditorParticleComponent::SetSpeedScale(float scale)
    {
        if (ParticleEmitterSettings::MaxSpeedScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_speedScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetGlobalSizeScale(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_sizeScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetParticleSizeScaleX(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_particleSizeScaleX = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetParticleSizeScaleY(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_particleSizeScaleY = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void EditorParticleComponent::SetParticleSizeScaleZ(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_particleSizeScaleZ = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }


    bool EditorParticleComponent::GetEnable()
    {
        return m_settings.m_enable;
    }

    AZ::Color EditorParticleComponent::GetColorTint()
    {
        return m_settings.m_color;
    }

    float EditorParticleComponent::GetCountScale()
    {
        return m_settings.m_countScale;
    }

    float EditorParticleComponent::GetTimeScale()
    {
        return m_settings.m_timeScale;
    }

    float EditorParticleComponent::GetSpeedScale()
    {
        return m_settings.m_speedScale;
    }

    float EditorParticleComponent::GetGlobalSizeScale()
    {
        return m_settings.m_sizeScale;
    }

    float EditorParticleComponent::GetParticleSizeScaleX()
    {
        return m_settings.m_particleSizeScaleX;
    }

    float EditorParticleComponent::GetParticleSizeScaleY()
    {
        return m_settings.m_particleSizeScaleY;
    }

    float EditorParticleComponent::GetParticleSizeScaleZ()
    {
        return m_settings.m_particleSizeScaleZ;
    }
} // namespace LmbrCentral
