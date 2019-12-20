
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

#include <AzTestShared/Utils/Utils.h>
#include "AzCore/Component/Entity.h"
#include "AzCore/Asset/AssetManager.h"
#include "AzCore/Slice/SliceComponent.h"

namespace UnitTest
{
    AZStd::string GetTestFolderPath()
    {
        return AZ_TRAIT_TEST_ROOT_FOLDER;
    }

    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s", GetTestFolderPath().c_str(), fileName);
    }
}

namespace AZ
{
    namespace Test
    {
        AZ::Data::Asset<AZ::SliceAsset> CreateSliceFromComponent(AZ::Component* assetComponent, AZ::Data::AssetId sliceAssetId)
        {
            auto* sliceEntity = aznew AZ::Entity;
            auto* contentEntity = aznew AZ::Entity;

            if (assetComponent)
            {
                contentEntity->AddComponent(assetComponent);
            }

            auto sliceAsset = Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(sliceAssetId);

            AZ::SliceComponent* component = sliceEntity->CreateComponent<AZ::SliceComponent>();
            component->SetIsDynamic(true);
            sliceAsset.Get()->SetData(sliceEntity, component);
            component->AddEntity(contentEntity);

            return sliceAsset;
        }

        AssertAbsorber::AssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        AssertAbsorber::~AssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        bool AssertAbsorber::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnAssert(const char* message)
        {
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnError(const char* window, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnWarning(const char* window, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            return true;
        }

    }// namespace Test
}// namespace AZ
