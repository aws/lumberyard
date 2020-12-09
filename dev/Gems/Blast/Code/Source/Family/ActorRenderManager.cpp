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

#include "StdAfx.h"

#include <Family/ActorRenderManager.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <Blast/BlastActor.h>
#include <Components/BlastMeshDataComponent.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

namespace Blast
{
    ActorRenderManager::ActorRenderManager(
        AZStd::shared_ptr<EntityProvider> entityProvider, BlastMeshData* meshData, uint32_t chunkCount, AZ::Vector3 scale)
        : m_chunkCount(chunkCount)
        , m_meshData(meshData)
        , m_entityProvider(AZStd::move(entityProvider))
        , m_chunkEntities(chunkCount, nullptr)
        , m_chunkTransformHandlers(chunkCount, nullptr)
        , m_chunkMeshHandlers(chunkCount, nullptr)
        , m_chunkActors(chunkCount, nullptr)
        , m_scale(scale)
    {
        for (auto chunkId = 0u; chunkId < chunkCount; ++chunkId)
        {
            auto entity = m_entityProvider->CreateEntity(
                {AZ::TransformComponentTypeId, AZ::Uuid::CreateString("{2F4BAD46-C857-4DCB-A454-C412DE67852A}")});
            AZ_Assert(
                entity,
                "Adding TransformComponent and MeshComponent must be successful. The chunk will not be rendered.");
            if (!entity)
            {
                continue;
            }

            m_chunkEntities[chunkId] = entity;

            entity->Init();
            entity->Activate();

            m_chunkTransformHandlers[chunkId] = AZ::TransformBus::FindFirstHandler(entity->GetId());
            m_chunkMeshHandlers[chunkId] = LmbrCentral::MeshComponentRequestBus::FindFirstHandler(entity->GetId());

            if (m_meshData)
            {
                const auto& meshAsset = m_meshData->GetMeshAsset(chunkId);
                m_chunkMeshHandlers[chunkId]->SetMeshAsset(meshAsset.GetId());
            }
            m_chunkMeshHandlers[chunkId]->SetVisibility(false);
        }
    }

    void ActorRenderManager::OnActorCreated(const BlastActor& actor)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        const AZStd::vector<uint32_t>& chunkIndices = actor.GetChunkIndices();

        for (uint32_t chunkId : chunkIndices)
        {
            m_chunkActors[chunkId] = &actor;
        }
    }

    void ActorRenderManager::OnActorDestroyed(const BlastActor& actor)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        const AZStd::vector<uint32_t>& chunkIndices = actor.GetChunkIndices();

        for (uint32_t chunkId : chunkIndices)
        {
            m_chunkActors[chunkId] = nullptr;
        }
    }

    void ActorRenderManager::SyncMeshes()
    {
        // It is more natural to have chunk entities be transform children of rigid body entity,
        // however having them separate and manually synchronizing transform is more efficient.
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        for (auto chunkId = 0u; chunkId < m_chunkCount; ++chunkId)
        {
            if (m_chunkActors[chunkId])
            {
                auto transform = m_chunkActors[chunkId]->GetWorldBody()->GetTransform();
                // Multiply by scale because the transform on the world body does not store scale
                transform.MultiplyByScale(m_scale);
                m_chunkTransformHandlers[chunkId]->SetWorldTM(transform);
            }
            m_chunkMeshHandlers[chunkId]->SetVisibility(m_chunkActors[chunkId] != nullptr);
        }
    }
} // namespace Blast
