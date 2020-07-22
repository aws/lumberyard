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

#include "PhysXTestFixtures.h"
#include "EditorTestUtilities.h"
#include "PhysXTestCommon.h"
#include <AzFramework/Physics/TerrainBus.h> 

#include <Shape.h>
#include <Utils.h>

namespace PhysXEditorTests
{
    bool NormalPointsAwayFromPosition(const AZ::Vector3& vertexA, const AZ::Vector3& vertexB, const AZ::Vector3& vertexC, const AZ::Vector3& position)
    {
        const AZ::Vector3 edge1 = (vertexB - vertexA).GetNormalized();
        const AZ::Vector3 edge2 = (vertexC - vertexA).GetNormalized();
        const AZ::Vector3 normal = edge1.Cross(edge2);
        return normal.Dot(vertexA - position) >= 0.f;
    }

    bool TriangleWindingOrderIsValid(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Vector3& insidePosition)
    {
        if (!vertices.empty() && !indices.empty())
        {
            // use indices to construct triangles
            for (auto i = 0; i < indices.size(); i+=3)
            {
                const AZ::Vector3 vertexA = vertices[indices[i + 0]];
                const AZ::Vector3 vertexB = vertices[indices[i + 1]];
                const AZ::Vector3 vertexC = vertices[indices[i + 2]];
                if (!NormalPointsAwayFromPosition(vertexA, vertexB, vertexC, insidePosition))
                {
                    return false;
                }
            }
        }
        else if (!vertices.empty())
        {
            // assume a triangle list order
            for (auto i = 0; i < vertices.size(); i+=3)
            {
                const AZ::Vector3 vertexA = vertices[i + 0];
                const AZ::Vector3 vertexB = vertices[i + 1];
                const AZ::Vector3 vertexC = vertices[i + 2];
                if (!NormalPointsAwayFromPosition(vertexA, vertexB, vertexC, insidePosition))
                {
                    return false;
                }
            }
        }

        return true;
    }

