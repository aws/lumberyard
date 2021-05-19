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

#include <Actor/EntityProvider.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    class MeshComponentRequests;
}

namespace Blast
{
    class BlastMeshData;
    class BlastActor;

    /// Responsible for synchronizing render meshes of BlastFamily to corresponding BlastActors.
    /// Ideally we would want to just have meshes directly on the same entity as BlastActor's
    /// rigid body, but existing MeshComponent cannot hold several meshes and several MeshComponents
    /// cannot exist on the same entity.
    class ActorRenderManager
    {
    public:
        // Initializes the manager by creating an entity with a render mesh for each chunk.
        // Initially all meshes are invisible.
        ActorRenderManager(
            AZStd::shared_ptr<EntityProvider> entityProvider, BlastMeshData* meshData, uint32_t chunkCount, AZ::Vector3 scale);

        // Callback that makes meshes corresponding to the actor visible and follows it's transform.
        void OnActorCreated(const BlastActor& actor);

        // Callback that makes meshes corresponding to the actor invisible.
        void OnActorDestroyed(const BlastActor& actor);

        // Update positions of entities with render meshes corresponding to their right dynamic bodies.
        void SyncMeshes();

    protected:
        uint32_t m_chunkCount;
        BlastMeshData* m_meshData;
        AZStd::shared_ptr<EntityProvider> m_entityProvider;

        AZStd::vector<AZStd::shared_ptr<AZ::Entity>> m_chunkEntities;
        AZStd::vector<AZ::TransformInterface*> m_chunkTransformHandlers;
        AZStd::vector<LmbrCentral::MeshComponentRequests*> m_chunkMeshHandlers;
        AZStd::vector<const BlastActor*> m_chunkActors;
        AZ::Vector3 m_scale;
    };
} // namespace Blast
