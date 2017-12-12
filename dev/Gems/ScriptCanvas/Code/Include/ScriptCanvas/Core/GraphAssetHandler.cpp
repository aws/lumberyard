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

#include "Graph.h"
#include "GraphAsset.h"
#include "GraphAssetHandler.h"

#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>


namespace ScriptCanvas
{
    //=========================================================================
    // GraphAssetHandler
    //=========================================================================
    GraphAssetHandler::GraphAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<GraphAsset>::Uuid());
    }

    //=========================================================================
    // CreateAsset
    //=========================================================================
    AZ::Data::AssetPtr GraphAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<GraphAsset>::Uuid(), "This handler deals only with GraphAsset type!");

        return aznew GraphAsset(id);
    }

    //=========================================================================
    // LoadAssetData
    //=========================================================================
    bool GraphAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        GraphAsset* graphAsset = asset.GetAs<GraphAsset>();
        AZ_Assert(graphAsset, "This should be a Script Canvas graph asset, as this is the only type we process!");
        if (graphAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, graphAsset->m_scriptCanvasData, m_serializeContext, assetLoadFilterCB);
            return loadSuccess;
        }
        return false;
    }

    //=========================================================================
    // LoadAssetData
    // Load through IFileIO
    //=========================================================================
    bool GraphAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            AZ::IO::FileIOStream stream(assetPath, AZ::IO::OpenMode::ModeRead);
            if (stream.IsOpen())
            {
                return LoadAssetData(asset, &stream, assetLoadFilterCB);
            }
        }

        return false;
    }

    //=========================================================================
    // SaveAssetData
    //=========================================================================
    bool GraphAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        GraphAsset* graphAsset = asset.GetAs<GraphAsset>();
        AZ_Assert(graphAsset, "This should be a Script Canvas graph asset, as this is the only type we process!");
        if (graphAsset && m_serializeContext)
        {
            AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create(stream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            bool graphSaved = binaryObjStream->WriteClass(&graphAsset->m_scriptCanvasData);
            binaryObjStream->Finalize();
            return graphSaved;
        }

        return false;
    }

    //=========================================================================
    // DestroyAsset
    //=========================================================================
    void GraphAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================.
    void GraphAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<GraphAsset>::Uuid());
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================.
    AZ::SerializeContext* GraphAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    //=========================================================================
    // SetSerializeContext
    //=========================================================================.
    void GraphAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "GraphAssetHandler: No serialize context provided! We will not be able to process Graph Asset type");
            }
        }
    }

    //=========================================================================
    // GetAssetTypeExtensions
    //=========================================================================.
    void GraphAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<GraphAsset>::Uuid())
        {
            extensions.push_back(GraphAsset::GetFileExtension());
        }
    }

    //=========================================================================
    // GetAssetType
    //=========================================================================.
    AZ::Data::AssetType GraphAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<GraphAsset>::Uuid();
    }


} // namespace AZ
