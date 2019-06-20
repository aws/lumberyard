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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include "ScriptEventDefinition.h"
#include <AzFramework/Asset/GenericAssetHandler.h>
#include "ScriptEventsBus.h"
#include "ScriptEvent.h"

namespace ScriptEvents
{
    class ScriptEventsAsset
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(ScriptEventsAsset, "{CB4D603E-8CB0-4D80-8165-4244F28AF187}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ScriptEventsAsset, AZ::SystemAllocator, 0);
        
        ScriptEventsAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
            
        }

        ScriptEventsAsset(const ScriptEventsAsset& rhs)
        {
            m_definition = rhs.m_definition;
        }
        ~ScriptEventsAsset() = default;

        static const char* GetDisplayName() { return "Script Events"; }
        static const char* GetGroup() { return "ScriptEvents"; }
        static const char* GetFileFilter() { return "scriptevents"; }

        AZ::Crc32 GetBusId() const { return AZ::Crc32(GetId().ToString<AZStd::string>().c_str()); }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptEventsAsset>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Field("m_definition", &ScriptEventsAsset::m_definition)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ScriptEventsAsset>("Script Events Asset", "")
                        ->DataElement(0, &ScriptEventsAsset::m_definition, "Definition", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }

        ScriptEvents::ScriptEvent m_definition;
    };

    class ScriptEventAssetHandler : public AzFramework::GenericAssetHandler<ScriptEventsAsset>
    {
    public:
        AZ_RTTI(ScriptEventAssetHandler, "{002E913D-339A-4238-BCCD-ED077BBD72C5}", AzFramework::GenericAssetHandler<ScriptEventsAsset>);

        ScriptEventAssetHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(), AZ::SerializeContext* serializeContext = nullptr)
            : AzFramework::GenericAssetHandler<ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
        {
        }

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            if (AzFramework::GenericAssetHandler<ScriptEventsAsset>::LoadAssetData(asset, stream, assetLoadFilterCB))
            {
                // Now register it.
                const ScriptEvents::ScriptEvent& definition = asset.GetAs<ScriptEventsAsset>()->m_definition;
                AZStd::intrusive_ptr<Internal::ScriptEvent> scriptEvent;
                ScriptEvents::ScriptEventBus::BroadcastResult(scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, asset.GetId(), definition.GetVersion());
                
                return true;
            }
            return false;
        }
    };

}

