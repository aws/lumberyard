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

#include <OSVR/OSVRBus.h>
#include <AzCore/Component/Component.h>
#include "HMDBus.h"

//OSVR Client Kit
#include <osvr/ClientKit/ClientKit.h>
#include <osvr/ClientKit/Display.h>

//OSVR Render Kit
#include <osvr/RenderKit/RenderManagerC.h>
#include <osvr/RenderKit/RenderManagerD3D11C.h>

namespace OSVR
{
    struct OSVRFrameInfo
    {
        //One RenderInfo struct for each eye
        OSVR_RenderInfoD3D11 renderInfo[STEREO_EYE_COUNT];
    };

    class OSVRDevice
        : public AZ::Component
        , public AZ::VR::HMDDeviceRequestBus::Handler
        , public AZ::VR::HMDInitRequestBus::Handler
        , private OSVR::OSVRRequestBus::Handler
    {
    public:

        AZ_COMPONENT(OSVRDevice, "{0A6F6BF4-6E41-40F8-B20B-0B2932339448}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        OSVRDevice() = default;
        ~OSVRDevice() override = default;

        /// AZ::Component //////////////////////////////////////////////////////////
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////////

        /// HMDInitBus overrides /////////////////////////////////////////////////////
        bool AttemptInit() override;
        void Shutdown() override;
        AZ::VR::HMDInitBus::HMDInitPriority GetInitPriority() const override;
        //////////////////////////////////////////////////////////////////////////////

        /// HMDDeviceBus overrides ///////////////////////////////////////////////////
        void GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo) override;
        bool CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[]) override;
        void DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget) override;
        void UpdateInternalState() override;
        AZ::VR::TrackingState* GetTrackingState() override;
        void SubmitFrame(const EyeTarget& left, const EyeTarget& right) override;
        void RecenterPose() override;
        void SetTrackingLevel(const AZ::VR::HMDTrackingLevel level) override;
        void OutputHMDInfo() override;
        AZ::VR::HMDDeviceInfo* GetDeviceInfo() override;
        bool IsInitialized() override;
        void UpdateTrackingStates() override;
        ////////////////////////////////////////////////////////////////////////////

    private:
        ///
        /// Retrieve a blob of JSON data about the OSVR interface at the given path.
        /// This will malloc a new string at the pointer that you provide and you will have to free it yourself.
        ///
        /// @param paramPath The virtual path that you want to get more detail on.
        /// @param params A reference to the string that you want to contain the parameters.
        /// @return If true, the parameters were properly allocated to the provided char**
        ///
        bool GetOSVRParameters(const char* paramPath, char*& params);

        ///
        /// Retrieve information about the current frame. This is the best
        /// way to get frame info. Do not access m_renderInfoCollection directly as
        /// this function properly double buffers render info access.
        ///
        /// @return A reference to the appropriate frame info
        ///
        OSVRFrameInfo& GetOSVRFrameInfo();

        AZ::Quaternion OSVRQuatToLYQuat(const OSVR_Quaternion& osvrQuat);
        AZ::Vector3 OSVRVec3ToLYVec3(const OSVR_Vec3& osvrVec3);

        ///OSVR Client Info
        
        /* It may seem strange that we have both C++ styled objects from the osvr namespace
        * and C style structures here. This is because the C++ API is more concise and handles
        * a lot of the nuances of OSVR much better than we can directly with the C API. For 
        * example, checking if m_displayConfig == nullptr wasn't enough to avoid a crash
        * in the AzTestRunner's integration tests but the m_cppDisp object handles validation
        * checks like that for us with no issue. 
        *
        * Why not just use the C++ API rather than the C API? The OSVR SDK makes extensive use of the 
        * C++ std which doesn't mesh well with AZStd (mainly when it comes to usage of std::string).
        * It also uses a ton of std exceptions in place of more traditional value checking.
        * Exceptions are disabled when we compile Lumberyard so we don't want to 
        * use the OSVR C++ api where we don't have to. We will retrieve the C API structs from the C++ objects
        * in AttemptInit(). 
        */
        osvr::clientkit::ClientContext* m_cppCtx = nullptr;
        osvr::clientkit::DisplayConfig* m_cppDisp = nullptr;

        OSVR_ClientContext  m_clientContext = nullptr;    ///< Main interface for querying anything about the client or establishing an OSVR_Interface
        OSVR_DisplayConfig  m_displayConfig = nullptr;    ///< Non-renderer interface to the display. Info from the RenderManager is more useful

        ///OSVR Render Manager Info

        OSVR_RenderManager                  m_renderManager = nullptr;      ///< Main interface to the render manager
        OSVR_RenderManagerD3D11             m_renderManagerD3D11 = nullptr; ///< D3D11 interface to the render manager
        OSVR_RenderParams                   m_renderParams;                 ///< Only necessary as a way to tell the presentation engine about our near and far planes
        OSVR_RenderInfoCount                m_renderInfoCount;              ///< Useful for knowing how many targets we have render info about
        OSVR_RenderInfoCollection           m_renderInfoCollection;         ///< An internal collection of OSVR_RenderInfoD3D11 structs that the RenderManager helps with. Access with GetOSVRFrameInfo;

        OSVRFrameInfo m_frameInfo[2];   ///< Collection for double buffered frame info

        AZ::VR::TrackingState m_trackingState; ///< Most recent tracking state of the HMD (not including any connected controllers).
        AZ::VR::HMDDeviceInfo m_deviceInfo;
    };
}// namespace OSVR