    TEST_F(PhysXEditorFixture, BoxShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        const AZ::Vector3 boxDimensions(1.f,1.f,1.f);

        // Given there is a box shape
        auto physicsSystem = AZ::Interface<Physics::System>::Get();
        AZ_Assert(physicsSystem, "Physics System required");

        auto shape = physicsSystem->CreateShape(Physics::ColliderConfiguration(), 
            Physics::BoxShapeConfiguration(boxDimensions));
        
        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.

        // valid number of vertices and indices
        EXPECT_TRUE(vertices.size() == 8);

        // 6 sides, 2 triangles per side, 3 indices per triangle
        EXPECT_TRUE(indices.size() == 6 * 2 * 3);

        // all vertices are inside dimensions AABB
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        for (auto vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        EXPECT_TRUE(bounds.GetWidth().IsClose(boxDimensions.GetX()));
        EXPECT_TRUE(bounds.GetDepth().IsClose(boxDimensions.GetY()));
        EXPECT_TRUE(bounds.GetHeight().IsClose(boxDimensions.GetZ()));

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, SphereShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is a sphere shape
        constexpr float radius = 1.f;
        auto shape = AZ::Interface<Physics::System>::Get()->CreateShape(Physics::ColliderConfiguration(), 
            Physics::SphereShapeConfiguration(radius));

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid radius from center
        bool vertexRadiusIsValid = true;
        for (auto vertex : vertices)
        {
            if (!vertex.GetLength().IsClose(radius))
            {
                vertexRadiusIsValid = false;
                break;
            }
        }
        EXPECT_TRUE(vertexRadiusIsValid);

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, CapsuleShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is a shape
        constexpr float height = 1.f;
        constexpr float radius = .25f;
        auto shape = AZ::Interface<Physics::System>::Get()->CreateShape(Physics::ColliderConfiguration(), 
            Physics::CapsuleShapeConfiguration(height, radius));

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // all vertices are inside dimensions AABB
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        for (auto vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        constexpr float halfHeight = height * .5f;
        const AZ::Vector3 min(-radius,-radius,-halfHeight);
        const AZ::Vector3 max(radius,radius,halfHeight);
        const AZ::Aabb expectedBounds = AZ::Aabb::CreateFromMinMax(min, max);
        EXPECT_TRUE(expectedBounds.Contains(bounds));

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, TerrainShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is terrain 
        constexpr float width = 2.f;
        constexpr float depth = 2.f;
        constexpr float height = 4.f;

        // SlopedTestTerrain creates a 2x2 quad terrain that is sloped from 0,0,0 to 2,2,4 
        auto terrain = PhysX::TestUtils::CreateSlopedTestTerrain();
        Physics::RigidBodyStatic* terrainBody = nullptr;
        Physics::TerrainRequestBus::BroadcastResult(terrainBody, &Physics::TerrainRequests::GetTerrainTile, 0.f, 0.f);
        AZ_Assert(terrainBody, "Failed to get terrain tile");

        auto terrainShape = terrainBody->GetShape(0);
        AZ_Assert(terrainShape, "Failed to get terrain shape");

        // When geometry is requested
        terrainShape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        bool verticesWithinBounds = true;
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        for (const auto& vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        EXPECT_TRUE(bounds.GetWidth().IsClose(width));
        EXPECT_TRUE(bounds.GetDepth().IsClose(height)); // AZ considers z to be depth
        EXPECT_TRUE(bounds.GetHeight().IsClose(depth)); // AZ considers y to be height

        // selecting a subset of the terrain geometry is also valid 
        vertices.clear();
        indices.clear();

        const AZ::Vector3 minBounds = AZ::Vector3(0.f, 0.f, 0.f);
        const AZ::Vector3 maxBounds = AZ::Vector3(1.2f, 1.f, height); // z bound not respected for heightfields
        AZ::Aabb optionalBounds = AZ::Aabb::CreateFromMinMax(minBounds, maxBounds);
        terrainShape->GetGeometry(vertices, indices, &optionalBounds);

        EXPECT_FALSE(vertices.empty());

        bounds.SetNull();
        for (const auto& vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        // even though we specified an x bound of 1.2 we should get back the full terrain quad or 
        // we will have a gap in the mesh at the edge
        EXPECT_TRUE(bounds.GetMax().GetX() > optionalBounds.GetMax().GetX());
        EXPECT_TRUE(bounds.GetWidth().IsClose(2.f));
        EXPECT_TRUE(bounds.GetHeight().IsClose(maxBounds.GetY())); // AZ considers y to be height
        EXPECT_TRUE(bounds.GetDepth().IsClose(3.f)); // max height (z) should be 3 for this sloping terrain

        // valid winding order
        const AZ::Vector3 positionUnderTerrain = AZ::Vector3(0.f,0.f,-1.f);
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, positionUnderTerrain));
    }

    TEST_F(PhysXEditorFixture, ConvexHullShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        auto physicsSystem = AZ::Interface<Physics::System>::Get();

        // Given there is a shape
        const PhysX::PointList testPoints = PhysX::TestUtils::GeneratePyramidPoints(1.0f);
        AZStd::vector<AZ::u8> cookedData;
        EXPECT_TRUE(physicsSystem->CookConvexMeshToMemory(testPoints.data(), 
            static_cast<AZ::u32>(testPoints.size()), cookedData));

        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(), 
            Physics::CookedMeshShapeConfiguration::MeshType::Convex);
        auto shape = physicsSystem->CreateShape(Physics::ColliderConfiguration(), shapeConfig);
        
        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, TriangleMeshShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        auto physicsSystem = AZ::Interface<Physics::System>::Get();

        // Given there is a shape
        PhysX::VertexIndexData cubeMeshData = PhysX::TestUtils::GenerateCubeMeshData(3.0f);
        AZStd::vector<AZ::u8> cookedData;
        physicsSystem->CookTriangleMeshToMemory(
            cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
            cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
            cookedData);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(), 
            Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

        // we need a valid triangle mesh shape
        auto shape = physicsSystem->CreateShape(Physics::ColliderConfiguration(), shapeConfig);

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }
} // namespace UnitTest