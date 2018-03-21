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

#include "StarterGameGem_precompiled.h"
#include "StarterGameAIUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AI/NavigationComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <IAISystem.h>
#include <INavigationSystem.h>
#include <MathConversion.h>


namespace StarterGameGem
{
    bool StarterGameAIUtility::IsOnNavMesh(const AZ::Vector3& pos)
    {
        INavigationSystem* navSystem = gEnv->pAISystem->GetNavigationSystem();
        NavigationAgentTypeID agentType = navSystem->GetAgentTypeID("MediumSizedCharacters");
        bool isValid = false;
        if (agentType)
            isValid = navSystem->IsLocationValidInNavigationMesh(agentType, AZVec3ToLYVec3(pos));
        else
            AZ_Warning("StarterGame", false, "%s: Invalid agent type.", __FUNCTION__);

        return isValid;
    }

    bool StarterGameAIUtility::GetClosestPointInNavMesh(const AZ::Vector3& pos, AZ::Vector3& found)
    {
        if (IsOnNavMesh(pos))
        {
            found = pos;
            return true;
        }

        INavigationSystem* navSystem = gEnv->pAISystem->GetNavigationSystem();
        NavigationAgentTypeID agentType = navSystem->GetAgentTypeID("MediumSizedCharacters");
        AZ_Warning("StarterGame", agentType, "%s: Invalid agent type.", __FUNCTION__);

        Vec3 closestPoint(0.0f, 0.0f, 0.0f);
        if (!navSystem->GetClosestPointInNavigationMesh(agentType, AZVec3ToLYVec3(pos), 3.0f, 3.0f, &closestPoint))
        {
            //AZ_Warning("StarterGame", false, "%s: Failed to find the closest point.", __FUNCTION__);
            found = AZ::Vector3::CreateZero();
            return false;
        }

        found = LYVec3ToAZVec3(closestPoint);
        return true;
    }

    float StarterGameAIUtility::GetArrivalDistanceThreshold(const AZ::EntityId& entityId)
    {
        float threshold = 0.0f;
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        const AZ::Entity::ComponentArrayType& components = entity->GetComponents();

        for (size_t i = 0; i < components.size(); ++i)
        {
            LmbrCentral::NavigationComponent* comp = azdynamic_cast<LmbrCentral::NavigationComponent*>(components[i]);
            if (comp != nullptr)
            {
                threshold = comp->GetArrivalDistance();
            }
        }
        return threshold;
    }

    void StarterGameAIUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameAIUtility>("StarterGameAIUtility")
                ->Method("IsOnNavMesh", &IsOnNavMesh)
                ->Method("GetClosestPointInNavMesh", &GetClosestPointInNavMesh)
                ->Method("GetArrivalDistanceThreshold", &GetArrivalDistanceThreshold)
            ;
        }

    }

}
