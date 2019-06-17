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

#include "precompiled.h"

#include "AssetGraphSceneData.h"

#include <Core/ScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    AssetGraphSceneData::AssetGraphSceneData(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
        : m_asset(scriptCanvasAsset)
    {
        m_tupleId.m_assetId = m_asset.GetId();
        AZ::Entity* sceneEntity = scriptCanvasAsset.Get() ? m_asset.Get()->GetScriptCanvasEntity() : nullptr;
        if (sceneEntity)
        {
            m_tupleId.m_scriptCanvasEntityId = sceneEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(m_tupleId.m_scriptCanvasGraphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
        }
    }

    void AssetGraphSceneData::Set(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        m_asset = scriptCanvasAsset;
        m_tupleId.m_assetId = scriptCanvasAsset.GetId();
        AZ::Entity* sceneEntity = scriptCanvasAsset.Get() ? m_asset.Get()->GetScriptCanvasEntity() : nullptr;
        if (sceneEntity)
        {
            m_tupleId.m_scriptCanvasEntityId = sceneEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(m_tupleId.m_scriptCanvasGraphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
        }
    }

    AZ::Outcome<void, AZStd::string> AssetGraphSceneMapper::Add(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset)
    {
        if (scriptCanvasAsset.GetId().IsValid())
        {
            auto assetIt = m_assetIdToDataMap.find(scriptCanvasAsset.GetId());
            if (assetIt != m_assetIdToDataMap.end())
            {
                assetIt->second->Set(scriptCanvasAsset);
            }
            else
            {
                m_assetIdToDataMap.emplace(scriptCanvasAsset.GetId(), AZStd::make_unique<AssetGraphSceneData>(scriptCanvasAsset));
            }

            return AZ::Success();
        }

        return AZ::Failure(AZStd::string("Script Canvas asset has invalid asset id"));
    }

    void AssetGraphSceneMapper::Remove(const AZ::Data::AssetId& assetId)
    {
        m_assetIdToDataMap.erase(assetId);
    }

    void AssetGraphSceneMapper::Clear()
    {
        m_assetIdToDataMap.clear();
    }

    AssetGraphSceneData* AssetGraphSceneMapper::GetByAssetId(const AZ::Data::AssetId& assetId) const
    {
        if (!assetId.IsValid())
        {
            return nullptr;
        }

        auto assetToGraphIt = m_assetIdToDataMap.find(assetId);
        if (assetToGraphIt != m_assetIdToDataMap.end())
        {
            return assetToGraphIt->second.get();
        }

        return nullptr;
    }

    AssetGraphSceneData* AssetGraphSceneMapper::GetByGraphId(AZ::EntityId graphId) const
    {
        if (!graphId.IsValid())
        {
            return nullptr;
        }

        auto graphToAssetIt = AZStd::find_if(m_assetIdToDataMap.begin(), m_assetIdToDataMap.end(), [&graphId](const AZStd::pair<AZ::Data::AssetId, AZStd::unique_ptr<AssetGraphSceneData>>& assetPair)
        {
            return assetPair.second->m_tupleId.m_scriptCanvasGraphId == graphId;
        });

        if (graphToAssetIt != m_assetIdToDataMap.end())
        {
            return graphToAssetIt->second.get();
        }

        return nullptr;
    }

    AssetGraphSceneData* AssetGraphSceneMapper::GetBySceneId(AZ::EntityId sceneId) const
    {
        AssetGraphSceneId tupleId;
        if (!sceneId.IsValid())
        {
            return nullptr;
        }

        auto sceneToAssetIt = AZStd::find_if(m_assetIdToDataMap.begin(), m_assetIdToDataMap.end(), [sceneId](const AZStd::pair<AZ::Data::AssetId, AZStd::unique_ptr<AssetGraphSceneData>>& assetPair)
        {
            return assetPair.second->m_tupleId.m_scriptCanvasEntityId == sceneId;
        });

        if (sceneToAssetIt != m_assetIdToDataMap.end())
        {
            return sceneToAssetIt->second.get();
        }
        return nullptr;
    }
}
