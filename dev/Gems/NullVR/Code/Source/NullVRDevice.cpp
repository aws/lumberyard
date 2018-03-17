
#include "NullVR_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <IConsole.h>

#include "NullVRDevice.h"
#include <d3d11.h>

#define LogMessage(...) CryLogAlways("[HMD][Null] - " __VA_ARGS__);

static const char* kDeviceName = "Null";

namespace NullVR
{
    void NullVRDevice::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<NullVRDevice, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<NullVRDevice>("NullVR", "Enables the VR system without a VR device.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "VR")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void NullVRDevice::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("HMDDevice"));
        provided.push_back(AZ_CRC("NullVRDevice"));
    }

    void NullVRDevice::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NullVRDevice"));
    }

    void NullVRDevice::Init()
    {
    }

    void NullVRDevice::Activate()
    {
        AZ::VR::HMDInitRequestBus::Handler::BusConnect();
    }

    void NullVRDevice::Deactivate()
    {
        AZ::VR::HMDInitRequestBus::Handler::BusDisconnect();
    }

    bool NullVRDevice::AttemptInit()
    {
        AZ::VR::HMDDeviceRequestBus::Handler::BusConnect();
        NullVRRequestBus::Handler::BusConnect();
        
        m_deviceInfo.productName = kDeviceName;
        m_deviceInfo.manufacturer = kDeviceName;

        m_initialized = true;

        using namespace AZ::VR;
        VREventBus::Broadcast(&VREvents::OnHMDInitialized);

        return true;
    }

    void NullVRDevice::Shutdown()
    {
        using namespace AZ::VR;
        VREventBus::Broadcast(&VREvents::OnHMDShutdown);

        AZ::VR::HMDDeviceRequestBus::Handler::BusDisconnect();
        NullVRRequestBus::Handler::BusDisconnect();
    }

    AZ::VR::HMDInitBus::HMDInitPriority NullVRDevice::GetInitPriority() const
    {
        return HMDInitPriority::kNullVR;
    }

    void NullVRDevice::GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo)
    {
        if (ISystem* system = GetISystem())
        {
            if (IConsole* console = system->GetIConsole())
            {
                if (ICVar* cvar = console->GetCVar("hmd_null_fov"))
                {
                    cameraInfo.fov = cvar->GetFVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_aspectRatio"))
                {
                    cameraInfo.aspectRatio = cvar->GetFVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_frustumPlane_horizontalDistance"))
                {
                    cameraInfo.frustumPlane.horizontalDistance = cvar->GetFVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_frustumPlane_verticalDistance"))
                {
                    cameraInfo.frustumPlane.verticalDistance = cvar->GetFVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_eyeOffsetX"))
                {
                    cameraInfo.eyeOffset = AZ::Vector3(cvar->GetFVal(), 0.0f, 0.0f);
                }
            }
        }
    }

    bool NullVRDevice::CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[])
    {
        for (size_t i = 0; i < eyeCount; ++i)
        {
            ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(renderDevice);

            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = desc.width;
            textureDesc.Height = desc.height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            ID3D11Texture2D* texture;
            d3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture);

            renderTargets[i]->deviceSwapTextureSet = nullptr; // this is just for the device internal render, and as we're null we don't need this
            renderTargets[i]->numTextures = 1;
            renderTargets[i]->textures = new void*[1];
            renderTargets[i]->textures[0] = texture;
        }

        return true;
    }

    void NullVRDevice::DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget)
    {
        // Note: The textures created in CreateRenderTarget and added to the renderTarget->textuers array are released in the calling function
    }

    AZ::VR::TrackingState* NullVRDevice::GetTrackingState()
    {
        return &m_trackingState;
    }

    void NullVRDevice::OutputHMDInfo()
    {
        LogMessage("Null Device");
    }

    AZ::VR::HMDDeviceInfo* NullVRDevice::GetDeviceInfo()
    {
        if (ISystem* system = GetISystem())
        {
            if (IConsole* console = system->GetIConsole())
            {
                if (ICVar* cvar = console->GetCVar("hmd_null_renderWidth"))
                {
                    m_deviceInfo.renderWidth = cvar->GetIVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_renderHeight"))
                {
                    m_deviceInfo.renderHeight = cvar->GetIVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_fovH"))
                {
                    m_deviceInfo.fovH = cvar->GetFVal();
                }

                if (ICVar* cvar = console->GetCVar("hmd_null_fovV"))
                {
                    m_deviceInfo.fovV = cvar->GetFVal();
                }
            }
        }

        return &m_deviceInfo;
    }

    bool NullVRDevice::IsInitialized()
    {
        return m_initialized;
    }

    const AZ::VR::Playspace* NullVRDevice::GetPlayspace()
    {
        return nullptr;
    }
}
