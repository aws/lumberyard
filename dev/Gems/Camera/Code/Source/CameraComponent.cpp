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
#include "Camera_precompiled.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "CameraComponent.h"

#include <MathConversion.h>
#include <IGameFramework.h>
#include <IRenderer.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Camera
{
    namespace ClassConverters
    {
        extern bool DeprecateCameraComponentWithoutEditor(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    CameraComponent::CameraComponent(const CameraProperties& properties)
        : m_fov(properties.m_fov)
        , m_nearClipPlaneDistance(properties.m_nearClipDistance)
        , m_farClipPlaneDistance(properties.m_farClipDistance)
        , m_specifyDimensions(properties.m_specifyFrustumDimensions)
        , m_frustumWidth(properties.m_frustumWidth)
        , m_frustumHeight(properties.m_frustumHeight)
    {
    }

    void CameraComponent::Init()
    {
        m_system = gEnv->pSystem;
        // Initialize local view.
        m_viewSystem = gEnv->pSystem->GetIViewSystem();
        if (!m_viewSystem)
        {
            AZ_Error("CameraComponent", m_viewSystem != nullptr, "The CameraComponent shouldn't be used without a local view system");
        }
    }

    void CameraComponent::Activate()
    {
        if (m_viewSystem)
        {
            if (m_view == nullptr)
            {
                m_view = m_viewSystem->CreateView();

                m_view->LinkTo(GetEntity());
                UpdateCamera();
            }
            MakeActiveView();
        }
        CameraRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        CameraBus::Handler::BusConnect();
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraAdded, GetEntityId());
    }

    void CameraComponent::Deactivate()
    {
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, GetEntityId());
        CameraBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        CameraRequestBus::Handler::BusDisconnect(GetEntityId());
        if (m_viewSystem)
        {
            if (m_view != nullptr && m_viewSystem->GetViewId(m_view) != 0)
            {
                m_view->Unlink();
            }
            if (m_viewSystem->GetActiveView() == m_view)
            {
                m_viewSystem->SetActiveView(m_prevViewId);
            }
            m_viewSystem->RemoveView(m_view);
            m_prevViewId = 0;
            m_view = nullptr;
        }
    }

    void CameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("CameraComponent", "{A0C21E18-F759-4E72-AF26-7A36FC59E477}", &ClassConverters::DeprecateCameraComponentWithoutEditor);
            serializeContext->Class<CameraComponent, AZ::Component>()
                ->Version(1)
                ->Field("Field of View", &CameraComponent::m_fov)
                ->Field("Near Clip Plane Distance", &CameraComponent::m_nearClipPlaneDistance)
                ->Field("Far Clip Plane Distance", &CameraComponent::m_farClipPlaneDistance)
                ->Field("SpecifyDimensions", &CameraComponent::m_specifyDimensions)
                ->Field("FrustumWidth", &CameraComponent::m_frustumWidth)
                ->Field("FrustumHeight", &CameraComponent::m_frustumHeight);
        }
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<CameraRequestBus>("CameraRequestBus")
                ->Event("GetNearClipDistance", &CameraRequestBus::Events::GetNearClipDistance)
                ->Event("GetFarClipDistance", &CameraRequestBus::Events::GetFarClipDistance)
                ->Event("GetFovDegrees", &CameraRequestBus::Events::GetFovDegrees)
                ->Event("SetFovDegrees", &CameraRequestBus::Events::SetFovDegrees)
                ->Event("GetFovRadians", &CameraRequestBus::Events::GetFovRadians)
                ->Event("SetFovRadians", &CameraRequestBus::Events::SetFovRadians)
                ->Event("GetFov", &CameraRequestBus::Events::GetFov) // Deprecated in 1.13
                ->Event("SetFov", &CameraRequestBus::Events::SetFov) // Deprecated in 1.13
                ->Event("SetNearClipDistance", &CameraRequestBus::Events::SetNearClipDistance)
                ->Event("SetFarClipDistance", &CameraRequestBus::Events::SetFarClipDistance)
                ->Event("MakeActiveView", &CameraRequestBus::Events::MakeActiveView)
                ->VirtualProperty("FieldOfView","GetFovDegrees","SetFovDegrees")
                ->VirtualProperty("NearClipDistance", "GetNearClipDistance", "SetNearClipDistance")
                ->VirtualProperty("FarClipDistance", "GetFarClipDistance", "SetFarClipDistance")
                ;

            behaviorContext->Class<CameraComponent>()->RequestBus("CameraRequestBus");

            behaviorContext->EBus<CameraSystemRequestBus>("CameraSystemRequestBus")
                ->Event("GetActiveCamera", &CameraSystemRequestBus::Events::GetActiveCamera)
                ;
        }
    }

    void CameraComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void CameraComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
    }

    void CameraComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
    }

    void CameraComponent::UpdateCamera()
    {
        auto viewParams = *m_view->GetCurrentParams();
        viewParams.fov = AZ::DegToRad(m_fov);
        viewParams.nearplane = m_nearClipPlaneDistance;
        viewParams.farplane = m_farClipPlaneDistance;
        m_view->SetCurrentParams(viewParams);
    }

    float CameraComponent::GetFovDegrees()
    {
        return m_fov;
    }

    float CameraComponent::GetFovRadians()
    {
        return AZ::DegToRad(m_fov);
    }

    float CameraComponent::GetNearClipDistance()
    {
        return m_nearClipPlaneDistance;
    }

    float CameraComponent::GetFarClipDistance()
    {
        return m_farClipPlaneDistance;
    }

    float CameraComponent::GetFrustumWidth()
    {
        return m_frustumWidth;
    }

    float CameraComponent::GetFrustumHeight()
    {
        return m_frustumHeight;
    }

    void CameraComponent::SetFovDegrees(float fovDegrees)
    {
        m_fov = AZ::GetClamp(fovDegrees, s_minFoV, s_maxFoV);
        UpdateCamera();
    }

    void CameraComponent::SetFovRadians(float fovRadians)
    {
        SetFovDegrees(AZ::RadToDeg(fovRadians));
    }

    void CameraComponent::SetNearClipDistance(float nearClipDistance) 
    {
        m_nearClipPlaneDistance = AZ::GetMin(nearClipDistance, m_farClipPlaneDistance);
        UpdateCamera();
    }

    void CameraComponent::SetFarClipDistance(float farClipDistance) 
    {
        m_farClipPlaneDistance = AZ::GetMax(farClipDistance, m_nearClipPlaneDistance);
        UpdateCamera();
    }

    void CameraComponent::SetFrustumWidth(float width) 
    {
        m_frustumWidth = width;
        UpdateCamera();
    }

    void CameraComponent::SetFrustumHeight(float height) 
    {
        m_frustumHeight = height;
        UpdateCamera();
    }

    void CameraComponent::MakeActiveView()
    {
        m_prevViewId = AZ::u32(m_viewSystem->GetActiveViewId());
        m_viewSystem->SetActiveView(m_view);
        UpdateCamera();
    }

    void CameraComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        CCamera& camera = m_view->GetCamera();
        camera.SetMatrix(AZTransformToLYTransform(world.GetOrthogonalized()));
    }
} //namespace Camera

