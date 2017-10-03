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
#include "StaticMeshComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <MathConversion.h>

#include <I3DEngine.h>
#include <ICryAnimation.h>

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////

    void StaticMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        StaticMeshComponentRenderNode::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<StaticMeshComponent, AZ::Component>()
                ->Version(1)
                ->Field("Static Mesh Render Node", &StaticMeshComponent::m_staticMeshRenderNode);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MeshComponentRequestBus>("MeshComponentRequestBus")
                ->Event("GetWorldBounds", &MeshComponentRequestBus::Events::GetWorldBounds)
                ->Event("GetLocalBounds", &MeshComponentRequestBus::Events::GetLocalBounds)
                ->Event("SetVisibility", &MeshComponentRequestBus::Events::SetVisibility)
                ->Event("GetVisibility", &MeshComponentRequestBus::Events::GetVisibility)
                ->VirtualProperty("Visibility", "GetVisibility", "SetVisibility");

            behaviorContext->Class<StaticMeshComponent>()->RequestBus("MeshComponentRequestBus");
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void StaticMeshComponentRenderNode::StaticMeshRenderOptions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<StaticMeshComponentRenderNode::StaticMeshRenderOptions>()
                ->Version(3, &VersionConverter)
                ->Field("Opacity", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_opacity)
                ->Field("MaxViewDistance", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_maxViewDist)
                ->Field("ViewDistanceMultiplier", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_viewDistMultiplier)
                ->Field("LODRatio", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_lodRatio)
                ->Field("CastDynamicShadows", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_castShadows)
                ->Field("CastLightmapShadows", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_castLightmap)
                ->Field("UseVisAreas", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_useVisAreas)
                ->Field("RainOccluder", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_rainOccluder)
                ->Field("AffectDynamicWater", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_affectDynamicWater)
                ->Field("ReceiveWind", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_receiveWind)
                ->Field("AcceptDecals", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_acceptDecals)
                ->Field("AffectNavmesh", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_affectNavmesh)
                ->Field("VisibilityOccluder", &StaticMeshComponentRenderNode::StaticMeshRenderOptions::m_visibilityOccluder)
                ;
        }
    }

    bool StaticMeshComponentRenderNode::StaticMeshRenderOptions::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove Bloom (m_allowBloom)
        // - Remove MotionBlur (m_allowMotionBlur)
        // - Remove DepthTest (m_depthTest)
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("Bloom", 0xc6cd7d1b));
            classElement.RemoveElementByName(AZ_CRC("MotionBlur", 0x917cdb53));
            classElement.RemoveElementByName(AZ_CRC("DepthTest", 0x532f68b9));
        }

        // conversion from version 2:
        // - Remove IndoorOnly (m_indoorOnly)
        if (classElement.GetVersion() <= 2)
        {
            classElement.RemoveElementByName(AZ_CRC("IndoorOnly", 0xc8ab6ddb));
        }

        return true;
    }

    void StaticMeshComponentRenderNode::Reflect(AZ::ReflectContext* context)
    {
        StaticMeshRenderOptions::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<StaticMeshComponentRenderNode>()
                ->Version(1)
                ->Field("Visible", &StaticMeshComponentRenderNode::m_visible)
                ->Field("Static Mesh", &StaticMeshComponentRenderNode::m_staticMeshAsset)
                ->Field("Material Override", &StaticMeshComponentRenderNode::m_material)
                ->Field("Render Options", &StaticMeshComponentRenderNode::m_renderOptions)
                ;
        }
    }

    float StaticMeshComponentRenderNode::GetDefaultMaxViewDist()
    {
        if (gEnv && gEnv->p3DEngine)
        {
            return gEnv->p3DEngine->GetMaxViewDistance(false);
        }

        // In the editor and the game, the dynamic lookup above should *always* hit.
        // This case essentially means no renderer (not even the null renderer) is present.
        return FLT_MAX;
    }

    StaticMeshComponentRenderNode::StaticMeshRenderOptions::StaticMeshRenderOptions()
        : m_opacity(1.f)
        , m_viewDistMultiplier(1.f)
        , m_lodRatio(100)
        , m_useVisAreas(true)
        , m_castShadows(true)
        , m_castLightmap(true)
        , m_rainOccluder(true)
        , m_affectNavmesh(true)
        , m_affectDynamicWater(false)
        , m_acceptDecals(true)
        , m_receiveWind(false)
        , m_visibilityOccluder(false)
    {
        m_maxViewDist = GetDefaultMaxViewDist();
    }

    StaticMeshComponentRenderNode::StaticMeshComponentRenderNode()
        : m_statObj(nullptr)
        , m_materialOverride(nullptr)
        , m_auxiliaryRenderFlags(0)
        , m_auxiliaryRenderFlagsHistory(0)
        , m_lodDistance(0.f)
        , m_isRegisteredWithRenderer(false)
        , m_objectMoved(false)
        , m_staticMeshAsset(static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_QUEUE_LOAD))
        , m_visible(true)
    {
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();
        m_worldTransform = AZ::Transform::CreateIdentity();
        m_renderTransform = Matrix34::CreateIdentity();
        // Connect for LegacyAssetEventBus::Handler
        AzFramework::LegacyAssetEventBus::Handler::BusConnect(AZ_CRC("mtl", 0xb01910e0));
    }

    StaticMeshComponentRenderNode::~StaticMeshComponentRenderNode()
    {
        DestroyMesh();
        // Disconnect for LegacyAssetEventBus::Handler
        AzFramework::LegacyAssetEventBus::Handler::BusDisconnect();
    }

    void StaticMeshComponentRenderNode::CopyPropertiesTo(StaticMeshComponentRenderNode& rhs) const
    {
        rhs.m_visible = m_visible;
        rhs.m_materialOverride = m_materialOverride;
        rhs.m_staticMeshAsset = m_staticMeshAsset;
        rhs.m_material = m_material;
        rhs.m_renderOptions = m_renderOptions;
    }

    void StaticMeshComponentRenderNode::AttachToEntity(AZ::EntityId id)
    {
        if (AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(m_attachedToEntityId);
        }

        if (id.IsValid())
        {
            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(id))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(id);
            }

            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(entityTransform, id, AZ::TransformBus, GetWorldTM);
            UpdateWorldTransform(entityTransform);
        }

        m_attachedToEntityId = id;
    }

    void StaticMeshComponentRenderNode::OnAssetPropertyChanged()
    {
        if (HasMesh())
        {
            DestroyMesh();
        }

        AZ::Data::AssetBus::Handler::BusDisconnect();

        CreateMesh();
    }

    void StaticMeshComponentRenderNode::RefreshRenderState()
    {
        if (gEnv->IsEditor())
        {
            UpdateLocalBoundingBox();

            AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(parentTransform, m_attachedToEntityId, AZ::TransformBus, GetWorldTM);
            OnTransformChanged(AZ::Transform::CreateIdentity(), parentTransform);

            m_renderOptions.OnChanged();

            if (HasMesh())
            {
                // Re-register with the renderer, as some render settings/flags require it.
                // Note that this is editor-only behavior (hence the guard above).
                if (m_isRegisteredWithRenderer)
                {
                    RegisterWithRenderer(false);
                    RegisterWithRenderer(true);
                }
            }
        }
    }

    void StaticMeshComponentRenderNode::SetAuxiliaryRenderFlags(uint32 flags)
    {
        m_auxiliaryRenderFlags = flags;
        m_auxiliaryRenderFlagsHistory |= flags;
    }

    void StaticMeshComponentRenderNode::UpdateAuxiliaryRenderFlags(bool on, uint32 mask)
    {
        if (on)
        {
            m_auxiliaryRenderFlags |= mask;
        }
        else
        {
            m_auxiliaryRenderFlags &= ~mask;
        }

        m_auxiliaryRenderFlagsHistory |= mask;
    }

    void StaticMeshComponentRenderNode::CreateMesh()
    {
        if (m_staticMeshAsset.GetId().IsValid())
        {
            if (!AZ::Data::AssetBus::Handler::BusIsConnected())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_staticMeshAsset.GetId());
            }

            m_staticMeshAsset.QueueLoad();
        }
    }

    void StaticMeshComponentRenderNode::DestroyMesh()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        RegisterWithRenderer(false);
        m_statObj = nullptr;

        EBUS_EVENT_ID(m_attachedToEntityId, MeshComponentNotificationBus, OnMeshDestroyed);

        m_staticMeshAsset.Release();
    }

    bool StaticMeshComponentRenderNode::HasMesh() const
    {
        return m_statObj != nullptr;
    }

    void StaticMeshComponentRenderNode::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_staticMeshAsset.Create(id);
        OnAssetPropertyChanged();
    }

    void StaticMeshComponentRenderNode::GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObjectSize(this);
    }

    void StaticMeshComponentRenderNode::OnTransformChanged(const AZ::Transform&, const AZ::Transform& parentWorld)
    {
        // The entity to which we're attached has moved.
        UpdateWorldTransform(parentWorld);
    }

    void StaticMeshComponentRenderNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_staticMeshAsset)
        {
            m_staticMeshAsset = asset;
            m_statObj = m_staticMeshAsset.Get()->m_statObj;

            if (HasMesh())
            {
                const AZStd::string& materialOverridePath = m_material.GetAssetPath();
                if (!materialOverridePath.empty())
                {
                    m_materialOverride = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialOverridePath.c_str());

                    AZ_Warning("MeshComponent", m_materialOverride != gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial(),
                        "Failed to load override material \"%s\".",
                        materialOverridePath.c_str());
                }
                else
                {
                    m_materialOverride = nullptr;
                }

                UpdateLocalBoundingBox();
                UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());

                RegisterWithRenderer(true);

                // Inform listeners that the mesh has been changed
                EBUS_EVENT_ID(m_attachedToEntityId, MeshComponentNotificationBus, OnMeshCreated, asset);
            }
        }
    }

    void StaticMeshComponentRenderNode::UpdateWorldTransform(const AZ::Transform& entityTransform)
    {
        m_worldTransform = entityTransform;

        m_renderTransform = AZTransformToLYTransform(m_worldTransform);

        UpdateWorldBoundingBox();

        m_objectMoved = true;
    }

    void StaticMeshComponentRenderNode::UpdateLocalBoundingBox()
    {
        m_localBoundingBox.Reset();

        if (HasMesh())
        {
            m_localBoundingBox.Add(m_statObj->GetAABB());
        }

        UpdateWorldBoundingBox();
    }

    void StaticMeshComponentRenderNode::UpdateWorldBoundingBox()
    {
        m_worldBoundingBox.SetTransformedAABB(m_renderTransform, m_localBoundingBox);

        if (m_isRegisteredWithRenderer)
        {
            // Re-register with the renderer to update culling info
            gEnv->p3DEngine->RegisterEntity(this);
        }
    }

    void StaticMeshComponentRenderNode::SetVisible(bool isVisible)
    {
        if (m_visible != isVisible)
        {
            m_visible = isVisible;
            RegisterWithRenderer(false);
            RegisterWithRenderer(true);
        }
    }

    bool StaticMeshComponentRenderNode::GetVisible()
    {
        return m_visible;
    }

    void StaticMeshComponentRenderNode::RegisterWithRenderer(bool registerWithRenderer)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            if (registerWithRenderer)
            {
                if (!m_isRegisteredWithRenderer)
                {
                    ApplyRenderOptions();

                    gEnv->p3DEngine->RegisterEntity(this);

                    m_isRegisteredWithRenderer = true;
                }
            }
            else
            {
                if (m_isRegisteredWithRenderer)
                {
                    gEnv->p3DEngine->FreeRenderNodeState(this);
                    m_isRegisteredWithRenderer = false;
                }
            }
        }
    }

    namespace StaticMeshInternal
    {
        void UpdateRenderFlag(bool enable, int mask, unsigned int& flags)
        {
            if (enable)
            {
                flags |= mask;
            }
            else
            {
                flags &= ~mask;
            }
        }
    }

    void StaticMeshComponentRenderNode::ApplyRenderOptions()
    {
        using StaticMeshInternal::UpdateRenderFlag;
        unsigned int flags = GetRndFlags();

        // Turn off any flag which has ever been set via auxiliary render flags
        UpdateRenderFlag(false, m_auxiliaryRenderFlagsHistory, flags);

        // Update flags according to current render settings
        UpdateRenderFlag(m_renderOptions.m_useVisAreas == false, ERF_OUTDOORONLY, flags);
        UpdateRenderFlag(m_renderOptions.m_castShadows, ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, flags);
        UpdateRenderFlag(m_renderOptions.m_castLightmap, ERF_DYNAMIC_DISTANCESHADOWS, flags);
        UpdateRenderFlag(m_renderOptions.m_rainOccluder, ERF_RAIN_OCCLUDER, flags);
        UpdateRenderFlag(m_visible == false, ERF_HIDDEN, flags);
        UpdateRenderFlag(m_renderOptions.m_receiveWind, ERF_RECVWIND, flags);
        UpdateRenderFlag(m_renderOptions.m_visibilityOccluder, ERF_GOOD_OCCLUDER, flags);

        UpdateRenderFlag(false == m_renderOptions.m_affectNavmesh, ERF_EXCLUDE_FROM_TRIANGULATION, flags);
        UpdateRenderFlag(false == m_renderOptions.m_affectDynamicWater, ERF_NODYNWATER, flags);
        UpdateRenderFlag(false == m_renderOptions.m_acceptDecals, ERF_NO_DECALNODE_DECALS, flags);

        // Apply current auxiliary render flags
        UpdateRenderFlag(true, m_auxiliaryRenderFlags, flags);

        m_fWSMaxViewDist = m_renderOptions.m_maxViewDist;

        SetViewDistanceMultiplier(m_renderOptions.m_viewDistMultiplier);

        SetLodRatio(static_cast<int>(m_renderOptions.m_lodRatio));

        SetRndFlags(flags);
    }

    CLodValue StaticMeshComponentRenderNode::ComputeLOD(int wantedLod, const SRenderingPassInfo& passInfo)
    {
        return CLodValue(wantedLod);
    }

    AZ::Aabb StaticMeshComponentRenderNode::CalculateWorldAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_worldBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.max));
        }
        return aabb;
    }

    AZ::Aabb StaticMeshComponentRenderNode::CalculateLocalAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_localBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.max));
        }
        return aabb;
    }

    /*IRenderNode*/ void StaticMeshComponentRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
    {
        if (!HasMesh())
        {
            return;
        }

        SRendParams rParams(inRenderParams);

        // Assign a unique pInstance pointer, otherwise effects involving SRenderObjData will not work for this object.  CEntityObject::Render does this for legacy entities.
        rParams.pInstance = this;

        rParams.fAlpha = m_renderOptions.m_opacity;

        _smart_ptr<IMaterial> previousMaterial = rParams.pMaterial;
        const int previousObjectFlags = rParams.dwFObjFlags;

        if (m_materialOverride)
        {
            rParams.pMaterial = m_materialOverride;
        }

        if (m_objectMoved)
        {
            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
            m_objectMoved = false;
        }

        rParams.pMatrix = &m_renderTransform;
        if (rParams.pMatrix->IsValid())
        {
            rParams.lodValue = ComputeLOD(inRenderParams.lodValue.LodA(), passInfo);
            m_statObj->Render(rParams, passInfo);
        }

        rParams.pMaterial = previousMaterial;
        rParams.dwFObjFlags = previousObjectFlags;
    }

    /*IRenderNode*/ bool StaticMeshComponentRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
    {
        const float lodRatio = GetLodRatioNormalized();
        if (lodRatio > 0.0f)
        {
            const float distMultiplier = 1.f / (lodRatio * frameLodInfo.fTargetSize);

            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = m_lodDistance * (lodIndex + 1) * distMultiplier;
            }
        }
        else
        {
            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = FLT_MAX;
            }
        }

        return true;
    }

    void StaticMeshComponentRenderNode::UpdateLodDistance(const SFrameLodInfo& frameLodInfo)
    {
        SMeshLodInfo lodInfo;

        if (HasMesh())
        {
            m_statObj->ComputeGeometricMean(lodInfo);
        }

        m_lodDistance = sqrt(lodInfo.fGeometricMean);
    }

    /*IRenderNode*/ EERType StaticMeshComponentRenderNode::GetRenderNodeType()
    {
        return eERType_StaticMeshRenderComponent;
    }

    /*IRenderNode*/ const char* StaticMeshComponentRenderNode::GetName() const
    {
        return "StaticMeshComponentRenderNode";
    }

    /*IRenderNode*/ const char* StaticMeshComponentRenderNode::GetEntityClassName() const
    {
        return "StaticMeshComponentRenderNode";
    }

    /*IRenderNode*/ Vec3 StaticMeshComponentRenderNode::GetPos(bool bWorldOnly /*= true*/) const
    {
        return m_renderTransform.GetTranslation();
    }

    /*IRenderNode*/ const AABB StaticMeshComponentRenderNode::GetBBox() const
    {
        return m_worldBoundingBox;
    }

    /*IRenderNode*/ void StaticMeshComponentRenderNode::SetBBox(const AABB& WSBBox)
    {
        m_worldBoundingBox = WSBBox;
    }

    /*IRenderNode*/ void StaticMeshComponentRenderNode::OffsetPosition(const Vec3& delta)
    {
        // Recalculate local transform
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(localTransform, m_attachedToEntityId, AZ::TransformBus, GetLocalTM);

        localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
        EBUS_EVENT_ID(m_attachedToEntityId, AZ::TransformBus, SetLocalTM, localTransform);

        m_objectMoved = true;
    }

    /*IRenderNode*/ struct IPhysicalEntity* StaticMeshComponentRenderNode::GetPhysics() const
    {
        return nullptr;
    }

    /*IRenderNode*/ void StaticMeshComponentRenderNode::SetPhysics(IPhysicalEntity* pPhys)
    {
    }

    /*IRenderNode*/ void StaticMeshComponentRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
    {
        m_materialOverride = pMat;

        if (pMat)
        {
            m_material.SetAssetPath(pMat->GetName());
        }
        else
        {
            // If no material is provided, we intend to reset to the original material so we treat
            // it as an asset reset to recreate the mesh.
            m_material.SetAssetPath("");
            OnAssetPropertyChanged();
        }
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> StaticMeshComponentRenderNode::GetMaterial(Vec3* pHitPos /*= nullptr*/)
    {
        if (m_materialOverride)
        {
            return m_materialOverride;
        }

        if (HasMesh())
        {
            return m_statObj->GetMaterial();
        }

        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> StaticMeshComponentRenderNode::GetMaterialOverride()
    {
        return m_materialOverride;
    }

    /*IRenderNode*/ float StaticMeshComponentRenderNode::GetMaxViewDist()
    {
        return(m_renderOptions.m_maxViewDist * 0.75f * GetViewDistanceMultiplier());
    }

    /*IRenderNode*/ IStatObj* StaticMeshComponentRenderNode::GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix, bool bReturnOnlyVisible)
    {
        if (0 == nPartId)
        {
            if (pMatrix)
            {
                *pMatrix = m_renderTransform;
            }

            return m_statObj;
        }

        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> StaticMeshComponentRenderNode::GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible, bool* pbDrawNear)
    {
        if (0 == nPartId)
        {
            return m_materialOverride;
        }

        return nullptr;
    }

    /*IRenderNode*/ ICharacterInstance* StaticMeshComponentRenderNode::GetEntityCharacter(unsigned int nSlot, Matrix34A* pMatrix, bool bReturnOnlyVisible)
    {
        return nullptr;
    }
    //////////////////////////////////////////////////////////////////////////


    // override from LegacyAssetEventBus::Handler
    // Notifies listeners that a file changed
    // Handle material Overrides

    void StaticMeshComponentRenderNode::OnFileChanged(AZStd::string assetPath)
    {
        if (m_materialOverride == nullptr)
        {
            return;
        }

        if (nullptr != strstr(assetPath.c_str(), m_materialOverride->GetName()))
        {
            OnAssetPropertyChanged();
        }
    }

    void StaticMeshComponentRenderNode::OnFileRemoved(AZStd::string assetPath)
    {
        if (m_materialOverride == nullptr)
        {
            return;
        }

        if (nullptr != strstr(assetPath.c_str(), m_materialOverride->GetName()))
        {
            OnAssetPropertyChanged();
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // StaticMeshComponent
    const float StaticMeshComponent::s_renderNodeRequestBusOrder = 100.f;

    void StaticMeshComponent::Activate()
    {
        m_staticMeshRenderNode.AttachToEntity(m_entity->GetId());

        // Note we are purposely connecting to buses before calling m_mesh.CreateMesh().
        // m_mesh.CreateMesh() can result in events (eg: OnMeshCreated) that we want receive.
        MaterialRequestBus::Handler::BusConnect(m_entity->GetId());
        MeshComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());
        m_staticMeshRenderNode.CreateMesh();
        StaticMeshComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void StaticMeshComponent::Deactivate()
    {
        StaticMeshComponentRequestBus::Handler::BusDisconnect();
        MaterialRequestBus::Handler::BusDisconnect();
        MeshComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        m_staticMeshRenderNode.DestroyMesh();
        m_staticMeshRenderNode.AttachToEntity(AZ::EntityId());
    }

    AZ::Aabb StaticMeshComponent::GetWorldBounds()
    {
        return m_staticMeshRenderNode.CalculateWorldAABB();
    }

    AZ::Aabb StaticMeshComponent::GetLocalBounds()
    {
        return m_staticMeshRenderNode.CalculateLocalAABB();
    }

    void StaticMeshComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_staticMeshRenderNode.SetMeshAsset(id);
    }

    void StaticMeshComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_staticMeshRenderNode.SetMaterial(material);
    }

    _smart_ptr<IMaterial> StaticMeshComponent::GetMaterial()
    {
        return m_staticMeshRenderNode.GetMaterial();
    }

    IRenderNode* StaticMeshComponent::GetRenderNode()
    {
        return &m_staticMeshRenderNode;
    }

    float StaticMeshComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    IStatObj* StaticMeshComponent::GetStatObj()
    {
        return m_staticMeshRenderNode.GetEntityStatObj();
    }

    bool StaticMeshComponent::GetVisibility()
    {
        return m_staticMeshRenderNode.GetVisible();
    }

    void StaticMeshComponent::SetVisibility(bool isVisible)
    {
        m_staticMeshRenderNode.SetVisible(isVisible);
    }
} // namespace LmbrCentral
