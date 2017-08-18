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

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include <LmbrCentral/Scripting/SpawnerComponentBus.h>

namespace LmbrCentral
{
    /**
    * SpawnerComponent
    *
    * SpawnerComponent facilitates spawning of a design-time selected or run-time provided "*.dynamicslice" at an entity's location with an optional offset.
    */
    class SpawnerComponent
        : public AZ::Component
        , private SpawnerComponentRequestBus::Handler
        , private AzFramework::SliceInstantiationResultBus::MultiHandler
    {
    public:
        AZ_COMPONENT(SpawnerComponent, "{8022A627-FA7D-4516-A155-657A0927A3CA}");

        SpawnerComponent();
        ~SpawnerComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SpawnerComponentRequestBus::Handler
        AzFramework::SliceInstantiationTicket Spawn() override;
        AzFramework::SliceInstantiationTicket SpawnRelative(const AZ::Transform& relative) override;
        AzFramework::SliceInstantiationTicket SpawnAbsolute(const AZ::Transform& world) override;
        AzFramework::SliceInstantiationTicket SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice) override;
        AzFramework::SliceInstantiationTicket SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative) override;
        AzFramework::SliceInstantiationTicket SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus::MultiHandler
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Private helpers
        AzFramework::SliceInstantiationTicket SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative);
        //////////////////////////////////////////////////////////////////////////

        // Serialized members
        AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset;
        bool m_spawnOnActivate = false;
    };
} // namespace LmbrCentral
