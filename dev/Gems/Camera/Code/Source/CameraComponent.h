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

#include <AzFramework/Components/CameraBus.h>
#include <IViewSystem.h>
#include <Cry_Camera.h>

namespace Camera
{
    struct CameraProperties
    {
        float m_fov;
        float m_nearClipDistance;
        float m_farClipDistance;
        float m_frustumWidth;
        float m_frustumHeight;
        bool m_specifyFrustumDimensions;
    };

    const float s_defaultFoV = 75.0f;
    const float s_minFoV = std::numeric_limits<float>::epsilon();
    const float s_maxFoV = AZ::RadToDeg(AZ::Constants::Pi);
    const float s_defaultNearPlaneDistance = 0.2f;
    const float s_defaultFarClipPlaneDistance = 1024.0f;
    const float s_defaultFrustumDimension = 256.f;
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    class CameraComponent
        : public AZ::Component
        , public CameraRequestBus::Handler
        , public CameraBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(CameraComponent, CameraComponentTypeId, AZ::Component);
        CameraComponent() = default;
        CameraComponent(const CameraProperties& properties);
        virtual ~CameraComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

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

        void UpdateCamera();
    protected:

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
    };
} // Camera