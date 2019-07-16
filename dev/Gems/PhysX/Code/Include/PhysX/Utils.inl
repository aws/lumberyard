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

namespace PhysX
{
    inline ActorData* Utils::GetUserData(const physx::PxActor* actor)
    {
        if (actor == nullptr || actor->userData == nullptr)
        {
            return nullptr;
        }

        ActorData* actorData = static_cast<ActorData*>(actor->userData);
        if (!actorData->IsValid())
        {
            AZ_Warning("PhysX::Utils::GetUserData", false, "The actor data does not look valid and is not safe to use");
            return nullptr;
        }

        return actorData;
    }

    inline Physics::Material* Utils::GetUserData(const physx::PxMaterial* material)
    {
        return (material == nullptr) ? nullptr : static_cast<Physics::Material*>(material->userData);
    }

    inline Physics::Shape* Utils::GetUserData(const physx::PxShape* pxShape)
    {
        return (pxShape == nullptr) ? nullptr : static_cast<Physics::Shape*>(pxShape->userData);
    }

    inline Physics::World* Utils::GetUserData(physx::PxScene* scene)
    {
        return scene ? static_cast<Physics::World*>(scene->userData) : nullptr;
    }

    inline void Utils::SetLayer(const Physics::CollisionLayer& layer, physx::PxFilterData& filterData)
    {
        filterData.word0 = (physx::PxU32)(layer.GetMask() >> 32);
        filterData.word1 = (physx::PxU32)(layer.GetMask());
    }

    inline void Utils::SetGroup(const Physics::CollisionGroup& group, physx::PxFilterData& filterData)
    {
        filterData.word2 = (physx::PxU32)(group.GetMask() >> 32);
        filterData.word3 = (physx::PxU32)(group.GetMask());
    }

    inline void Utils::SetCollisionLayerAndGroup(physx::PxShape* shape, const Physics::CollisionLayer& layer, const Physics::CollisionGroup&  group)
    {
        physx::PxFilterData filterData;
        SetLayer(layer, filterData);
        SetGroup(group, filterData);
        shape->setSimulationFilterData(filterData);
    }
}