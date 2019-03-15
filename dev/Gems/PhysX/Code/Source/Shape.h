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

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

namespace Physics
{
    class Material;
}

namespace PhysX
{
    class Material;

    class Shape
        : public Physics::Shape
        , public AZStd::enable_shared_from_this<Shape>
    {
    public:
        AZ_CLASS_ALLOCATOR(Shape, AZ::SystemAllocator, 0);
        AZ_RTTI(Shape, "{0A47DDD6-2BD7-43B3-BF0D-2E12CC395C13}", Physics::Shape);

        Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration);
        Shape(physx::PxShape* nativeShape);

        Shape(Shape&& shape);
        Shape& operator=(Shape&& shape);
        Shape(const Shape& shape) = delete;
        Shape& operator=(const Shape& shape) = delete;

        physx::PxShape* GetPxShape();

        void SetMaterial(const AZStd::shared_ptr<Physics::Material>& material) override;
        AZStd::shared_ptr<Physics::Material> GetMaterial() const override;

        void SetMaterials(const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& materials);
        const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& GetMaterials();

        void SetCollisionLayer(const Physics::CollisionLayer& layer) override;
        Physics::CollisionLayer GetCollisionLayer() const override;

        void SetCollisionGroup(const Physics::CollisionGroup& group) override;
        Physics::CollisionGroup GetCollisionGroup() const override;

        void SetName(const char* name) override;
        
        void SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation) override;

        void* GetNativePointer() override;

    private:
        void BindMaterialsWithPxShape();
        void ExtractMaterialsFromPxShape();

        using PxShapeUniquePtr = AZStd::unique_ptr<physx::PxShape, AZStd::function<void(physx::PxShape*)>>;
        Shape() = default;

        PxShapeUniquePtr m_pxShape;
        AZStd::vector<AZStd::shared_ptr<PhysX::Material>> m_materials;
        Physics::CollisionLayer m_collisionLayer;
        Physics::CollisionGroup m_collisionGroup;
    };
}