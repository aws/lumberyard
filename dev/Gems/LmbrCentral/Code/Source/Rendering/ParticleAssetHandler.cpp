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

#include "LmbrCentral_precompiled.h"

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/XML/rapidxml.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <LmbrCentral/Rendering/ParticleAsset.h>
#include "ParticleAssetHandler.h"

#define PARTICLE_LIB_EXT "xml"

namespace LmbrCentral
{
    bool LoadFromBuffer(ParticleAsset* data, char* buffer, size_t bufferSize)
    {
        using namespace AZ::rapidxml;

        xml_document<char> xmlDoc;
        xmlDoc.parse<parse_no_data_nodes>(buffer);

        xml_node<char>* root = xmlDoc.first_node();
        if (root)
        {
            xml_attribute<char>* nameAttribute = root->first_attribute("Name");
            if (nameAttribute)
            {
                const char* name = nameAttribute->value();
                if (name && name[0])
                {
                    xml_node<char>* particlesNode = root->first_node("Particles");
                    while (particlesNode)
                    {
                        nameAttribute = particlesNode->first_attribute("Name");
                        if (nameAttribute)
                        {
                            const char* effectName = nameAttribute->value();
                            if (effectName && effectName[0])
                            {
                                data->m_emitterNames.push_back(AZStd::string::format("%s.%s", name, effectName));
                            }
                        }

                        particlesNode = particlesNode->next_sibling("Particles");
                    }

                    return true;
                }
            }
        }

        return false;
    }

    ParticleAssetHandler::~ParticleAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr ParticleAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<ParticleAsset>::Uuid(), "Invalid asset type! We handle only 'ParticleAsset'");

        // Look up the asset path to ensure it's actually a particle library.
        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew ParticleAsset;
    }

    bool ParticleAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        // Load from preloaded stream.
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<ParticleAsset>::Uuid(), "Invalid asset type! We handle only 'ParticleAsset'");
        if (stream)
        {
            ParticleAsset* data = asset.GetAs<ParticleAsset>();

            const size_t sizeBytes = static_cast<size_t>(stream->GetLength());
            char* buffer = new char[sizeBytes];
            stream->Read(sizeBytes, buffer);

            bool loaded = LoadFromBuffer(data, buffer, sizeBytes);

            delete[] buffer;

            return loaded;
        }

        return false;
    }

    bool ParticleAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        bool loaded = false;

        // Load from CryPak.
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO is not initialized.");
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        const AZ::IO::Result result = fileIO->Open(assetPath, AZ::IO::OpenMode::ModeRead, fileHandle);
        if (result)
        {
            ParticleAsset* data = asset.GetAs<ParticleAsset>();

            AZ::u64 sizeBytes = 0;
            if (fileIO->Size(fileHandle, sizeBytes))
            {
                char* buffer = new char[sizeBytes];
                if (fileIO->Read(fileHandle, buffer, sizeBytes))
                {
                    loaded = LoadFromBuffer(data, buffer, sizeBytes);
                }

                delete[] buffer;
            }

            fileIO->Close(fileHandle);
        }

        return loaded;
    }

    void ParticleAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void ParticleAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<ParticleAsset>::Uuid());
    }

    void ParticleAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<ParticleAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<ParticleAsset>::Uuid());
    }

    void ParticleAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType ParticleAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<ParticleAsset>::Uuid();
    }

    const char* ParticleAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Particles";
    }

    const char* ParticleAssetHandler::GetGroup() const
    {
        return "Particles";
    }

    const char* ParticleAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Particle.png";
    }

    AZ::Uuid ParticleAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid("{0F35739E-1B40-4497-860D-D6FF5D87A9D9}");
    }

    void ParticleAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("xml");
    }

    bool ParticleAssetHandler::CanHandleAsset(const AZ::Data::AssetId& id) const
    {
        // Look up the asset path to ensure it's actually a particle library.
        AZStd::string assetPath;
        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);

        bool isParticleLibrary = false;

        if (!assetPath.empty())
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "FileIO is not initialized.");
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            const AZ::IO::Result result = fileIO->Open(assetPath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);
            if (result)
            {
                AZ::u64 fileSize = 0;
                if (fileIO->Size(fileHandle, fileSize) && fileSize)
                {
                    AZStd::vector<char> memoryBuffer;
                    memoryBuffer.resize_no_construct(fileSize);
                    if (fileIO->Read(fileHandle, memoryBuffer.begin(), fileSize, true))
                    {
                        isParticleLibrary = (nullptr != strstr(memoryBuffer.begin(), "ParticleLibrary"));
                    }
                }

                fileIO->Close(fileHandle);
            }
        }

        return isParticleLibrary;
    }
} // namespace LmbrCentral

