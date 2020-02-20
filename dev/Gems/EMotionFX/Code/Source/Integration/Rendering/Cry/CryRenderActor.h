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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <Integration/Rendering/Cry/CryRenderBackendCommon.h>
#include <Integration/Rendering/RenderActor.h>

namespace EMotionFX
{
    namespace Integration
    {
        class ActorAsset;

        class CryRenderActorRequests
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::mutex;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::Data::AssetId;

            virtual void Finalize() {}
        };

        using CryRenderActorRequestBus = AZ::EBus<CryRenderActorRequests>;

        class CryRenderActor
            : public RenderActor
            , private CryRenderActorRequestBus::Handler
        {
        public:
            AZ_RTTI(EMotionFX::Integration::CryRenderActor, "{5DCC47DC-448A-4CF8-B370-1764B45FD1D5}", EMotionFX::Integration::RenderActor)
            AZ_CLASS_ALLOCATOR_DECL

            CryRenderActor(ActorAsset* actorAsset);
            ~CryRenderActor();
            bool Init();

            size_t GetNumLODs() const { return m_meshLODs.size(); }
            MeshLOD* GetMeshLOD(size_t lodIndex) { return m_meshLODs[lodIndex].m_isReady ? &m_meshLODs[lodIndex] : nullptr; }

        private:
            bool BuildLODMeshes();

            // CryRenderActorNotifications
            void Finalize() override;

            ActorAsset* m_actorAsset;
            AZStd::vector<MeshLOD> m_meshLODs; ///< Mesh render data (for CryRenderer)
        };
    } // namespace Integration
} // namespace EMotionFX
