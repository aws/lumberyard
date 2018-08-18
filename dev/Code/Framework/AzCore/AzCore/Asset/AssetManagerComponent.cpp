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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    //=========================================================================
    // AssetDatabaseComponent
    // [6/25/2012]
    //=========================================================================
    AssetManagerComponent::AssetManagerComponent()
    {
    }

    //=========================================================================
    // Activate
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::Activate()
    {
        Data::AssetManager::Descriptor desc;
        Data::AssetManager::Create(desc);
        SystemTickBus::Handler::BusConnect();
    }

    //=========================================================================
    // Deactivate
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::Deactivate()
    {
        Data::AssetManager::Instance().DispatchEvents(); // clear any waiting assets.

        SystemTickBus::Handler::BusDisconnect();
        Data::AssetManager::Destroy();
    }

    //=========================================================================
    // OnTick
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::OnSystemTick()
    {
        Data::AssetManager::Instance().DispatchEvents();
    }

    //=========================================================================
    // GetProvidedServices
    //=========================================================================
    void AssetManagerComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    //=========================================================================
    // GetIncompatibleServices
    //=========================================================================
    void AssetManagerComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    //=========================================================================
    // GetDependentServices
    //=========================================================================
    void AssetManagerComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("DataStreamingService", 0xb1b981f5));
        dependent.push_back(AZ_CRC("JobsService", 0xd5ab5a50));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void AssetManagerComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Data::AssetId>()
                ->Version(1)
                ->Field("guid", &Data::AssetId::m_guid)
                ->Field("subId", &Data::AssetId::m_subId)
                ;

            serializeContext->Class<AZ::Data::AssetData>()
                ->Version(1)
                ;

            serializeContext->Class<AssetManagerComponent, AZ::Component>()
                ->Version(1)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AssetManagerComponent>(
                    "Asset Database", "Asset database system functionality")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }
}

#endif // #ifndef AZ_UNITY_BUILD
