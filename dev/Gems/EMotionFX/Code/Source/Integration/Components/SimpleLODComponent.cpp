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


#include "EMotionFX_precompiled.h"
#include <MathConversion.h>
#include <IRenderAuxGeom.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Integration/Components/SimpleLODComponent.h>
#include <MCore/Source/AttributeString.h>


namespace EMotionFX
{
    namespace Integration
    {

        void SimpleLODComponent::Configuration::Reflect(AZ::ReflectContext *context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(1)
                    ->Field("LODDistances", &Configuration::m_lodDistances)
                    ;
            }
        }

        void SimpleLODComponent::Reflect(AZ::ReflectContext* context)
        {
            Configuration::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SimpleLODComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &SimpleLODComponent::m_configuration)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SimpleLODComponent>(
                        "Simple LOD distance", "The Simple LOD distance component alters the actor LOD level based on distance to camera")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }

        }

        SimpleLODComponent::Configuration::Configuration()
        {
        }

        SimpleLODComponent::SimpleLODComponent(const Configuration* config)
            : m_actorInstance(nullptr)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        SimpleLODComponent::~SimpleLODComponent()
        {
        }

        void SimpleLODComponent::Init()
        {
        }

        void SimpleLODComponent::Activate()
        {
            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();
        }

        void SimpleLODComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        void SimpleLODComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;
        }

        void SimpleLODComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = nullptr;
        }

        void SimpleLODComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            UpdateLODLevelByDistance(m_actorInstance, m_configuration.m_lodDistances, GetEntityId());
        }

        AZ::u32 SimpleLODComponent::GetLODByDistance(const AZStd::vector<float>& distances, float distance)
        {
            const size_t max = distances.size();
            for (size_t i = 0; i < max; ++i)
            {
                const float rDistance = distances[i];
                if (distance < rDistance)
                {
                    return i;
                }
            }

            return max - 1;
        }

        void SimpleLODComponent::UpdateLODLevelByDistance(EMotionFX::ActorInstance * actorInstance, const AZStd::vector<float>& distances, AZ::EntityId entityId)
        {
            if (actorInstance)
            {
                AZ::Transform worldTransform;
                AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
                const AZ::Vector3& worldPos = worldTransform.GetPosition();

                // Compute the distance between the camera and the entity
                if (gEnv->pSystem)
                {
                    const CCamera& camera = gEnv->pSystem->GetViewCamera();
                    const AZ::Vector3& cameraPos = LYVec3ToAZVec3(camera.GetPosition());
                    const AZ::VectorFloat distance = cameraPos.GetDistance(worldPos);
                    const AZ::u32 lodByDistance = GetLODByDistance(distances, distance);
                    actorInstance->SetLODLevel(lodByDistance);
                }
            }
        }
    } // namespace integration
} // namespace EMotionFX

