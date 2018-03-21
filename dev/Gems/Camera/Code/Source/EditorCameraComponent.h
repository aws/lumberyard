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
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzFramework/Components/CameraBus.h>

#include <AzFramework/Components/EditorEntityEvents.h>
#include "CameraComponent.h"
#include <IViewSystem.h>
#include <Cry_Camera.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    class EditorCameraComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public CameraRequestBus::Handler
        , public CameraBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorCameraNotificationBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCameraComponent, EditorCameraComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        virtual ~EditorCameraComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CameraRequestBus::Handler
        float GetFovDegrees() override;
        float GetFovRadians() override;
        float GetNearClipDistance() override;
        float GetFarClipDistance() override;
        float GetFrustumWidth() override;
        float GetFrustumHeight() override;
        void SetFovDegrees(float fov) override;
        void SetFovRadians(float fov) override;
        void SetNearClipDistance(float nearClipDistance) override;
        void SetFarClipDistance(float farClipDistance) override;
        void SetFrustumWidth(float width) override;
        void SetFrustumHeight(float height) override;
        void MakeActiveView() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CameraBus::Handler
        AZ::EntityId GetCameras() override { return GetEntityId(); }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayRequestBus::Handler
        void DisplayEntity(bool& handled) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// EditorCameraNotificationBus::Handler
        void OnViewportViewEntityChanged(const AZ::EntityId& newViewId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;
        //////////////////////////////////////////////////////////////////////////
    protected:
        void UpdateCamera();
        void EditorDisplay(AzFramework::EntityDebugDisplayRequests& displayInterface, const AZ::Transform& world, bool& handled);
        AZ::Crc32 OnPossessCameraButtonClicked();
        AZStd::string GetCameraViewButtonText() const;

        //////////////////////////////////////////////////////////////////////////
        /// Private Data
        //////////////////////////////////////////////////////////////////////////
        IView* m_view = nullptr;
        AZ::u32 m_prevViewId = 0;
        IViewSystem* m_viewSystem = nullptr;
        ISystem* m_system = nullptr;

        //////////////////////////////////////////////////////////////////////////
        /// Reflected Data
        //////////////////////////////////////////////////////////////////////////
        float m_fov = s_defaultFoV;
        float m_nearClipPlaneDistance = s_defaultNearPlaneDistance;
        float m_farClipPlaneDistance = s_defaultFarClipPlaneDistance;
        bool m_specifyDimensions = false;
        float m_frustumWidth = s_defaultFrustumDimension;
        float m_frustumHeight = s_defaultFrustumDimension;
        bool m_viewButton = false;
        float m_frustumViewPercentLength = 1.f;
        AZ::Color m_frustumDrawColor = AZ::Color{1.0f, 0.f, 0.f, 1.f};
    };
} // Camera