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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "EditorCameraComponent.h"

#include <MathConversion.h>
#include <IGameFramework.h>
#include <IRenderer.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Camera
{
    void EditorCameraComponent::Init()
    {
        if (gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            if (m_system = gEnv->pGame->GetIGameFramework()->GetISystem())
            {
                // Initialize local view.
                if (!(m_viewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem()))
                {
                    AZ_Error("CameraComponent", m_viewSystem != nullptr, "The CameraComponent shouldn't be used without a local view system");
                }
            }
        }
    }

    void EditorCameraComponent::Activate()
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
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorCameraComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        CameraBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        CameraRequestBus::Handler::BusDisconnect(GetEntityId());
        if (m_viewSystem)
        {
            if (m_viewSystem->GetActiveView() == m_view)
            {
                m_viewSystem->SetActiveView(m_prevViewId);
            }
            m_viewSystem->RemoveView(m_view);
            m_prevViewId = 0;
            m_view = nullptr;
        }
    }

    void EditorCameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<EditorCameraComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Field of View", &EditorCameraComponent::m_fov)
                ->Field("Near Clip Plane Distance", &EditorCameraComponent::m_nearClipPlaneDistance)
                ->Field("Far Clip Plane Distance", &EditorCameraComponent::m_farClipPlaneDistance)
                ->Field("SpecifyDimensions", &EditorCameraComponent::m_specifyDimensions)
                ->Field("FrustumWidth", &EditorCameraComponent::m_frustumWidth)
                ->Field("FrustumHeight", &EditorCameraComponent::m_frustumHeight);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCameraComponent>("Camera", "The Camera component allows an entity to be used as a camera")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Camera")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Camera.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Camera.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(0, &EditorCameraComponent::m_fov, "Field of view", "Vertical field of view in degrees")
                        ->Attribute(AZ::Edit::Attributes::Min, MIN_FOV)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                        ->Attribute(AZ::Edit::Attributes::Step, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RadToDeg(AZ::Constants::Pi) - 0.0001f)       //We assert at fovs >= Pi so set the max for this field to be just under that
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshValues", 0x28e720d4))
                    ->DataElement(0, &EditorCameraComponent::m_nearClipPlaneDistance, "Near clip distance",
                        "Distance to the near clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, CAMERA_MIN_NEAR)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, &EditorCameraComponent::GetFarClipDistance)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->DataElement(0, &EditorCameraComponent::m_farClipPlaneDistance, "Far clip distance",
                        "Distance to the far clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, &EditorCameraComponent::GetNearClipDistance)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 10.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->DataElement(0, &EditorCameraComponent::m_specifyDimensions, "Specify dimensions",
                        "when true you can specify the dimensions of the frustum")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(0, &EditorCameraComponent::m_frustumWidth, "Frustum width",
                        "the width or horizontal resolution of the frustum")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorCameraComponent::m_specifyDimensions)
                    ->DataElement(0, &EditorCameraComponent::m_frustumHeight, "Frustum height",
                        "the height or vertical resolution of the frustum")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorCameraComponent::m_specifyDimensions)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<EditorCameraComponent>()->RequestBus("CameraRequestBus");
        }

    }

    void EditorCameraComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorCameraComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
    }

    void EditorCameraComponent::UpdateCamera()
    {
        auto viewParams = *m_view->GetCurrentParams();
        viewParams.fov = AZ::DegToRad(m_fov);
        viewParams.nearplane = m_nearClipPlaneDistance;
        viewParams.farplane = m_farClipPlaneDistance;
        m_view->SetCurrentParams(viewParams);
    }

    float EditorCameraComponent::GetFov()
    {
        return m_fov;
    }

    float EditorCameraComponent::GetNearClipDistance()
    {
        return m_nearClipPlaneDistance;
    }

    float EditorCameraComponent::GetFarClipDistance()
    {
        return m_farClipPlaneDistance;
    }

    float EditorCameraComponent::GetFrustumWidth()
    {
        return m_frustumWidth;
    }

    float EditorCameraComponent::GetFrustumHeight()
    {
        return m_frustumHeight;
    }

    void EditorCameraComponent::SetFov(float fov)
    {
        m_fov = AZ::GetClamp(fov, s_minFoV, s_maxFoV); 
        UpdateCamera();
    }

    void EditorCameraComponent::SetNearClipDistance(float nearClipDistance)
    {
        m_nearClipPlaneDistance = AZ::GetMin(nearClipDistance, m_farClipPlaneDistance); 
        UpdateCamera();
    }

    void EditorCameraComponent::SetFarClipDistance(float farClipDistance)
    {
        m_farClipPlaneDistance = AZ::GetMax(farClipDistance, m_nearClipPlaneDistance); 
        UpdateCamera();
    }

    void EditorCameraComponent::SetFrustumWidth(float width)
    {
        m_frustumWidth = width; 
        UpdateCamera();
    }

    void EditorCameraComponent::SetFrustumHeight(float height)
    {
        m_frustumHeight = height; 
        UpdateCamera();
    }

    void EditorCameraComponent::MakeActiveView()
    {
        m_prevViewId = AZ::u32(m_viewSystem->GetActiveViewId());
        m_viewSystem->SetActiveView(m_view);
        UpdateCamera();
    }

    void EditorCameraComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        CCamera& camera = m_view->GetCamera();
        camera.SetMatrix(AZTransformToLYTransform(world.GetOrthogonalized()));
    }

    void EditorCameraComponent::DisplayEntity(bool& handled)
    {
        auto* displayInterface = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        if (displayInterface)
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
            EditorDisplay(*displayInterface, transform, handled);
        }
    }

    void EditorCameraComponent::EditorDisplay(AzFramework::EntityDebugDisplayRequests& displayInterface, const AZ::Transform& world, bool& handled)
    {
        const float distance = 4.f;

        float tangent = static_cast<float>(tan(0.5f * AZ::DegToRad(m_fov)));
        float height = distance * tangent;
        float width = height * displayInterface.GetAspectRatio();

        AZ::Vector3 points[4];
        points[0] = AZ::Vector3( width, distance,  height);
        points[1] = AZ::Vector3(-width, distance,  height);
        points[2] = AZ::Vector3(-width, distance, -height);
        points[3] = AZ::Vector3( width, distance, -height);

        AZ::Vector3 start(0, 0, 0);

        displayInterface.PushMatrix(world);
        displayInterface.DrawLine(start, points[0]);
        displayInterface.DrawLine(start, points[1]);
        displayInterface.DrawLine(start, points[2]);
        displayInterface.DrawLine(start, points[3]);
        displayInterface.DrawPolyLine(points, AZ_ARRAY_SIZE(points));
        displayInterface.PopMatrix();
    }

    void EditorCameraComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        CameraProperties properties;
        properties.m_fov = m_fov;
        properties.m_nearClipDistance = m_nearClipPlaneDistance;
        properties.m_farClipDistance = m_farClipPlaneDistance;
        properties.m_specifyFrustumDimensions = m_specifyDimensions;
        properties.m_frustumWidth = m_frustumWidth;
        properties.m_frustumHeight = m_frustumHeight;
        CameraComponent* cameraComponent = gameEntity->CreateComponent<CameraComponent>(properties);
    }

} //namespace Camera

