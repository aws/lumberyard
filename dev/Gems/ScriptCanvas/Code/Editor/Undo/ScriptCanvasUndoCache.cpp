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
#include <precompiled.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/Bus/UndoBus.h>
#include <Editor/Undo/ScriptCanvasUndoCache.h>

namespace ScriptCanvasEditor
{
    UndoCache::UndoCache()
    {
    }

    UndoCache::~UndoCache()
    {
    }

    void UndoCache::Clear()
    {
        m_dataMap.clear();
    }

    void UndoCache::PurgeCache(AZ::EntityId scriptCanvasEntityId)
    {
        m_dataMap.erase(scriptCanvasEntityId);
    }

    void UndoCache::PopulateCache(AZ::Entity* scriptCanvasEntity)
    {
        if (scriptCanvasEntity)
        {
            UpdateCache(scriptCanvasEntity->GetId());
        }
    }

    void UndoCache::UpdateCache(AZ::EntityId scriptCanvasEntityId)
    {
        // Lookup the graph item and perform a snapshot of all it's serializable elements
        UndoData undoData;
        UndoRequestBus::BroadcastResult(undoData, &UndoRequests::CreateUndoData, scriptCanvasEntityId);
      
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZStd::vector<AZ::u8>& newData = m_dataMap[scriptCanvasEntityId];
        newData.clear();
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&newData);
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *serializeContext, AZ::DataStream::ST_BINARY);
        if (!objStream->WriteClass(&undoData))
        {
            AZ_Assert(false, "Unable to serialize Script Canvas scene and graph data for undo/redo");
            return;
        }

        objStream->Finalize();
    }

    const AZStd::vector<AZ::u8>& UndoCache::Retrieve(AZ::EntityId scriptCanvasEntityId)
    {
        auto it = m_dataMap.find(scriptCanvasEntityId);

        if (it == m_dataMap.end())
        {
            return m_emptyData;
        }

        return it->second;
    }
}