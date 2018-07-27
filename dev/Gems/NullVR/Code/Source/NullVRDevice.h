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

#include <NullVR/NullVRBus.h>
#include "HMDBus.h"

namespace NullVR
{
    class NullVRDevice
        : public AZ::VR::HMDDeviceRequestBus::Handler
        , public AZ::VR::HMDInitRequestBus::Handler
        , public AZ::Component
        , protected NullVRRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NullVRDevice, "{1ECE0CC3-23F5-4F8A-AA14-5E6123E736D5}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // NullVRRequestBus interface implementation
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        /// HMDInitBus overrides ///////////////////////////////////////////////
        bool AttemptInit() override;
        void Shutdown() override;
        AZ::VR::HMDInitBus::HMDInitPriority GetInitPriority() const override;
        ////////////////////////////////////////////////////////////////////////

        /// HMDDeviceBus overrides /////////////////////////////////////////////
        void GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo) override;
        bool CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[]) override;
        void DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget) override;
        AZ::VR::TrackingState* GetTrackingState() override;
        
        void OutputHMDInfo() override;
        AZ::VR::HMDDeviceInfo* GetDeviceInfo() override;
        bool IsInitialized() override;
        const AZ::VR::Playspace* GetPlayspace() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZ::VR::HMDDeviceInfo m_deviceInfo;
        bool m_initialized = false;
        AZ::VR::TrackingState m_trackingState;
    };
}
