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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Editor/Components/EditorSimpleLODComponent.h>


namespace EMotionFX
{
    namespace Integration
    {
        void EditorSimpleLODComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorSimpleLODComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("LODDistances", &EditorSimpleLODComponent::m_lodDistances)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorSimpleLODComponent>(
                        "Simple LOD Distance", "The Simple LOD distance component alters the actor LOD level based on ")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Mannequin.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Mannequin.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorSimpleLODComponent::m_lodDistances,
                            "LOD Distance (Max)", "The maximum camera distance of this LOD.")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->ElementAttribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->ElementAttribute(AZ::Edit::Attributes::Suffix, " m");
                }
            }

        }

        EditorSimpleLODComponent::EditorSimpleLODComponent()
            : m_actorInstance(nullptr)
        {
        }

        EditorSimpleLODComponent::~EditorSimpleLODComponent()
        {
        }

        void EditorSimpleLODComponent::Activate()
        {
            EMotionFXPtr<EMotionFX::ActorInstance> actorInstance;
            ActorComponentRequestBus::EventResult(
                actorInstance,
                GetEntityId(),
                &ActorComponentRequestBus::Events::GetActorInstance);

            if (actorInstance)
            {
                m_actorInstance = actorInstance.get();
                const AZ::u32 numLODs = m_actorInstance->GetActor()->GetNumLODLevels();
                if (numLODs != m_lodDistances.size())
                {
                    GenerateDefaultDistances(numLODs);
                }
            }
            else
            {
                m_actorInstance = nullptr;
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();
        }

        void EditorSimpleLODComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        void EditorSimpleLODComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            if (m_actorInstance != actorInstance)
            {
                m_actorInstance = actorInstance;
                const AZ::u32 numLODs = m_actorInstance->GetActor()->GetNumLODLevels();
                if (numLODs != m_lodDistances.size())
                {
                    GenerateDefaultDistances(numLODs);
                }
            }
        }

        void EditorSimpleLODComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = nullptr;
            m_lodDistances.clear();
        }

        void EditorSimpleLODComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            SimpleLODComponent::UpdateLODLevelByDistance(m_actorInstance, m_lodDistances, GetEntityId());
        }

        void EditorSimpleLODComponent::GenerateDefaultDistances(AZ::u32 numLodLevels)
        {
            // Generate the default LOD to 10, 20, 30....
            m_lodDistances.resize(numLodLevels);
            for (AZ::u32 i = 0; i < numLodLevels; ++i)
            {
                m_lodDistances[i] = i * 10.0f + 10.0f;
            }
        }

        void EditorSimpleLODComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            SimpleLODComponent::Configuration cfg;
            cfg.m_lodDistances = m_lodDistances;

            gameEntity->AddComponent(aznew SimpleLODComponent(&cfg));
        }
    }
}