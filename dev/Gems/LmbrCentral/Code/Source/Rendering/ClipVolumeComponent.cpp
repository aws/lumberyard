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

#include "LmbrCentral_precompiled.h"
#include "ClipVolumeComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <MathConversion.h>
#include <I3DEngine.h>
#include "IEntitySystem.h"

namespace LmbrCentral
{
    void ClipVolumeComponentCommon::CreateClipVolume()
    {
        m_pClipVolume = gEnv->p3DEngine->CreateClipVolume();

        UpdateClipVolume();

        LmbrCentral::ClipVolumeComponentNotificationBus::Broadcast(&ClipVolumeComponentNotifications::OnClipVolumeCreated, m_pClipVolume);
    }

    void ClipVolumeComponentCommon::UpdateClipVolume()
    {
        if (m_pBspTree)
        {
            gEnv->pEntitySystem->ReleaseBSPTree3D(m_pBspTree);
        }
        m_pRenderMesh = nullptr;

        if (m_vertices.empty() || !m_pClipVolume)
        {
            return;
        }

        AZ::Transform currentTransform;
        AZ::TransformBus::EventResult(currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        // Convert vertices for use with the BSP tree system and render mesh
        using BSPFace = DynArray<Vec3>;
        using BSPFaceList = DynArray<BSPFace>;

        BSPFaceList faceList;
        faceList.reserve(m_vertices.size() / 3);

        for (int i = 0; i < m_vertices.size(); i += 3)
        {
            BSPFace face;
            face.push_back(AZVec3ToLYVec3(m_vertices[i]));
            face.push_back(AZVec3ToLYVec3(m_vertices[i + 1]));
            face.push_back(AZVec3ToLYVec3(m_vertices[i + 2]));
            faceList.push_back(face);
        }

        m_pBspTree = gEnv->pEntitySystem->CreateBSPTree3D(faceList);

        AZStd::vector<SVF_P3S_C4B_T2S> renderPoints;
        renderPoints.resize(m_vertices.size());
        AZStd::vector<vtx_idx> indices;
        indices.resize(m_vertices.size());
        for (int i = 0; i < m_vertices.size(); ++i)
        {
            renderPoints[i].xyz = AZVec3ToLYVec3(m_vertices[i]);
            indices[i] = i;
        }

        m_pRenderMesh = gEnv->pRenderer->CreateRenderMeshInitialized(renderPoints.data(), renderPoints.size(), eVF_P3S_C4B_T2S, indices.data(), indices.size(), prtTriangleList, "ClipVol", "ClipVol");

        gEnv->p3DEngine->UpdateClipVolume(m_pClipVolume, m_pRenderMesh, m_pBspTree, AZTransformToLYTransform(currentTransform), true, IClipVolume::eClipVolumeAffectedBySun, "ClipVolume");
    }

    void ClipVolumeComponentCommon::UpdatedVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_vertices = vertices;
        // Update current clip volume
        UpdateClipVolume();
    }

    void ClipVolumeComponentCommon::CopyPropertiesTo(ClipVolumeComponentCommon& rhs) const
    {
        rhs.m_vertices = m_vertices;
    }

    void ClipVolumeComponentCommon::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ClipVolumeComponentCommon>()
                ->Version(1)
                ->Field("ShapeVertices", &ClipVolumeComponentCommon::m_vertices);
        }
    }

    void ClipVolumeComponentCommon::Cleanup()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        if (m_pClipVolume)
        {
            LmbrCentral::ClipVolumeComponentNotificationBus::Broadcast(&ClipVolumeComponentNotifications::OnClipVolumeDestroyed, m_pClipVolume);
            gEnv->p3DEngine->DeleteClipVolume(m_pClipVolume);
            m_pClipVolume = nullptr;
        }
        if (m_pBspTree)
        {
            gEnv->pEntitySystem->ReleaseBSPTree3D(m_pBspTree);
        }
        m_pRenderMesh = nullptr;

        m_entityId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
    }

    void ClipVolumeComponentCommon::AttachToEntity(AZ::EntityId entityId)
    {
        Cleanup();

        if (entityId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        }

        m_entityId = entityId;
    }

    void ClipVolumeComponentCommon::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ::Transform currentTransform;
        AZ::TransformBus::EventResult(currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        gEnv->p3DEngine->UpdateClipVolume(m_pClipVolume, m_pRenderMesh, m_pBspTree, AZTransformToLYTransform(currentTransform), true, IClipVolume::eClipVolumeAffectedBySun, "ClipVolume");
    }
 
    void ClipVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        ClipVolumeComponentCommon::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ClipVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("ShapeVertices", &ClipVolumeComponent::m_vertices)
                ->Field("ClipVolumeData", &ClipVolumeComponent::m_clipVolume);
        }
    }

    ClipVolumeComponent::ClipVolumeComponent()
    {
    }

    ClipVolumeComponent::ClipVolumeComponent(AZStd::vector<AZ::Vector3>& vertices)
    {
        m_vertices = vertices;
    }

    void ClipVolumeComponent::Activate()
    {
        m_clipVolume.AttachToEntity(GetEntityId());
        m_clipVolume.CreateClipVolume();

        ClipVolumeComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ClipVolumeComponent::Deactivate()
    {
        ClipVolumeComponentRequestBus::Handler::BusDisconnect();

        m_clipVolume.Cleanup();
    }


} //namespace LmbrCentral
