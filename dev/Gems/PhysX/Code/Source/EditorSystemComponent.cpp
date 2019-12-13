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

#include <PhysX_precompiled.h>
#include "EditorSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysX/ConfigurationBus.h>
#include <Editor/ConfigStringLineEditCtrl.h>
#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

namespace PhysX
{
    const float EditorSystemComponent::s_minEditorWorldUpdateInterval = 0.1f;
    const float EditorSystemComponent::s_fixedDeltaTime = 0.1f;

    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
        Physics::WorldConfiguration editorWorldConfiguration = configuration.m_worldConfiguration;
        editorWorldConfiguration.m_fixedTimeStep = 0.0f;

        Physics::SystemRequestBus::BroadcastResult(m_editorWorld, &Physics::SystemRequests::CreateWorldCustom,
            Physics::EditorPhysicsWorldId, editorWorldConfiguration);

        PhysX::RegisterConfigStringLineEditHandler(); // Register custom unique string line edit control

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();
        m_editorWorld = nullptr;
    }

    AZStd::shared_ptr<Physics::World> EditorSystemComponent::GetEditorWorld()
    {
        return m_editorWorld;
    }

    void EditorSystemComponent::MarkEditorWorldDirty()
    {
        m_editorWorldDirty = true;
    }

    void EditorSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_intervalCountdown -= deltaTime;

        if (m_editorWorldDirty && m_editorWorld && m_intervalCountdown <= 0.0f)
        {
            m_editorWorld->Update(s_fixedDeltaTime);
            m_intervalCountdown = s_minEditorWorldUpdateInterval;
            m_editorWorldDirty = false;
        }
    }

    // This will be called when the IEditor instance is ready
    void EditorSystemComponent::NotifyRegisterViews()
    {
        RegisterForEditorEvents();
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorSystemComponent::OnStopPlayInEditor()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void EditorSystemComponent::OnCrySystemShutdown(ISystem&)
    {
        UnregisterForEditorEvents();
    }

    void EditorSystemComponent::RegisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::UnregisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->UnregisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::OnEditorNotifyEvent(const EEditorNotifyEvent editorEvent)
    {
        // Updating default material library requires Cry Surface Types initialization finished
        // Hence we do it on eNotify_OnInit event
        if (editorEvent == eNotify_OnInit)
        {
            UpdateDefaultMaterialLibrary();
        }
    }

    void EditorSystemComponent::UpdateDefaultMaterialLibrary()
    {
        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &ConfigurationRequests::GetConfiguration);

        AZ_Error("Physics", !configuration.m_materialLibrary.IsError(), "Default material library '%s' not found, generating default library", configuration.m_materialLibrary.GetHint().c_str());

        if (!configuration.m_materialLibrary.GetId().IsValid() || configuration.m_materialLibrary.IsError())
        {
            // if the default material library is not set, we generate a new one from the Cry Engine surface types
            AZ::Data::AssetId newLibraryAssetId = GenerateSurfaceTypesLibrary();

            if (newLibraryAssetId.IsValid())
            {
                // New material library successfully created, set it to configuration
                configuration.m_materialLibrary =
                    AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(newLibraryAssetId, true, nullptr, true);

                // Update the configuration (this will also update the configuration file)
                PhysX::ConfigurationRequestBus::Broadcast(&ConfigurationRequests::SetConfiguration, configuration);
            }
        }
    }

    AZ::Data::AssetId EditorSystemComponent::GenerateSurfaceTypesLibrary()
    {
        AZ::Data::AssetId resultAssetId;

        auto assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();

        AZStd::vector<AZStd::string> assetTypeExtensions;
        AZ::AssetTypeInfoBus::Event(assetType, &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);

        if (assetTypeExtensions.size() == 1)
        {
            const char* DefaultAssetFilename = "SurfaceTypeMaterialLibrary";

            // Constructing the path to the library asset
            const AZStd::string& assetExtension = assetTypeExtensions[0];

            // Use the path relative to the asset root to avoid hardcoding full path in the configuration
            AZStd::string relativePath = DefaultAssetFilename;
            AzFramework::StringFunc::Path::ReplaceExtension(relativePath, assetExtension.c_str());

            // Try to find an already existing material library
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(resultAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.c_str(), azrtti_typeid<Physics::MaterialLibraryAsset>(), false);

            if (!resultAssetId.IsValid())
            {
                const char* assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");

                AZStd::string fullPath;
                AzFramework::StringFunc::Path::ConstructFull(assetRoot, DefaultAssetFilename, assetExtension.c_str(), fullPath);

                bool materialLibraryCreated = false;
                AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(materialLibraryCreated,
                    &AZ::LegacyConversion::LegacyConversionRequests::CreateSurfaceTypeMaterialLibrary, fullPath);

                if (materialLibraryCreated)
                {
                    // Find out the asset ID for the material library we've just created
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        resultAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                        relativePath.c_str(),
                        azrtti_typeid<Physics::MaterialLibraryAsset>(), true);
                }
                else
                {
                    AZ_Warning("PhysX", false,
                        "GenerateSurfaceTypesLibrary: Failed to create material library at %s. "
                        "Please check if the file is writable", fullPath.c_str());
                }
            }
        }
        else
        {
            AZ_Warning("PhysX", false, 
                "GenerateSurfaceTypesLibrary: Number of extensions for the physics material library asset is %u"
                " but should be 1. Please check if the asset registered itself with the asset system correctly",
                assetTypeExtensions.size())
        }

        return resultAssetId;
    }
}

