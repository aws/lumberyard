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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"

#include "../../System.h"

#ifndef EXCLUDE_OCULUS_SDK

#include "OculusDevice.h"
#include "OculusResources.h"
#include "OculusLog.h"
#include <OculusSDK/LibOVR/Src/OVR_Stereo.h>

namespace
{
    Quat OVRQuatfToQuat(const ovrQuatf& ovrQuat)
    {
        return Quat {
                   ovrQuat.w, ovrQuat.x, ovrQuat.y, ovrQuat.z
        };
    }

    Quat OVRQuatfToQuatFixed(const ovrQuatf& ovrQuat)
    {
        return Quat {
                   ovrQuat.w, ovrQuat.x, -ovrQuat.z, ovrQuat.y
        };
    }

    Vec3 OVRVector3fToVec3(const ovrVector3f& ovrVec3)
    {
        return Vec3(ovrVec3.x, ovrVec3.y, ovrVec3.z);
    }

    Vec3 OVRVector3fToVec3Fixed(const ovrVector3f& ovrVec3)
    {
        return Vec3(ovrVec3.x, -ovrVec3.z, ovrVec3.y);
    }
}


int OculusDevice::ms_sys_hmd_reset_sensor = 0;

OculusDevice::OculusDevice()
    : m_refCount(1)
    , m_plugInState(0)
    , m_devInfo()
    , m_hmdInfo()
    , m_senInfo()
{
    memset(&m_devInfo, 0, sizeof(m_devInfo));
    memset(&m_hmdInfo, 0, sizeof(m_hmdInfo));
    memset(&m_senInfo, 0, sizeof(m_senInfo));

    ovr_Initialize();
    m_ovrDevice = ovrHmd_Create(0);

    if (!m_ovrDevice)
    {
        return;
    }

    CacheDisplayInfo();

    // Turn on full tracking recommended (not required)
    ovrHmd_ConfigureTracking(m_ovrDevice,
        ovrTrackingCap_Orientation |
        ovrTrackingCap_MagYawCorrection |
        ovrTrackingCap_Position, 0);

    ovrHmd_GetTrackingState(m_ovrDevice, ovr_GetTimeInSeconds());
}


OculusDevice::~OculusDevice()
{
    ovrHmd_Destroy(m_ovrDevice);
    ovr_Shutdown();
}


void OculusDevice::AddRef()
{
    CryInterlockedIncrement(&m_refCount);
}


void OculusDevice::Release()
{
    long refCount = CryInterlockedDecrement(&m_refCount);
#if !defined(_RELEASE)
    IF (refCount < 0, 0)
    {
        __debugbreak();
    }
#endif
    IF (refCount == 0, 0)
    {
        delete this;
    }
}

