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
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventDefinition.h>

namespace ScriptEvents
{
    class SystemComponent
        : public AZ::Component
        , protected ScriptEvents::ScriptEventBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{8BAD5292-56C3-4657-99F2-515A2BDE23C1}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        virtual void RegisterAssetHandler()
        {
            m_runtimeAssetHandler = AZStd::make_unique<ScriptEventAssetHandler>(ScriptEvents::ScriptEventsAsset::GetDisplayName(), ScriptEvents::ScriptEventsAsset::GetGroup(), ScriptEvents::ScriptEventsAsset::GetFileFilter(), AZ::AzTypeInfo<ScriptEvents::SystemComponent>::Uuid());

            AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
            if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
            {
                return; // Asset Type already handled
            }

            AZ::Data::AssetManager::Instance().RegisterHandler(m_runtimeAssetHandler.get(), assetType);

            // Use AssetCatalog service to register ScriptCanvas asset type and extension
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptEvents::ScriptEventsAsset::GetFileFilter());

        }

        virtual void UnregisterAssetHandler()
        {
            for (auto& asset : m_scriptEvents)
            {
                asset.second.reset();
            }

            m_scriptEvents.clear();

            if (m_runtimeAssetHandler)
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(m_runtimeAssetHandler.get());
                m_runtimeAssetHandler.reset();
            }
        }

    protected:

        ////////////////////////////////////////////////////////////////////////
        // ScriptEvents::ScriptEventBus::Handler
        AZStd::intrusive_ptr<Internal::ScriptEvent> RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        void RegisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition) override;
        AZStd::intrusive_ptr<Internal::ScriptEvent> GetScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        const FundamentalTypes* GetFundamentalTypes() override { return &m_fundamentalTypes; }
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>> m_runtimeAssetHandler;

        // Script Event Assets
        AZStd::unordered_map<ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>> m_scriptEvents;

        FundamentalTypes m_fundamentalTypes;
    };
}


