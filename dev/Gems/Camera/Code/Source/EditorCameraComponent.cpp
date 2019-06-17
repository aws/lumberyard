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

#include "EditorCameraComponent.h"
#include "ViewportCameraSelectorWindow.h"

#include <MathConversion.h>
#include <IGameFramework.h>
#include <IRenderer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace Camera
{
    void EditorCameraComponent::Init()
    {
        // init happens in slice compile.  We avoid reaching out to touch system resources in init.
    }

    void EditorCameraComponent::Activate()
    {
        if ((!m_viewSystem)||(!m_system))
        {
            // perform first-time init
            if (m_system = gEnv->pSystem)
            {
                // Initialize local view.
                if (!(m_viewSystem = m_system->GetIViewSystem()))
                {
                    AZ_Error("CameraComponent", m_viewSystem != nullptr, "The CameraComponent shouldn't be used without a local view system");
                }
            }
        }

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
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraAdded, GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
    }

    void EditorCameraComponent::Deactivate()
    {
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        CameraBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        CameraRequestBus::Handler::BusDisconnect(GetEntityId());
        if (m_viewSystem)
        {
            if (m_view != nullptr)
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
                ->Field("FrustumHeight", &EditorCameraComponent::m_frustumHeight)
                ->Field("ViewButton", &EditorCameraComponent::m_viewButton)
                ->Field("FrustumLengthPercent", &EditorCameraComponent::m_frustumViewPercentLength)
                ->Field("FrustumDrawColor", &EditorCameraComponent::m_frustumDrawColor)
                ;

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
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-camera.html")
                    ->DataElement(AZ::Edit::UIHandlers::Button, &EditorCameraComponent::m_viewButton, "", "Sets the view to this camera")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCameraComponent::OnPossessCameraButtonClicked)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, &EditorCameraComponent::GetCameraViewButtonText)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCameraComponent::m_fov, "Field of view", "Vertical field of view in degrees")
                        ->Attribute(AZ::Edit::Attributes::Min, MIN_FOV)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                        ->Attribute(AZ::Edit::Attributes::Step, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RadToDeg(AZ::Constants::Pi) - 0.0001f)       //We assert at fovs >= Pi so set the max for this field to be just under that
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshValues", 0x28e720d4))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCameraComponent::m_nearClipPlaneDistance, "Near clip distance",
                        "Distance to the near clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, CAMERA_MIN_NEAR)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, &EditorCameraComponent::GetFarClipDistance)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCameraComponent::m_farClipPlaneDistance, "Far clip distance",
                        "Distance to the far clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, &EditorCameraComponent::GetNearClipDistance)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 10.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Debug")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCameraComponent::m_frustumViewPercentLength, "Frustum length", "Frustum length percent .01 to 100")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " percent")
                        ->Attribute(AZ::Edit::Attributes::Step, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &EditorCameraComponent::m_frustumDrawColor, "Frustum color", "Frustum draw color RGB")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<EditorCameraComponent>()->RequestBus("CameraRequestBus");
        }

    }

    void EditorCameraComponent::OnSelected()
    {
        EditorCameraNotificationBus::Handler::BusConnect();
    }

    void EditorCameraComponent::OnDeselected()
    {
        EditorCameraNotificationBus::Handler::BusDisconnect();
    }

    void EditorCameraComponent::OnViewportViewEntityChanged(const AZ::EntityId& newViewId)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    AZ::Crc32 EditorCameraComponent::OnPossessCameraButtonClicked()
    {
        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::ShowViewPane, s_viewportCameraSelectorName);
        if (currentViewEntity != GetEntityId())
        {
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, GetEntityId());
        }
        else
        {
            // set the view entity id back to Invalid, thus enabling the editor camera
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::string EditorCameraComponent::GetCameraViewButtonText() const
    {
        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        if (currentViewEntity == GetEntityId())
        {
            return "Return to default editor camera";
        }
        else
        {
            return "Be this camera";
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

    float EditorCameraComponent::GetFovDegrees()
    {
        return m_fov;
    }

    float EditorCameraComponent::GetFovRadians()
    {
        return AZ::DegToRad(m_fov);
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

    void EditorCameraComponent::SetFovDegrees(float fov)
    {
        m_fov = AZ::GetClamp(fov, s_minFoV, s_maxFoV); 
        UpdateCamera();
    }

    void EditorCameraComponent::SetFovRadians(float fovRadians)
    {
        SetFovDegrees(AZ::RadToDeg(fovRadians));
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

    void EditorCameraComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        EditorDisplay(debugDisplay, transform);
    }

    void EditorCameraComponent::EditorDisplay(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& world)
    {
        const float distance = m_farClipPlaneDistance * m_frustumViewPercentLength * 0.01f;

        float tangent = static_cast<float>(tan(0.5f * AZ::DegToRad(m_fov)));
        float height = distance * tangent;
        float width = height * debugDisplay.GetAspectRatio();

        AZ::Vector3 farPoints[4];
        farPoints[0] = AZ::Vector3( width, distance,  height);
        farPoints[1] = AZ::Vector3(-width, distance,  height);
        farPoints[2] = AZ::Vector3(-width, distance, -height);
        farPoints[3] = AZ::Vector3( width, distance, -height);

        AZ::Vector3 start(0, 0, 0);
        AZ::Vector3 nearPoints[4];
        nearPoints[0] = farPoints[0].GetNormalizedSafe() * m_nearClipPlaneDistance;
        nearPoints[1] = farPoints[1].GetNormalizedSafe() * m_nearClipPlaneDistance;
        nearPoints[2] = farPoints[2].GetNormalizedSafe() * m_nearClipPlaneDistance;
        nearPoints[3] = farPoints[3].GetNormalizedSafe() * m_nearClipPlaneDistance;

        debugDisplay.PushMatrix(world);
        debugDisplay.SetColor(m_frustumDrawColor.GetAsVector4());
        debugDisplay.DrawLine(nearPoints[0], farPoints[0]);
        debugDisplay.DrawLine(nearPoints[1], farPoints[1]);
        debugDisplay.DrawLine(nearPoints[2], farPoints[2]);
        debugDisplay.DrawLine(nearPoints[3], farPoints[3]);
        debugDisplay.DrawPolyLine(nearPoints, AZ_ARRAY_SIZE(nearPoints));
        debugDisplay.DrawPolyLine(farPoints, AZ_ARRAY_SIZE(farPoints));
        debugDisplay.PopMatrix();
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