void OculusDevice::InitD3D(const D3DInitParams& params)
{
    ovrHmd_AttachToWindow(m_ovrDevice, params.window, nullptr, nullptr);

    ovrD3D11Config d3d11cfg;
    d3d11cfg.D3D11.Header.API = ovrRenderAPI_D3D11;
    d3d11cfg.D3D11.Header.RTSize = ovrSizei {
        m_ovrDevice->Resolution.w, m_ovrDevice->Resolution.h
    };
    d3d11cfg.D3D11.Header.Multisample = params.multiSample;
    d3d11cfg.D3D11.pDevice = params.d3dDevice;
    d3d11cfg.D3D11.pDeviceContext = params.deviceContext;
    d3d11cfg.D3D11.pBackBufferRT = params.backbufferView;
    d3d11cfg.D3D11.pSwapChain = params.swapChain;
    ovrHmd_ConfigureRendering(m_ovrDevice, &d3d11cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive | ovrDistortionCap_Vignette,
        m_ovrDevice->DefaultEyeFov, m_eyeDesc);

    ovrHmd_SetEnabledCaps(m_ovrDevice, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

    ovrHmd_DismissHSWDisplay(m_ovrDevice);
    ResetSensor();
}

void OculusDevice::InitEyes(const D3DEyeTextures& params)
{
    m_eyeTextures[0].D3D11.Header.API = ovrRenderAPI_D3D11;
    m_eyeTextures[0].D3D11.Header.TextureSize = ovrSizei {
        m_hmdInfo.rechResolution, m_hmdInfo.recvResolution
    };
    m_eyeTextures[0].D3D11.Header.RenderViewport = ovrRecti {
        0, 0, m_hmdInfo.rechResolution, m_hmdInfo.recvResolution
    };
    m_eyeTextures[1] = m_eyeTextures[0]; //using a texture for each eye, so viewports are the same;

    m_eyeTextures[0].D3D11.pTexture = params.leftTexture;
    m_eyeTextures[0].D3D11.pSRView = params.leftView;
    m_eyeTextures[1].D3D11.pTexture = params.rightTexture;
    m_eyeTextures[1].D3D11.pSRView = params.rightView;
}

void OculusDevice::CacheDisplayInfo()
{
    auto trackingState = ovrHmd_GetTrackingState(m_ovrDevice, ovr_GetTimeInSeconds());

    m_hmdInfo.hResolution = m_ovrDevice->Resolution.w;
    m_hmdInfo.vResolution = m_ovrDevice->Resolution.h;
    ovrSizei recommendedTex0Size = ovrHmd_GetFovTextureSize(m_ovrDevice, ovrEye_Left,
            m_ovrDevice->DefaultEyeFov[0], c_ovrPixelDensity);
    ovrSizei recommendedTex1Size = ovrHmd_GetFovTextureSize(m_ovrDevice, ovrEye_Right,
            m_ovrDevice->DefaultEyeFov[1], c_ovrPixelDensity);

    //assume left and right eye are same size for now, true for dk1 and dk2
    assert(recommendedTex0Size.w == recommendedTex1Size.w && recommendedTex0Size.h == recommendedTex1Size.h);
    //https://issues.labcollab.net/browse/LMBR-5381
    //We should be using the recommended size, but CryEngine doesn't like it when the main render targets aren't the same size as the backbuffer.
    //m_hmdInfo.rechResolution = recommendedTex0Size.w;
    //m_hmdInfo.recvResolution = recommendedTex0Size.h;
    m_hmdInfo.rechResolution = m_ovrDevice->Resolution.w;
    m_hmdInfo.recvResolution = m_ovrDevice->Resolution.h;

    cry_strcpy(m_devInfo.sensorProductName, info.ProductName);
    cry_strcpy(m_devInfo.sensorManufacturer, info.Manufacturer);
    m_devInfo.hmdVersion = m_ovrDevice->FirmwareMajor;

    strcpy_s(m_devInfo.serialNumber, info.SerialNumber);
}

OculusDevice* OculusDevice::CreateInstance()
{
    return new OculusDevice();
}


void OculusDevice::InitCVars()
{
#if !defined(_RELEASE)
    {
        static bool s_init = false;
        if (s_init)
        {
            __debugbreak();
        }
        s_init = true;
    }
#endif

    REGISTER_CVAR2("sys_hmd_reset_sensor", &ms_sys_hmd_reset_sensor, 0,
        VF_NULL, "Triggers a reset of the tracking sensor for the head mounted display device.");
}


void OculusDevice::RenderDeviceInfo()
{
    OculusDevice* pDevice = OculusResources::GetAccess().GetWrappedDevice();
    if (pDevice)
    {
        if (ms_sys_hmd_reset_sensor)
        {
            pDevice->ResetSensor();
            ms_sys_hmd_reset_sensor = 0;
        }
    }
}


void OculusDevice::TickLatencyTester()
{
}

void OculusDevice::ResetSensor()
{
    if (!m_ovrDevice)
    {
        return;
    }

    ovrHmd_RecenterPose(m_ovrDevice);
}

Quat OculusDevice::GetOrientation() const
{
    // If no device or orientation tracking is off, return identity
    if (!m_ovrDevice)
    {
        return Quat::CreateIdentity();
    }

    return OVRQuatfToQuatFixed(m_lastRotation);
}

Quat OculusDevice::GetCurrentOrientation() const
{
    // If no device or orientation tracking is off, return identity
    if (!m_ovrDevice)
    {
        return Quat::CreateIdentity();
    }

    auto trackingState = ovrHmd_GetTrackingState(m_ovrDevice, ovr_GetTimeInSeconds());
    if ((trackingState.StatusFlags & ovrStatus_OrientationTracked) == 0)
    {
        // Orientation tracking is off
        return OVRQuatfToQuatFixed(m_lastRotation);
    }

    auto eyePose = trackingState.HeadPose.ThePose;
    m_lastRotation = eyePose.Orientation;
    return OVRQuatfToQuatFixed(m_lastRotation);
}

void OculusDevice::EnablePrediction(float delta, bool enablePrediction, bool enablePredictionFilter)
{
    // No prediction for now
}

Quat OculusDevice::GetPredictedOrientation() const
{
    // No prediction for now
    return Quat::CreateIdentity();
}


IHMDDevice::FrameParams OculusDevice::BeginFrame()
{
    gEnv->pRenderer->RC_HMDBeginFrame();
    return FrameParams {};
}

void OculusDevice::EndFrame()
{
    gEnv->pRenderer->RC_HMDEndFrame();
}

IHMDDevice::EyeParams OculusDevice::BeginEye(Eye eye, float znear)
{
    ovrEyeType currentEye = Eye::Left == eye ? ovrEye_Left : ovrEye_Right;

    auto trackingState = ovrHmd_GetTrackingState(m_ovrDevice, ovr_GetTimeInSeconds());
    auto eyePose = trackingState.HeadPose.ThePose;
    eyePose.Orientation = m_lastRotation;
    //auto eyePose = ovrHmd_GetEyePose(m_ovrDevice, currentEye);
    // Cache eye poses for EndFrame call
    gEnv->pRenderer->RC_HMDSetEyePose(currentEye, &eyePose, sizeof(eyePose));
    auto scaleOffset = OVR::CreateNDCScaleAndOffsetFromFov(m_eyeDesc[currentEye].Fov);
    auto fovY = atan_tpl(m_eyeDesc[currentEye].Fov.UpTan) + atan_tpl(m_eyeDesc[currentEye].Fov.DownTan);
    auto fovX = atan_tpl(m_eyeDesc[currentEye].Fov.LeftTan) + atan_tpl(m_eyeDesc[currentEye].Fov.RightTan);
    auto eyePos = OVRVector3fToVec3Fixed(eyePose.Position);
    eyePos = Matrix33 {
        OVRQuatfToQuatFixed(m_lastRotation)
    }.GetInverted().TransformVector(eyePos);

    auto tanFov = tanf(0.5f * fovY);
    auto aspect = (scaleOffset.Scale.x * tanFov * m_hmdInfo.rechResolution) / m_hmdInfo.recvResolution;

    auto offset = (znear * (-scaleOffset.Offset.x)) / scaleOffset.Scale.x;

    return {
               eyePos, OVRVector3fToVec3Fixed(m_eyeDesc[currentEye].ViewAdjust).scale(-1.0f), offset,
               fovY, aspect
    };
}

void OculusDevice::EndEye()
{
}

void OculusDevice::RT_BeginFrame()
{
    ovrHmd_BeginFrame(m_ovrDevice, 0);
}

void OculusDevice::RT_EndFrame()
{
    ovrHmd_EndFrame(m_ovrDevice, m_eyePoses, &m_eyeTextures[0].Texture);
}

void OculusDevice::RT_SetEyePose(uint32 eye, void* pose)
{
    m_eyePoses[eye] = *reinterpret_cast<ovrPosef*>(pose);
}

#endif // #ifndef EXCLUDE_OCULUS_SDK
