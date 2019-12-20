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


#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/FileTagAsset.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "FileTagComponent.h"


#include <AzCore/std/string/wildcard.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/XML/rapidxml.h>

namespace AzFramework
{
    namespace FileTag
    {
        void FileTagComponent::Activate()
        {
            m_fileTagManager.reset(aznew FileTagManager());
        }
        void FileTagComponent::Deactivate()
        {
            m_fileTagManager.reset();
        }
        void FileTagComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagComponent>()
                    ->Version(1);
            }
        }

        void FileTagQueryComponent::Activate()
        {
            m_fileTagBlackListQueryManager.reset(aznew FileTagQueryManager(FileTagType::BlackList));
            m_fileTagWhiteListQueryManager.reset(aznew FileTagQueryManager(FileTagType::WhiteList));
        }
        void FileTagQueryComponent::Deactivate()
        {
            m_fileTagBlackListQueryManager.reset();
            m_fileTagWhiteListQueryManager.reset();
        }

        void FileTagQueryComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagQueryComponent>()
                    ->Version(1);
            }
        }

        void BlackListFileComponent::Activate()
        {
            m_fileTagBlackListQueryManager.reset(aznew FileTagQueryManager(FileTagType::BlackList));
            if (!m_fileTagBlackListQueryManager.get()->Load())
            {
                AZ_Error("FileTagQueryComponent", false, "Not able to load default blacklist file (%s). Please make sure that it exists on disk.\n", FileTagQueryManager::GetDefaultFileTagFilePath(FileTagType::BlackList).c_str());
            }

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        void BlackListFileComponent::Deactivate()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            m_fileTagBlackListQueryManager.reset();
        }

        void BlackListFileComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlackListFileComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        void BlackListFileComponent::OnCatalogLoaded(const char* catalogFile)
        {
            AZ_UNUSED(catalogFile);

            AZStd::vector<AZStd::string> registeredAssetPaths;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(registeredAssetPaths, &AZ::Data::AssetCatalogRequests::GetRegisteredAssetPaths);

            const char* dependencyXmlPattern = "*_dependencies.xml";
            for (const AZStd::string& assetPath : registeredAssetPaths)
            {
                if (!AZStd::wildcard_match(dependencyXmlPattern, assetPath.c_str()))
                {
                    continue;
                }

                if (!m_fileTagBlackListQueryManager.get()->LoadEngineDependencies(assetPath))
                {
                    AZ_Error("BlackListFileComponent", false, "Failed to add assets referenced from %s to the black list", assetPath.c_str());
                }
            }
        }
    }
}
