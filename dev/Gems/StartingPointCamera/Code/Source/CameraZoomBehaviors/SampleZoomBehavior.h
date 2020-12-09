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

#include <CameraFramework/ICameraZoomBehavior.h>
#include <StartingPointCamera/StartingPointCameraZoomBehaviorRequestBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    class SampleZoomBehavior
        : public Camera::ICameraZoomBehavior
        , private AZ::TickBus::Handler
        , private Camera::CameraZoomBehaviorRequestBus::Handler
   {
    public:
        ~SampleZoomBehavior() override = default;
        AZ_RTTI( SampleZoomBehavior, "{EFDA60A6-16B3-4199-BE31-1C7F105BFF25}", Camera::ICameraZoomBehavior );
        AZ_CLASS_ALLOCATOR(SampleZoomBehavior, AZ::SystemAllocator, 0);
        static void Reflect( AZ::ReflectContext* reflection );

        //////////////////////////////////////////////////////////////////////////
        // ICameraZoomBehavior
        void ModifyZoom(const AZ::Transform& targetTransform, float& inOutZoom) override;
        void Activate(AZ::EntityId) override;
        void Deactivate() override;

    private:

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handlera
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //////////////////////////////////////////////////////////////////////////
        // Camera::CameraZoomBehaviorRequestBus::Handler
        void SetZoomTarget(float zoomTarget) override;
        void SetTimeToZoom(float timeToZoom) override;
        void StartZoom() override;
        void StopZoom() override;

        static float const DefaultTimeToZoom;
        static float const DefaultZoom;

        bool m_zoomOnActivate   = false;
        float m_timeToZoom      = DefaultTimeToZoom;
        float m_zoom            = DefaultZoom;      // Current zoom modifier to apply to the camera
        float m_zoomTarget      = DefaultZoom;      // The zoom modifier that is being interpolated to
        float m_zoomVelocity    = 0.0f;             // The velocity for modifying the zoom 
                                                    // note that we use velocity and not a current time calc, 
                                                    // as then if the zoom in is interrupted then the zoom out time is correctly shorter.
   };
}
