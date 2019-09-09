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

#include <PhysX_precompiled.h>

#include <Source/Pipeline/CgfMeshBuilder/CgfMeshAssetBuilderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <ISystem.h>

namespace PhysX
{
    namespace Pipeline
    {
        void CgfMeshAssetBuilderComponent::Activate()
        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;

            builderDescriptor.m_name = "Legacy CGF to PhysX Builder";
            builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.cgf", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            builderDescriptor.m_busId = azrtti_typeid<CgfMeshAssetBuilderWorker>();
            builderDescriptor.m_createJobFunction = AZStd::bind(&CgfMeshAssetBuilderWorker::CreateJobs, &m_meshAssetBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&CgfMeshAssetBuilderWorker::ProcessJob, &m_meshAssetBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

            m_meshAssetBuilder.BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
        }

        void CgfMeshAssetBuilderComponent::Deactivate()
        {
            m_meshAssetBuilder.BusDisconnect();
        }

        void CgfMeshAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<CgfMeshAssetBuilderComponent, AZ::Component>()
                    ->Version(0)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                    ;
            }
        }

        void CgfMeshAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("CgfPxMeshAssetBuilder", 0x154f57e6));
        }

        void CgfMeshAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("CgfPxMeshAssetBuilder", 0x154f57e6));
        }
    }
}