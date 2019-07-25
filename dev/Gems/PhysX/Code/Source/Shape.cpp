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
#include <PhysX/Utils.h>
#include <Source/Utils.h>
#include <Source/Shape.h>
#include <Source/Material.h>
#include <Source/Collision.h>
#include <AzFramework/Physics/Material.h>

namespace PhysX
{
    Shape::Shape(Shape&& shape)
        : m_pxShape(AZStd::move(shape.m_pxShape))
        , m_materials(AZStd::move(shape.m_materials))
        , m_collisionLayer(AZStd::move(shape.m_collisionLayer))
        , m_collisionGroup(AZStd::move(shape.m_collisionGroup))
    {
        if (m_pxShape)
        {
            m_pxShape->userData = this;
        }
    }

    Shape& Shape::operator=(Shape&& shape)
    {
        m_pxShape = AZStd::move(shape.m_pxShape);
        m_materials = AZStd::move(shape.m_materials);
        m_collisionLayer = AZStd::move(shape.m_collisionLayer);
        m_collisionGroup = AZStd::move(shape.m_collisionGroup);

        if (m_pxShape)
        {
            m_pxShape->userData = this;
        }

        return *this;
    }

    auto releasePxShape = [](physx::PxShape* shape)
    {
        shape->userData = nullptr;
        shape->release();
    };

    Shape::Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        : m_collisionLayer(colliderConfiguration.m_collisionLayer)
    {
        if (physx::PxShape* newShape = Utils::CreatePxShapeFromConfig(colliderConfiguration, shapeConfiguration, m_collisionGroup))
        {
            m_pxShape = PxShapeUniquePtr(newShape, releasePxShape);
            m_pxShape->userData = this;

            ExtractMaterialsFromPxShape();
        }
    }

    Shape::Shape(physx::PxShape* nativeShape)
    {
        m_pxShape = PxShapeUniquePtr(nativeShape, releasePxShape);
        m_pxShape->acquireReference();
        m_pxShape->userData = this;

        ExtractMaterialsFromPxShape();
    }

    physx::PxShape* Shape::GetPxShape()
    {
        if (m_pxShape)
        {
            return m_pxShape.get();
        }
        return nullptr;
    }

    void Shape::SetMaterial(const AZStd::shared_ptr<Physics::Material>& material)
    {
        if (auto materialWrapper = AZStd::rtti_pointer_cast<PhysX::Material>(material))
        {
            m_materials.clear();
            m_materials.emplace_back(materialWrapper);

            BindMaterialsWithPxShape();
        }
        else
        {
            AZ_Warning("PhysX Shape", false, "Trying to assign material of unknown type");
        }
    }

    AZStd::shared_ptr<Physics::Material> Shape::GetMaterial() const
    {
        if (!m_materials.empty())
        {
            return m_materials[0];
        }
        return nullptr;
    }

    void Shape::SetMaterials(const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& materials)
    {
        m_materials = materials;

        BindMaterialsWithPxShape();
    }


    void Shape::BindMaterialsWithPxShape()
    {
        if (m_pxShape)
        {
            AZStd::vector<physx::PxMaterial*> pxMaterials;
            pxMaterials.reserve(m_materials.size());

            for (const auto& material : m_materials)
            {
                pxMaterials.emplace_back(material->GetPxMaterial());
            }

            AZ_Warning("PhysX Shape", m_materials.size() < std::numeric_limits<AZ::u16>::max(), "Trying to assign too many materials, cutting down");
            size_t materialsCount = AZStd::GetMin(m_materials.size() - 1, static_cast<size_t>(std::numeric_limits<AZ::u16>::max()));
            m_pxShape->setMaterials(&pxMaterials[0], static_cast<physx::PxU16>(materialsCount));
        }
    }

    void Shape::ExtractMaterialsFromPxShape()
    {
        if (m_pxShape == nullptr)
        {
            return;
        }
        const int BufferSize = 100;

        AZ_Warning("PhysX Shape", m_pxShape->getNbMaterials() < BufferSize, "Shape has too many materials, consider increasing the buffer");

        physx::PxMaterial* assignedMaterials[BufferSize];
        int materialsCount = m_pxShape->getMaterials(assignedMaterials, BufferSize, 0);

        m_materials.clear();
        m_materials.reserve(materialsCount);

        for (int i = 0; i < materialsCount; ++i)
        {
            if (assignedMaterials[i]->userData == nullptr)
            {
                AZ_Warning("PhysX Shape", false, "Trying to assign material with no user data. Make sure you are creating materials using MaterialManager");
                continue;
            }

            m_materials.push_back(static_cast<PhysX::Material*>(PhysX::Utils::GetUserData(assignedMaterials[i]))->shared_from_this());
        }
    }

    const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& Shape::GetMaterials()
    {
        return m_materials;
    }

    void Shape::SetCollisionLayer(const Physics::CollisionLayer& layer)
    {
        m_collisionLayer = layer;
        physx::PxFilterData filterData = m_pxShape->getSimulationFilterData();
        Collision::SetLayer(layer, filterData);
        m_pxShape->setSimulationFilterData(filterData);
        m_pxShape->setQueryFilterData(filterData);
    }

    Physics::CollisionLayer Shape::GetCollisionLayer() const
    {
        return m_collisionLayer;
    }

    void Shape::SetCollisionGroup(const Physics::CollisionGroup& group)
    {
        m_collisionGroup = group;
        physx::PxFilterData filterData = m_pxShape->getSimulationFilterData();
        Collision::SetGroup(m_collisionGroup, filterData);
        m_pxShape->setSimulationFilterData(filterData);
        m_pxShape->setQueryFilterData(filterData);
    }

    Physics::CollisionGroup Shape::GetCollisionGroup() const
    {
        return m_collisionGroup;
    }

    void Shape::SetName(const char* name)
    {
        if (m_pxShape)
        {
            m_pxShape->setName(name);
        }
    }

    void Shape::SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation)
    {
        physx::PxTransform pxShapeTransform = PxMathConvert(offset, rotation);
        AZ_Warning("Physics::Shape", m_pxShape->isExclusive(), "Non-exclusive shapes are not mutable after they're attached to a body.");
        if (m_pxShape->getGeometryType() == physx::PxGeometryType::eCAPSULE)
        {
            physx::PxQuat lyToPxRotation(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
            pxShapeTransform.q *= lyToPxRotation;
        }
        m_pxShape->setLocalPose(pxShapeTransform);
    }

    void* Shape::GetNativePointer()
    {
        return m_pxShape.get();
    }

    bool Shape::IsTrigger() const
    {
        if (m_pxShape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE)
        {
            return true;
        }
        return false;
    }
}