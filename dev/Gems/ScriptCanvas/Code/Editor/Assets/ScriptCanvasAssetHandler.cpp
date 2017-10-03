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

#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <ScriptCanvas/Bus/DocumentContextBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>

#include <GraphCanvas/GraphCanvasBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/string/string_view.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasAssetHandler::ScriptCanvasAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(GetAssetType());
    }

    AZ::Data::AssetPtr ScriptCanvasAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        auto assetData = aznew ScriptCanvasAsset(id);
        // The Graph and Scene component live on the same entity
        AZ::Entity* scriptCanvasEntity{};
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(scriptCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateScene);
        if (scriptCanvasEntity)
        {
            scriptCanvasEntity->SetName("Script Canvas Graph");
            scriptCanvasEntity->CreateComponent<Graph>();
            assetData->SetScriptCanvasEntity(scriptCanvasEntity);
        }

        ScriptCanvasAssetFileInfo scFileInfo;
        scFileInfo.m_fileModificationState = ScriptCanvasFileState::NEW;
        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::RegisterScriptCanvasAsset, id, scFileInfo);
        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SetScriptCanvasAssetModificationState, id, scFileInfo.m_fileModificationState);
        return assetData;
    }

    bool ScriptCanvasAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
        AZ_Assert(scriptCanvasAsset, "This should be an scene slice asset, as this is the only type we process!");
        if (scriptCanvasAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, scriptCanvasAsset->GetScriptCanvasData(), m_serializeContext, assetLoadFilterCB);
            if (loadSuccess)
            {
                ScriptCanvasAssetFileInfo scFileInfo;
                scFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;
                DocumentContextRequestBus::Broadcast(&DocumentContextRequests::RegisterScriptCanvasAsset, asset.GetId(), scFileInfo);
                DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SetScriptCanvasAssetModificationState, asset.GetId(), scFileInfo.m_fileModificationState);
            }
            return loadSuccess;
        }
        return false;
    }

    bool ScriptCanvasAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        //ScriptCanvas files are source assets and should be placed in a source asset directory
        AZ::IO::FileIOStream stream;
        if (AzFramework::StringFunc::Path::IsRelative(assetPath))
        {
            bool pathFound = false;
            AZStd::string_view relPath = assetPath;

            AZStd::string fullAssetPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relPath, fullAssetPath);
            if (pathFound)
            {
                stream.Open(fullAssetPath.data(), AZ::IO::OpenMode::ModeRead);
            }
        }
        // If the asset could not be open from the source path or the asset path is absolute, then attempt to open the asset path.
        if (!stream.IsOpen())
        {
            stream.Open(assetPath, AZ::IO::OpenMode::ModeRead);
        }

        if (stream.IsOpen())
        {
            return LoadAssetData(asset, &stream, assetLoadFilterCB);
        }

        AZ_Error("Script Canvas", false, "Unable to open script canvas asset with relative path %s", assetPath);
        return false;
    }

    bool ScriptCanvasAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        return SaveAssetData(asset.GetAs<ScriptCanvasAsset>(), stream);
    }

    bool ScriptCanvasAssetHandler::SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream)
    {
        return SaveAssetData(assetData, stream, AZ::DataStream::ST_XML);
    }

    bool ScriptCanvasAssetHandler::SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream, AZ::DataStream::StreamType streamType)
    {
        if (assetData && m_serializeContext)
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, streamType);
            bool sliceSaved = objStream->WriteClass(&assetData->GetScriptCanvasData());
            objStream->Finalize();
            sliceSaved = stream->Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && sliceSaved;
            return sliceSaved;
        }

        return false;
    }

    void ScriptCanvasAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================.
    AZ::SerializeContext* ScriptCanvasAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    //=========================================================================
    // SetSerializeContext
    //=========================================================================.
    void ScriptCanvasAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "ScriptCanvasAssetHandler: No serialize context provided! We will not be able to process Graph Asset type");
            }
        }
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================.
    void ScriptCanvasAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(GetAssetType());
    }

    //=========================================================================
    // GetAssetType
    //=========================================================================.
    AZ::Data::AssetType ScriptCanvasAssetHandler::GetAssetType() const
    {
        return ScriptCanvasAssetHandler::GetAssetTypeStatic();
    }

    const char* ScriptCanvasAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Script Canvas";
    }

    AZ::Data::AssetType ScriptCanvasAssetHandler::GetAssetTypeStatic()
    {
        return azrtti_typeid<ScriptCanvasAsset>();
    }

    //=========================================================================
    // GetAssetTypeExtensions
    //=========================================================================.
    void ScriptCanvasAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(GetFileExtension());
    }

    const char* ScriptCanvasAssetHandler::GetGroup() const
    {
        return "Script";
    }

    const char* ScriptCanvasAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png";
    }

    AZ::Uuid ScriptCanvasAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<EditorScriptCanvasComponent>();
    }

} // namespace ScriptCanvasEditor
