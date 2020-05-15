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

#include "LegacyTerrain_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

//From CryCommon
#include <Terrain/Bus/LegacyTerrainBus.h>

#include <LegacyTerrainLevelComponent.h>

namespace LegacyTerrain
{
    void LegacyTerrainLevelConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LegacyTerrainLevelConfig, AZ::ComponentConfig>()
                ->Version(1);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<LegacyTerrainLevelConfig>(
                    "Legacy Terrain System Component", "Data required for the legacy terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13) }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void LegacyTerrainLevelComponent::Reflect(AZ::ReflectContext* context)
    {
        LegacyTerrainLevelConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LegacyTerrainLevelComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &LegacyTerrainLevelComponent::m_configuration)
                ;
        }
    }

    void LegacyTerrainLevelComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void LegacyTerrainLevelComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void LegacyTerrainLevelComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void LegacyTerrainLevelComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    LegacyTerrainLevelComponent::LegacyTerrainLevelComponent(const LegacyTerrainLevelConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void LegacyTerrainLevelComponent::Init()
    {
    }

    void LegacyTerrainLevelComponent::Activate()
    {
        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (isInstantiated)
        {
            AZ_Warning("LegacyTerrain", false, "The legacy terrain system was already instantiated");
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin);

        // If we're running in the Editor in Game Mode, make sure the Editor version of terrain data is used to instantiate our runtime terrain.
        // This allows us to see edited but unsaved data.  Otherwise, we'll see the last exported version of the runtime terrain.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            LegacyTerrainEditorDataRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainEditorDataRequests::CreateTerrainSystemFromEditorData);
        }
        else
        {
            LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::CreateTerrainSystem, (uint8*)nullptr, 0);
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd);

        AZ_Error("LegacyTerrain", isInstantiated, "Failed to initialize the legacy terrain system");
    }

    void LegacyTerrainLevelComponent::Deactivate()
    {
        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (!isInstantiated)
        {
            return;
        }

        //Before removing the terrain system from memory, it is important to make sure
        //there are no pending culling jobs because removing the terrain causes the Octree culling jobs
        //to recalculate and those jobs may access Octree nodes that don't exist anymore.
        if (gEnv)
        {
            gEnv->p3DEngine->WaitForCullingJobsCompletion();
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin);

        // If we're running in the Editor in Game Mode, the Editor terrain data needs to know that we're destroying the terrain system too.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            LegacyTerrainEditorDataRequestBus::Broadcast(&LegacyTerrainEditorDataRequests::DestroyTerrainSystem);
        }
        else
        {
            LegacyTerrainInstanceRequestBus::Broadcast(&LegacyTerrainInstanceRequests::DestroyTerrainSystem);
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd);
    }

    bool LegacyTerrainLevelComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const LegacyTerrainLevelConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool LegacyTerrainLevelComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<LegacyTerrainLevelConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }
}
