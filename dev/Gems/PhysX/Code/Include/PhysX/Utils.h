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

#include <PxActor.h>
#include <PxMaterial.h>
#include <PxShape.h>
#include <PhysX/UserDataTypes.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Collision.h>

namespace Physics
{
    class Material;
    class Shape;
}

namespace PhysX
{
    namespace Utils
    {
        ActorData* GetUserData(const physx::PxActor* actor);
        Physics::Material* GetUserData(const physx::PxMaterial* material);
        Physics::Shape* GetUserData(const physx::PxShape* shape);
        Physics::World* GetUserData(physx::PxScene* scene);
        void SetLayer(const Physics::CollisionLayer& layer, physx::PxFilterData& filterData);
        void SetGroup(const Physics::CollisionGroup& group, physx::PxFilterData& filterData);
        void SetCollisionLayerAndGroup(physx::PxShape* shape, const Physics::CollisionLayer& layer, const Physics::CollisionGroup&  group);
    }
}

#include <PhysX/Utils.inl>