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

#include "ScriptEventBase.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                void ScriptEventEntry::Reflect(AZ::ReflectContext* context)
                {
                    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
                    {
                        if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<AZ::Crc32, ScriptEventEntry> >::GetGenericInfo())
                        {
                            genericClassInfo->Reflect(serialize);
                        }

                        serialize->Class<ScriptEventEntry>()
                            ->Version(1)
                            ->Field("m_scriptEventAssetId", &ScriptEventEntry::m_scriptEventAssetId)
                            ->Field("m_eventName", &ScriptEventEntry::m_eventName)
                            ->Field("m_eventSlotId", &ScriptEventEntry::m_eventSlotId)
                            ->Field("m_resultSlotId", &ScriptEventEntry::m_resultSlotId)
                            ->Field("m_parameterSlotIds", &ScriptEventEntry::m_parameterSlotIds)
                            ->Field("m_numExpectedArguments", &ScriptEventEntry::m_numExpectedArguments)
                            ->Field("m_resultEvaluated", &ScriptEventEntry::m_resultEvaluated)
                            ;
                    }
                }

                ScriptEventBase::ScriptEventBase()
                    : m_version(0)
                    , m_scriptEventAssetId(0)
                {
                }

                void ScriptEventBase::OnInit()
                {
                    ScriptCanvas::ScriptEventNodeRequestBus::Handler::BusConnect(GetEntityId());

                    if (m_scriptEventAssetId.IsValid())
                    {
                        Initialize(m_scriptEventAssetId);
                    }
                }

                void ScriptEventBase::Initialize(const AZ::Data::AssetId assetId)
                {
                    if (assetId.IsValid())
                    {
                        m_scriptEventAssetId = assetId;

                        GraphRequestBus::Event(GetGraphId(), &GraphRequests::AddDependentAsset, GetEntityId(), azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), assetId);

                        AZ::Data::AssetBus::Handler::BusConnect(assetId);
                    }

                    AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, false, nullptr, true);
                }

                void ScriptEventBase::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId());
                    m_definition = assetData.Get()->m_definition;
                    OnScriptEventReady(asset);
                }

                void ScriptEventBase::OnAssetChanged(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&, void* userData)
                {
                    ScriptEventBase* node = static_cast<ScriptEventBase*>(userData);
                    (void)node;
                }

                void ScriptEventBase::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId());
                    ScriptEvents::ScriptEvent& definition = assetData.Get()->m_definition;
                    if (definition.GetVersion() > m_version)
                    {
                        // The asset has changed.
                        AZ_Warning("Script Canvas", false, "The Script Event (%s) asset has changed, graphs that depend on it may require updates.", definition.GetName().c_str());
                        ScriptEvents::ScriptEventBus::BroadcastResult(m_scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, m_scriptEventAssetId, definition.GetVersion());
                    }

                    m_definition = definition;
                 }

                void ScriptEventBase::OnActivate()
                {
                    AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId, true, nullptr, true);
                }

                void ScriptEventBase::OnDeactivate()
                {
                    ScriptCanvas::ScriptEventNodeRequestBus::Handler::BusDisconnect();

                    if (AZ::Data::AssetBus::Handler::BusIsConnectedId(m_scriptEventAssetId))
                    {
                        AZ::Data::AssetBus::Handler::BusDisconnect(m_scriptEventAssetId);
                    }
                }

                AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> ScriptEventBase::GetAsset() const
                {
                    return AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId);
                }

            } // namespace Internal
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Core/ScriptEventBase.generated.cpp>
