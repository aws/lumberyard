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

#include <ScriptCanvas/Core/Graph.h>

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

    void UndoCache::PurgeCache(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        m_dataMap.erase(scriptCanvasId);
    }

    void UndoCache::PopulateCache(AZ::Entity* scriptCanvasEntity)
    {
        if (scriptCanvasEntity)
        {
            ScriptCanvas::Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(scriptCanvasEntity);

            if (graph)
            {
                UpdateCache(graph->GetScriptCanvasId());
            }
        }
    }

    void UndoCache::UpdateCache(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        // Lookup the graph item and perform a snapshot of all it's serializable elements
        UndoData undoData;
        UndoRequestBus::BroadcastResult(undoData, &UndoRequests::CreateUndoData, scriptCanvasId);
      
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZStd::vector<AZ::u8>& newData = m_dataMap[scriptCanvasId];
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

    const AZStd::vector<AZ::u8>& UndoCache::Retrieve(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        auto it = m_dataMap.find(scriptCanvasId);

        if (it == m_dataMap.end())
        {
            return m_emptyData;
        }

        return it->second;
    }
}