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
#include <LmbrCentral_precompiled.h>
#include "MaterialOwnerRequestBusHandlerImpl.h"
#include <IEntityRenderState.h>
#include <I3DEngine.h>

namespace LmbrCentral
{
    
    void MaterialOwnerRequestBusHandlerImpl::Activate(IRenderNode* renderNode, const AZ::EntityId& entityId)
    {
        m_clonedMaterial = nullptr;
        m_renderNode = renderNode;
        m_readyEventSent = false;

        MaterialOwnerNotificationBus::Bind(m_notificationBus, entityId);

        if (m_renderNode)
        {
            if (!m_renderNode->IsReady())
            {
                // Some material owners, in particular MeshComponents, may not be ready upon activation because the
                // actual mesh data and default material haven't been loaded yet. Until the RenderNode is ready,
                // it's material probably isn't valid.
                MeshComponentNotificationBus::Handler::BusConnect(entityId);
            }
            else
            {
                // For some material owner types (like DecalComponent), the material is ready immediately. But we can't
                // send the event yet because components are still being Activated, so we delay until the first tick.
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::Deactivate()
    {
        m_notificationBus = nullptr;
        MeshComponentNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MaterialOwnerRequestBusHandlerImpl::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ_Assert(IsMaterialOwnerReady(), "Got OnMeshCreated but the RenderNode still isn't ready");
        SendReadyEvent();
        MeshComponentNotificationBus::Handler::BusDisconnect();
    }

    void MaterialOwnerRequestBusHandlerImpl::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (!m_readyEventSent && IsMaterialOwnerReady())
        {
            SendReadyEvent();
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::SendReadyEvent()
    {
        AZ_Assert(!m_readyEventSent, "OnMaterialOwnerReady already sent");
        if (!m_readyEventSent)
        {
            m_readyEventSent = true;
            MaterialOwnerNotificationBus::Event(m_notificationBus, &MaterialOwnerNotifications::OnMaterialOwnerReady);
        }
    }

    bool MaterialOwnerRequestBusHandlerImpl::IsMaterialOwnerReady()
    {
        return m_renderNode && m_renderNode->IsReady();
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterial(MaterialPtr material)
    {
        if (m_renderNode)
        {
            if (material && material->IsSubMaterial())
            {
                 AZ_Error("MaterialOwnerRequestBus", false, "Material Owner cannot be given a Sub-Material.");
            }
            else
            {
                m_clonedMaterial = nullptr;
                m_renderNode->SetMaterial(material);
            }
        }
    }

    MaterialOwnerRequestBusHandlerImpl::MaterialPtr MaterialOwnerRequestBusHandlerImpl::GetMaterial()
    {
        MaterialPtr material = nullptr;

        if (m_renderNode)
        {
            material = m_renderNode->GetMaterial();

            if (!m_renderNode->IsReady())
            {
                if (material)
                {
                    AZ_Warning("MaterialOwnerRequestBus", false, "A Material was found, but Material Owner is not ready. May have unexpected results. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                }
                else
                {
                    AZ_Error("MaterialOwnerRequestBus", false, "Material Owner is not ready and no Material was found. Assets probably have not finished loading yet. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                }
            }
            
            AZ_Assert(nullptr == m_clonedMaterial || material == m_clonedMaterial, "MaterialOwnerRequestBusHandlerImpl and RenderNode are out of sync");
        }

        return material;
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterialHandle(const MaterialHandle& materialHandle)
    {
        SetMaterial(materialHandle.m_material);
    }

    MaterialHandle MaterialOwnerRequestBusHandlerImpl::GetMaterialHandle()
    {
        MaterialHandle m;
        m.m_material = GetMaterial();
        return m;
    }

    bool MaterialOwnerRequestBusHandlerImpl::IsMaterialCloned() const
    {
        return nullptr != m_clonedMaterial;
    }

    void MaterialOwnerRequestBusHandlerImpl::CloneMaterial()
    {
        CloneMaterial(GetMaterial());
    }

    void MaterialOwnerRequestBusHandlerImpl::CloneMaterial(MaterialPtr material)
    {
        if (material && m_renderNode)
        {
            AZ_Assert(nullptr == m_clonedMaterial, "Material has already been cloned. This operation is wasteful.");

            m_clonedMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(material);
            AZ_Assert(m_clonedMaterial, "Failed to clone material. The original will be used.");
            if (m_clonedMaterial)
            {
                m_renderNode->SetMaterial(m_clonedMaterial);
            }
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterialParamVector4(const AZStd::string& name, const AZ::Vector4& value, int materialId)
    {
        if (GetMaterial())
        {
            if (!IsMaterialCloned())
            {
                CloneMaterial();
            }

            const int materialIndex = materialId - 1;
            Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
            const bool success = GetMaterial()->SetGetMaterialParamVec4(name.c_str(), vec4, false, true, materialIndex);
            AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterialParamVector3(const AZStd::string& name, const AZ::Vector3& value, int materialId)
    {
        if (GetMaterial())
        {
            if (!IsMaterialCloned())
            {
                CloneMaterial();
            }

            const int materialIndex = materialId - 1;
            Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
            const bool success = GetMaterial()->SetGetMaterialParamVec3(name.c_str(), vec3, false, true, materialIndex);
            AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterialParamColor(const AZStd::string& name, const AZ::Color& value, int materialId)
    {
        if (GetMaterial())
        {
            // When value had garbage data is was not only making the material render black, it also corrupted something
            // on the GPU, making black boxes flicker over the sky.
            // It was garbage due to a bug in the Color object node where all fields have to be set to some value manually; the default is not 0.
            if ((value.GetR() < 0 || value.GetR() > 1) ||
                (value.GetG() < 0 || value.GetG() > 1) ||
                (value.GetB() < 0 || value.GetB() > 1) ||
                (value.GetA() < 0 || value.GetA() > 1))
            {
                return;
            }

            if (!IsMaterialCloned())
            {
                CloneMaterial();
            }

            const int materialIndex = materialId - 1;
            Vec4 vec4(value.GetR(), value.GetG(), value.GetB(), value.GetA());
            const bool success = GetMaterial()->SetGetMaterialParamVec4(name.c_str(), vec4, false, true, materialIndex);
            AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
        }
    }

    void MaterialOwnerRequestBusHandlerImpl::SetMaterialParamFloat(const AZStd::string& name, float value, int materialId  /*= 0*/)
    {
        if (GetMaterial())
        {
            if (!IsMaterialCloned())
            {
                CloneMaterial();
            }

            const int materialIndex = materialId - 1;
            const bool success = GetMaterial()->SetGetMaterialParamFloat(name.c_str(), value, false, true, materialIndex);
            AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
        }
    }

    AZ::Vector4 MaterialOwnerRequestBusHandlerImpl::GetMaterialParamVector4(const AZStd::string& name, int materialId)
    {
        AZ::Vector4 value = AZ::Vector4::CreateZero();

        MaterialPtr material = GetMaterial();
        if (material)
        {
            const int materialIndex = materialId - 1;
            Vec4 vec4;
            if (material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true, materialIndex))
            {
                value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
            }
            else
            {
                AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        return value;
    }

    AZ::Vector3 MaterialOwnerRequestBusHandlerImpl::GetMaterialParamVector3(const AZStd::string& name, int materialId)
    {
        AZ::Vector3 value = AZ::Vector3::CreateZero();

        MaterialPtr material = GetMaterial();
        if (material)
        {
            const int materialIndex = materialId - 1;
            Vec3 vec3;
            if (material->SetGetMaterialParamVec3(name.c_str(), vec3, true, true, materialIndex))
            {
                value.Set(vec3.x, vec3.y, vec3.z);
            }
            else
            {
                AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        return value;
    }

    AZ::Color MaterialOwnerRequestBusHandlerImpl::GetMaterialParamColor(const AZStd::string& name, int materialId)
    {
        AZ::Color value = AZ::Color::CreateZero();

        MaterialPtr material = GetMaterial();
        if (material)
        {
            const int materialIndex = materialId - 1;
            Vec4 vec4;
            if (material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true, materialIndex))
            {
                value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
            }
            else
            {
                AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        return value;
    }

    float MaterialOwnerRequestBusHandlerImpl::GetMaterialParamFloat(const AZStd::string& name, int materialId)
    {
        float value = 0.0f;

        MaterialPtr material = GetMaterial();
        if (material)
        {
            const int materialIndex = materialId - 1;
            const bool success = material->SetGetMaterialParamFloat(name.c_str(), value, true, true, materialIndex);
            AZ_Error("Material Owner", success, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
        }

        return value;
    }


} // namespace LmbrCentral



