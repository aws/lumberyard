
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

#include "CloudGemDefectReporter_precompiled.h"

#include "ImageLoaderSystemComponent.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <platform.h>
#include <ISystem.h>
#include <ITexture.h>
#include <IRenderer.h>

#include "jpgd.h"

namespace CloudGemDefectReporter
{

    ImageLoaderSystemComponent::ImageLoaderSystemComponent()
    {

    }

    void ImageLoaderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ImageLoaderSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ImageLoaderSystemComponent>("ImageLoaderSystemComponent", "Provides a facility to load an JPEG file in to a render target")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ImageLoaderRequestBus>("ImageLoaderRequestBus")
                ->Event("CreateRenderTargetFromImageFile", &ImageLoaderRequestBus::Events::CreateRenderTargetFromImageFile)
                ;
        }
    }

    void ImageLoaderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ImageLoaderSystemComponent"));
    }

    void ImageLoaderSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ImageLoaderSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ImageLoaderSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ImageLoaderSystemComponent"));
    }

    void ImageLoaderSystemComponent::Init()
    {
    }

    void ImageLoaderSystemComponent::Activate()
    {
        ImageLoaderRequestBus::Handler::BusConnect();
    }

    void ImageLoaderSystemComponent::Deactivate()
    {
        ImageLoaderRequestBus::Handler::BusDisconnect();
        if (m_currentTextureID != -1)
        {
            SSystemGlobalEnvironment* pEnv = GetISystem()->GetGlobalEnvironment();
            pEnv->pRenderer->RemoveTexture(m_currentTextureID);
        }
    }

    bool ImageLoaderSystemComponent::CreateRenderTargetFromImageFile(const AZStd::string& renderTargetName, const AZStd::string& imagePath)
    {
        AZStd::string filePath(imagePath.c_str());
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, filePath);

        using namespace AZ::IO;

        auto fileBase = FileIOBase::GetDirectInstance();
        if (!fileBase)
        {
            AZ_Warning("ImageLoaderSystemComponent", false, "ImageLoaderSystemComponent:File system not active when attempting to load JPEG %s", filePath.c_str());
            return false;
        }

        AZStd::string extension(PathUtil::GetExt(filePath.c_str()));
        if (extension != "jpg" && extension != "jpeg")
        {
            AZ_Warning("ImageLoaderSystemComponent", false, "ImageLoaderSystemComponent:Attempt to load non-JPEG for image file: %s", filePath.c_str());
            return false;
        }

        if (!fileBase->Exists(filePath.c_str()))
        {
            AZ_Warning("ImageLoaderSystemComponent", false, "ImageLoaderSystemComponent:JPEG file not found for image file: %s", filePath.c_str());
            return false;
        }

        HandleType fileHandle = InvalidHandle;
        if (fileBase->Open(filePath.c_str(), OpenMode::ModeRead | OpenMode::ModeBinary, fileHandle))
        {
            AZ::u64 fileSize = 0;
            fileBase->Size(fileHandle, fileSize);
            AZ::u8* readBuffer = new AZ::u8[fileSize];
            fileBase->Read(fileHandle, readBuffer, fileSize);
            fileBase->Close(fileHandle);

            jpgd::jpeg_decoder_mem_stream jpgStream(readBuffer, static_cast<jpgd::uint>(fileSize));
            int width = 0;
            int height = 0;
            int comps = 3;
            AZ::u8* imgData = decompress_jpeg_image_from_stream(&jpgStream, &width, &height, &comps, comps);
            if (!imgData)
            {
                AZ_Warning("ImageLoaderSystemComponent", false, "ImageLoaderSystemComponent:Unable to decode JPEG file for image file: %s", filePath.c_str());
                delete[] readBuffer;
                return false;
            }

            // convert 24 bit RGB to 32 bit RGBA
            byte* data = NULL;
            data = new byte[width * height * 4];
            for (int i = 0; i < width * height; ++i)
            {
                data[i * 4 + 0] = imgData[i * 3 + 0];
                data[i * 4 + 1] = imgData[i * 3 + 1];
                data[i * 4 + 2] = imgData[i * 3 + 2];
                data[i * 4 + 3] = 255;
            }

            SSystemGlobalEnvironment* pEnv = GetISystem()->GetGlobalEnvironment();
            // destroy previous texture (of potentially same name) before trying to create new texture
            if (m_currentTextureID != -1)
            {
                pEnv->pRenderer->RemoveTexture(m_currentTextureID);
            }

            int textureID = pEnv->pRenderer->DownLoadToVideoMemory(data,
                width,
                height,
                eTF_R8G8B8A8,
                eTF_R8G8B8A8,
                1,
                false,
                FILTER_BILINEAR,
                0,
                renderTargetName.c_str());

            delete[] readBuffer;
            delete[] data;

            ITexture* texture = pEnv->pRenderer->EF_GetTextureByID(textureID);
            if (!texture || !texture->IsTextureLoaded())
            {
                AZ_Warning("ImageLoaderSystemComponent", false, "ImageLoaderSystemComponent:No JPEG texture created for image file: %s", filePath.c_str());
                return false;
            }
            m_currentTextureID = textureID;
            m_renderTargetName = renderTargetName;
            
            return true;
        }
        return false;
    }

    AZStd::string  ImageLoaderSystemComponent::GetRenderTargetName()
    {
        return m_renderTargetName;
    }

} // namespace CloudGemDefectReporter


