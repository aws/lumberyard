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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/IO/FileIO.h>
#include <EMotionFX/Source/Allocators.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        /**
         *
         */
        class EMotionFXAsset : public AZ::Data::AssetData
        {
        public:
            AZ_RTTI(EMotionFXAsset, "{043F606A-A483-4910-8110-D8BC4B78922C}", AZ::Data::AssetData)
            AZ_CLASS_ALLOCATOR(EMotionFXAsset, EMotionFXAllocator, 0)

            AZStd::vector<AZ::u8> m_emfxNativeData;
        };

        /**
         *
         */
        template<typename DataType>
        class EMotionFXAssetHandler
            : public AZ::Data::AssetHandler
            , private AZ::AssetTypeInfoBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(EMotionFXAssetHandler<DataType>, EMotionFXAllocator, 0)

            EMotionFXAssetHandler()
            {
                Register();
            }

            ~EMotionFXAssetHandler() override
            {
                Unregister();
            }

            AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
            {
                (void)id;
                (void)type;
                return aznew DataType();
            }

            bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                (void)assetLoadFilterCB;

                DataType* assetData = asset.GetAs<DataType>();

                if (stream->GetLength() > 0)
                {
                    assetData->m_emfxNativeData.resize(stream->GetLength());
                    stream->Read(stream->GetLength(), assetData->m_emfxNativeData.data());

                    return true;
                }

                return false;
            }

            bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                (void)assetLoadFilterCB;

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

            bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override
            {
                (void)asset;
                (void)stream;
                AZ_Error("EMotionFX", false, "Asset handler does not support asset saving.");
                return false;
            }

            void DestroyAsset(AZ::Data::AssetPtr ptr) override
            {
                delete ptr;
            }

            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
            {
                assetTypes.push_back(azrtti_typeid<DataType>());
            }

            void Register()
            {
                AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset database isn't ready!");
                AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<DataType>());

                AZ::AssetTypeInfoBus::Handler::BusConnect(azrtti_typeid<DataType>());
            }

            void Unregister()
            {
                AZ::AssetTypeInfoBus::Handler::BusDisconnect(azrtti_typeid<DataType>());

                if (AZ::Data::AssetManager::IsReady())
                {
                    AZ::Data::AssetManager::Instance().UnregisterHandler(this);
                }
            }

            void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override
            {
                if (!loadStageSucceeded || !OnInitAsset(asset))
                {
                    AssetHandler::InitAsset(asset, false, isReload);
                    return;
                }

                AssetHandler::InitAsset(asset, true, isReload);
            }

            virtual bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
            {
                (void)asset;
                return true;
            }

            const char* GetGroup() const override
            {
                return "Animation";
            }
        };
    } // namespace Integration
} // namespace EMotionFX

