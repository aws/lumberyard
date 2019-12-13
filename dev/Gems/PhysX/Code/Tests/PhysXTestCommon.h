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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Shape.h>

#include <PhysX/HeightFieldAsset.h>

namespace PhysX
{
    using EntityPtr = AZStd::unique_ptr<AZ::Entity>;

    namespace TestUtils
    {
        void UpdateDefaultWorld(float timeStep, AZ::u32 numSteps);

        AZ::Data::Asset<PhysX::Pipeline::HeightFieldAsset> CreateHeightField(const AZStd::vector<uint16_t>& samples, int numRows, int numCols);

        EntityPtr CreateFlatTestTerrain(float width = 1.0f, float depth = 1.0f);

        EntityPtr CreateFlatTestTerrainWithMaterial(float width = 1.0f, float depth = 1.0f, const Physics::MaterialSelection& materialSelection = Physics::MaterialSelection());

        EntityPtr CreateSlopedTestTerrain(float width = 1.0f, float depth = 1.0f, float height = 1.0f);

        EntityPtr CreateSphereEntity(const AZ::Vector3& position, const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);

        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);
    }
}