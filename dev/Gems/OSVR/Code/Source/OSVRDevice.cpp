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

#include "OSVR_precompiled.h"
#include "OSVRDevice.h"

#include <IRenderer.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>
#include <MathConversion.h>

#include <d3d11.h>

//OSVR Client Kit
#include <osvr/ClientKit/ParametersC.h>
#include <osvr/Client/CreateContext.h>
#include <osvr/ClientKit/ServerAutoStartC.h>
#include <osvr/ClientKit/TransformsC.h>

//RapidJSON includes For parsing device info
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/document.h>

//For timing out
#include <AzCore/std/chrono/types.h>
#include <AzCore/std/chrono/chrono.h>

#define LogMessage(...) CryLogAlways("[HMD][OSVR] - " __VA_ARGS__);

namespace OSVR
{

    void OSVRDevice::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OSVRDevice, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<OSVRDevice>(
                    "OSVR Device Manager", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "VR")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ;
            }
        }
    }

    void OSVRDevice::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("HMDDevice"));
        provided.push_back(AZ_CRC("OSVRDevice"));
    }

    void OSVRDevice::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("OSVRDevice"));
    }

    void OSVRDevice::Activate()
    {
        //Connect to EBus to receive messages
        AZ::VR::HMDInitRequestBus::Handler::BusConnect();
    }

    void OSVRDevice::Deactivate()
    {
        AZ::VR::HMDInitRequestBus::Handler::BusDisconnect();
    }

    bool OSVRDevice::AttemptInit()
    {
        LogMessage("Attempting to start OSVR Server. ");
        osvrClientAttemptServerAutoStart();

        LogMessage("Attempting to initialize OSVR SDK");

        m_cppCtx = new osvr::clientkit::ClientContext("com.Amazon.Lumberyard");
        if (m_cppCtx == nullptr)
        {
            LogMessage("Failed to initialize OSVR SDK");
            Shutdown();
            return false;
        }

        m_cppDisp = new osvr::clientkit::DisplayConfig(*m_cppCtx);
        if (m_cppDisp == nullptr)
        {
            LogMessage("Failed to initialize OSVR Display configuration");
            Shutdown();
            return false;
        }

        if (!m_cppDisp->valid())
        {
            LogMessage("Failed to retrieve display info from server.");
            Shutdown();
            return false;
        }

        bool startupSucceded = false;
        LogMessage("Waiting for display to fully initialize...");
        {
            auto start = AZStd::chrono::system_clock::now();
            AZStd::chrono::milliseconds difference(0);
            AZStd::chrono::milliseconds timeout = AZStd::chrono::seconds(30);

            //Try for a while to connect to the display
            while (difference < timeout && !startupSucceded)
            {
                startupSucceded = m_cppDisp->checkStartup();
                m_cppCtx->update();

                difference = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::system_clock::now() - start);
            }
            if (startupSucceded != true)
            {
                LogMessage("Failed to initialize display. Message: timed out");
                Shutdown();
                return false;
            }
        }

        if (m_cppDisp->getNumViewers() < 1)
        {
            LogMessage("Failed to retrieve number of viewers.");
            Shutdown();
            return false;
        }

        /*
            Retrieve simpler C style structures from C++ classes
            See header for a more indepth explanation
        */
        m_clientContext = m_cppCtx->get();
        m_displayConfig = m_cppDisp->getDisplayConfig();

        LogMessage("Gathering additional info...");

        //Retrieve Metadata from OSVR in JSON format
        const char* path = "/display";
        char* jsonString;

        if (!GetOSVRParameters(path, jsonString))
        {
            Shutdown();
            return false;
        }

        //Parse json for info about the display in order to fill the deviceInfo
        rapidjson::Document description;
        description.Parse(jsonString);

        rapidjson::Value& hmd = description["hmd"];
        rapidjson::Value& device = hmd["device"];
        rapidjson::Value& resolutions = hmd["resolutions"];
        rapidjson::Value& fieldOfView = hmd["field_of_view"];

        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;

        //Go through every "resolution" on the display and determine the total render size
        for (rapidjson::SizeType i = 0; i < resolutions.Size(); ++i)
        {
            rapidjson::Value& resolution = resolutions[i];
            renderWidth = max(renderWidth, resolution["width"].GetUint());
            renderHeight = max(renderHeight, resolution["height"].GetUint());
        }

        m_deviceInfo.fovH = DEG2RAD(static_cast<float>(fieldOfView["monocular_horizontal"].GetDouble()));
        m_deviceInfo.fovV = DEG2RAD(static_cast<float>(fieldOfView["monocular_vertical"].GetDouble()));
        m_deviceInfo.manufacturer = device["vendor"].GetString();
        m_deviceInfo.productName = device["model"].GetString();
        m_deviceInfo.renderWidth = renderWidth >> 1;    //OSVR gives full render width of the whole device but we only want the render width for one eye
        m_deviceInfo.renderHeight = renderHeight;

        delete jsonString;

        // Connect to the HMDDeviceBus in order to get HMD messages from the rest of the VR system.
        AZ::VR::HMDDeviceRequestBus::Handler::BusConnect();
        OSVR::OSVRRequestBus::Handler::BusConnect();

        using namespace AZ::VR;
        VREventBus::Broadcast(&VREvents::OnHMDInitialized);

        return true;
    }

    AZ::VR::HMDInitBus::HMDInitPriority OSVRDevice::GetInitPriority() const
    {
        return HMDInitPriority::kLowest;
    }

    void OSVRDevice::Shutdown()
    {
        using namespace AZ::VR;
        VREventBus::Broadcast(&VREvents::OnHMDShutdown);

        AZ::VR::HMDDeviceRequestBus::Handler::BusDisconnect();
        OSVR::OSVRRequestBus::Handler::BusDisconnect();

        if (m_renderManager)
        {
            osvrDestroyRenderManager(m_renderManager);
            m_renderManager = nullptr;
        }

        if (m_cppDisp)
        {
            delete m_cppDisp;
            m_cppDisp = nullptr;
            m_displayConfig = nullptr;
        }

        if (m_cppCtx)
        {
            delete m_cppCtx;
            m_cppCtx = nullptr;
            m_clientContext = nullptr;
        }

        LogMessage("Attempting to shutdown OSVR server that we may have started");
        osvrClientReleaseAutoStartedServer();
    }

    void OSVRDevice::GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo)
    {
        //Just set some default camera info if there isn't any real info to retrieve from the render manager yet
        if (m_renderManager == nullptr)
        {
            cameraInfo = AZ::VR::PerEyeCameraInfo();
            return;
        }

        OSVR_EyeCount osvrEye = 0;
        if (eye == STEREO_EYE_RIGHT)
        {
            osvrEye = 1;
        }

        OSVRFrameInfo& frameInfo = GetOSVRFrameInfo();

        OSVR_ProjectionMatrix proj;
        proj = frameInfo.renderInfo[osvrEye].projection;

        //Sanity check what the server gives us, on the first few frames, sometimes it give us nan or within epsilon.
        if( isnan(proj.left) || isnan(proj.right) || isnan(proj.top) || isnan(proj.bottom) || isnan(proj.nearClip) ||
            abs(proj.left) < DBL_EPSILON    || abs(proj.right) < DBL_EPSILON    || abs(proj.top) < DBL_EPSILON ||
            abs(proj.bottom) < DBL_EPSILON  || abs(proj.nearClip) < DBL_EPSILON || abs(proj.farClip) < DBL_EPSILON)//nan/sanity checks
        {
            cameraInfo = AZ::VR::PerEyeCameraInfo();
            return;
        }

        float left, right, top, bottom, nearClip;
        left = static_cast<float>(proj.left);
        right = static_cast<float>(proj.right);
        top = static_cast<float>(proj.top);
        bottom = static_cast<float>(proj.bottom);
        nearClip = static_cast<float>(proj.nearClip);

        cameraInfo.aspectRatio = (right - left) / (top - bottom);
        cameraInfo.fov = atan(fabs(top) / nearClip) + atan(fabs(bottom) / nearClip);
        cameraInfo.frustumPlane.horizontalDistance = (right + left) * 0.5f;
        cameraInfo.frustumPlane.verticalDistance = (top + bottom) * 0.5f;

        //Calculate half the IPD by taking the distance between the two eyes and cutting it in half
        OSVR_Vec3* leftTranslation = &(frameInfo.renderInfo[STEREO_EYE_LEFT].pose.translation);
        AZ::Vector3 leftEyePosition = AZ::Vector3(leftTranslation->data[0], leftTranslation->data[1], leftTranslation->data[2]);

        OSVR_Vec3* rightTranslation = &(frameInfo.renderInfo[STEREO_EYE_RIGHT].pose.translation);
        AZ::Vector3 rightEyePosition = AZ::Vector3(rightTranslation->data[0], rightTranslation->data[1], rightTranslation->data[2]);

        float halfIPD = leftEyePosition.GetDistance(rightEyePosition) * 0.5f;
        if (eye == STEREO_EYE_LEFT)
        {
            halfIPD *= -1;
        }
        cameraInfo.eyeOffset = AZ::Vector3(halfIPD, 0.0f, 0.0f);
    }

    bool OSVRDevice::CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[])
    {
        HRESULT hr;

        ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(renderDevice);

        //We have to initialize the render manager here because it's the only place where the render device is passed into the system
        //If we don't pass in the device and context into OSVR it's possible that it will create a different device than the one that we create
        //these textures with. In that case OSVR will be unable to present due to a device mismatch.
        if (m_renderManager == nullptr)
        {
            LogMessage("Connecting to rendering system...");

            ID3D11DeviceContext* context = nullptr;
            d3dDevice->GetImmediateContext(&context);

            //Need to specify these or else the display cannot be opened
            OSVR_GraphicsLibraryD3D11 graphicsLibrary;
            graphicsLibrary.context = context;
            graphicsLibrary.device = d3dDevice;
            if (osvrCreateRenderManagerD3D11(m_clientContext, "Direct3D11", graphicsLibrary, &m_renderManager, &m_renderManagerD3D11) != OSVR_RETURN_SUCCESS ||
                m_renderManager == nullptr || m_renderManagerD3D11 == nullptr)
            {
                LogMessage("Could not create D3D11 RenderManager.");
                Shutdown();
                return false;
            }

            if (osvrRenderManagerGetDoingOkay(m_renderManager) != OSVR_RETURN_SUCCESS)
            {
                LogMessage("OSVR Render Manager is not doing okay! Failed to initialize");
                Shutdown();
                return false;
            }

            LogMessage("OSVR Render Manager is doing okay.");

            OSVR_OpenResultsD3D11 openResults = {};
            if (osvrRenderManagerOpenDisplayD3D11(m_renderManagerD3D11, &openResults) != OSVR_RETURN_SUCCESS || openResults.status == OSVR_OPEN_STATUS_FAILURE)
            {
                LogMessage("Could not open display with D3D11");
                Shutdown();
                return false;
            }

            LogMessage("Connected to D3D11");

            LogMessage("Setting up D3D11 Systems...")

                if (osvrRenderManagerGetDefaultRenderParams(&m_renderParams) != OSVR_RETURN_SUCCESS)
                {
                    LogMessage("Failed to query default render params.");
                    Shutdown();
                    return false;
                }

            //Replace the default render params info with our camera's info
            float nearPlane = gEnv->pRenderer->GetCamera().GetNearPlane();
            float farPlane = gEnv->pRenderer->GetCamera().GetFarPlane();
            m_renderParams.nearClipDistanceMeters = nearPlane;
            m_renderParams.farClipDistanceMeters = farPlane;

            if (osvrRenderManagerGetRenderInfoCollection(m_renderManager, m_renderParams, &m_renderInfoCollection) != OSVR_RETURN_SUCCESS)
            {
                LogMessage("Failed to retrieve render info collection.");
                Shutdown();
                return false;
            }

            if (osvrRenderManagerGetNumRenderInfoInCollection(m_renderInfoCollection, &m_renderInfoCount) != OSVR_RETURN_SUCCESS)
            {
                LogMessage("Failed to retrieve render info collection size.");
                Shutdown();
                return false;
            }
        }

        OSVR_RenderManagerRegisterBufferState renderBufferRegisterState;

        if (osvrRenderManagerStartRegisterRenderBuffers(&renderBufferRegisterState) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Could not begin registering render buffers.");
            return false;
        }

        for (uint32_t i = 0; i < eyeCount; ++i)
        {
            D3D11_TEXTURE2D_DESC textureDesc    = {};
            textureDesc.Width                   = desc.width;
            textureDesc.Height                  = desc.height;
            textureDesc.MipLevels               = 1;
            textureDesc.ArraySize               = 1;
            textureDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
            textureDesc.SampleDesc.Count        = 1;
            textureDesc.SampleDesc.Quality      = 0;
            textureDesc.Usage                   = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags               = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.CPUAccessFlags          = 0;
            textureDesc.MiscFlags               = 0;

            //Create only one texture; OSVR render manager will handle an internal swapchain
            ID3D11Texture2D* texture = nullptr;
            hr = d3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
            if (FAILED(hr))
            {
                LogMessage("Failed to create texture for render target %d", i);
                return false;
            }

            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
            renderTargetViewDesc.Format = textureDesc.Format;
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;

            //Create render target view
            ID3D11RenderTargetView* renderTargetView;

            hr = d3dDevice->CreateRenderTargetView(texture, &renderTargetViewDesc, &renderTargetView);
            if (FAILED(hr))
            {
                LogMessage("Failed to create render target view for render target %d", i);
                return false;
            }

            //This will be deleted in DestroyRenderTarget
            OSVR_RenderBufferD3D11* osvrRBD3D11 = new OSVR_RenderBufferD3D11;
            osvrRBD3D11->colorBuffer = texture;
            osvrRBD3D11->colorBufferView = renderTargetView;

            if (osvrRenderManagerRegisterRenderBufferD3D11(renderBufferRegisterState, *osvrRBD3D11) != OSVR_RETURN_SUCCESS)
            {
                LogMessage("Failed to register render buffer for eye %d", i);
                return false;
            }

            renderTargets[i]->deviceSwapTextureSet = osvrRBD3D11;
            renderTargets[i]->numTextures = 1;
            renderTargets[i]->textures = new void*[1];
            renderTargets[i]->textures[0] = texture;
        }

        if (osvrRenderManagerFinishRegisterRenderBuffers(m_renderManager, renderBufferRegisterState, false) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Could not end registering render buffers");
            return false;
        }

        return true;
    }

    void OSVRDevice::DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget)
    {
        OSVR_RenderBufferD3D11* osvrRBD3D11 = static_cast<OSVR_RenderBufferD3D11*>(renderTarget.deviceSwapTextureSet);
        delete osvrRBD3D11;
        renderTarget.deviceSwapTextureSet = nullptr;
    }

    void OSVRDevice::UpdateInternalState()
    {
        osvrClientUpdate(m_clientContext);
    }

    AZ::VR::TrackingState* OSVRDevice::GetTrackingState()
    {
        return &m_trackingState;
    }

    void OSVRDevice::SubmitFrame(const EyeTarget& left, const EyeTarget& right)
    {
        OSVR_RenderManagerPresentState presentState;

        if (osvrRenderManagerStartPresentRenderBuffers(&presentState) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to start presentation of render buffers");
            return;
        }

        //When the OSVR_ViewportDescription is returned from OSVR it contains pixel values but when we use it to
        //supply a viewport it's actually expecting clip space. It is only expecting clip space for the display seen by that eye.
        //If we were to supply one texture for both eyes, then we would use this viewport to specify which part of the texture would be seen by which eye.
        OSVR_ViewportDescription viewport;
        viewport.left = 0;
        viewport.lower = 0;
        viewport.width = 1;
        viewport.height = 1;

        OSVR_RenderBufferD3D11* leftRenderTarget = static_cast<OSVR_RenderBufferD3D11*>(left.renderTarget);
        OSVR_RenderBufferD3D11* rightRenderTarget = static_cast<OSVR_RenderBufferD3D11*>(right.renderTarget);

        OSVRFrameInfo frameInfo = GetOSVRFrameInfo();

        if (osvrRenderManagerPresentRenderBufferD3D11(presentState, *leftRenderTarget, frameInfo.renderInfo[0], viewport) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to present left eye!");
            return;
        }

        if (osvrRenderManagerPresentRenderBufferD3D11(presentState, *rightRenderTarget, frameInfo.renderInfo[1], viewport) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to present right eye!");
            return;
        }

        if (osvrRenderManagerFinishPresentRenderBuffers(m_renderManager, presentState, m_renderParams, false) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to finish presentation of render buffers");
            return;
        }
    }

    void OSVRDevice::RecenterPose()
    {
        //Resets the "room to world" transform back to an identity transform
        if (osvrClientClearRoomToWorldTransform(m_clientContext) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to clear changes to room to world transform");
            return;
        }

        //Updates internal transforms so that "-Z" is the direction that the HMD is currently facing
        if (osvrClientSetRoomRotationUsingHead(m_clientContext) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to recenter pose!");
        }
    }

    void OSVRDevice::SetTrackingLevel(const AZ::VR::HMDTrackingLevel level)
    {
        //OSVR has no configurable tracking level yet
    }

    void OSVRDevice::OutputHMDInfo()
    {
        //Some versions of the OSVR SDK have been returning UTF-8 strings that CryLogAlways doesn't like to handle properly; converting to AZ strings seems to work.
        //Without this I've had the manufacturer name return garbage when passed to CryLogAlways.
        AZStd::string deviceName = m_deviceInfo.productName != nullptr ? m_deviceInfo.productName : "unknown";
        AZStd::string manufacturerName = m_deviceInfo.manufacturer != nullptr ? m_deviceInfo.manufacturer : "unknown";

        LogMessage("Device: %s", deviceName.c_str());
        LogMessage("Manufacturer: %s", manufacturerName.c_str());
        LogMessage("Render Resolution: %dx%d", m_deviceInfo.renderWidth, m_deviceInfo.renderHeight);
        LogMessage("Horizontal FOV: %.2f degrees", RAD2DEG(m_deviceInfo.fovH));
        LogMessage("Vertical FOV: %.2f degrees", RAD2DEG(m_deviceInfo.fovV));
    }

    bool OSVRDevice::GetOSVRParameters(const char* paramPath, char*& params)
    {
        size_t jsonLength;

        if (osvrClientGetStringParameterLength(m_clientContext, paramPath, &jsonLength) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to retrieve length of display JSON.");
            return false;
        }

        params = new char[jsonLength];
        if (osvrClientGetStringParameter(m_clientContext, paramPath, params, jsonLength) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to retrieve display JSON.");
            return false;
        }

        return true;
    }

    OSVRFrameInfo& OSVRDevice::GetOSVRFrameInfo()
    {
        static IRenderer* renderer = gEnv->pRenderer;
        const int frameID = renderer->GetFrameID(false);

        return m_frameInfo[frameID & 1]; // these parameters are double-buffered.
    }

    AZ::Quaternion OSVRDevice::OSVRQuatToLYQuat(const OSVR_Quaternion& osvrQuat)
    {
        AZ::Quaternion quat(osvrQuat.data[1], osvrQuat.data[2], osvrQuat.data[3], osvrQuat.data[0]);
        AZ::Matrix3x3 m33 = AZ::Matrix3x3::CreateFromQuaternion(quat);

        AZ::Vector3 column1 = -m33.GetColumn(2);
        m33.SetColumn(2, m33.GetColumn(1));
        m33.SetColumn(1, column1);
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x3(m33);

        AZ::Quaternion result = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi) * rotation;
        return result;
    }

    AZ::Vector3 OSVRDevice::OSVRVec3ToLYVec3(const OSVR_Vec3& osvrVec3)
    {
        return AZ::Vector3(osvrVec3.data[0], -osvrVec3.data[2], osvrVec3.data[1]);
    }

    AZ::VR::HMDDeviceInfo* OSVRDevice::GetDeviceInfo()
    {
        return &m_deviceInfo;
    }

    bool OSVRDevice::IsInitialized()
    {
        return m_clientContext != nullptr;
    }

    void OSVRDevice::UpdateTrackingStates()
    {
        //If the render manager didn't fire up properly, lets not bother to update since we can't render
        if (m_renderManager == nullptr)
        {
            return;
        }

        //Retrieve render info once per frame
        if (osvrRenderManagerGetRenderInfoCollection(m_renderManager, m_renderParams, &m_renderInfoCollection) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to retrieve render info collection");
        }

        OSVR_RenderInfoD3D11 leftEyeInfo, rightEyeInfo;

        if (osvrRenderManagerGetRenderInfoFromCollectionD3D11(m_renderInfoCollection, STEREO_EYE_LEFT, &leftEyeInfo) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to retrieve left eye render info");
        }
        if (osvrRenderManagerGetRenderInfoFromCollectionD3D11(m_renderInfoCollection, STEREO_EYE_RIGHT, &rightEyeInfo) != OSVR_RETURN_SUCCESS)
        {
            LogMessage("Failed to retrieve right eye render info");
        }

        OSVRFrameInfo& frameInfo = GetOSVRFrameInfo();
        frameInfo.renderInfo[0] = leftEyeInfo;
        frameInfo.renderInfo[1] = rightEyeInfo;


        //Retrieve poses and apply them to the tracking state
        AZ::VR::TrackingState::StatusFlags flags = AZ::VR::HMDStatus_OrientationTracked | AZ::VR::HMDStatus_CameraPoseTracked;

        //Check to see if we've lost connection to the server
        if (osvrClientCheckStatus(m_clientContext) == OSVR_RETURN_SUCCESS)
        {
            flags |= AZ::VR::HMDStatus_PositionConnected | AZ::VR::HMDStatus_HmdConnected;
        }

        m_trackingState.statusFlags = flags;

        //Retrieve Pose
        OSVR_PoseState poseState;
        osvrClientGetViewerPose(m_displayConfig, 0, &poseState);

        m_trackingState.pose.position = OSVRVec3ToLYVec3(poseState.translation);
        m_trackingState.pose.orientation = OSVRQuatToLYQuat(poseState.rotation);

        //Retrieve Dynamics
        //TODO: When this is easily accessible from OSVR

    }

} //namespace OSVR