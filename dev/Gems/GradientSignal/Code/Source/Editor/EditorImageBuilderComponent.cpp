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

#include "GradientSignal_precompiled.h"
#include "EditorImageBuilderComponent.h"
#include <GradientSignal/ImageAsset.h>
#include <GradientSignal/ImageSettings.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QImageReader>
#include <QDirIterator>
#include <GradientSignalSystemComponent.h>

namespace GradientSignal
{
    EditorImageBuilderPluginComponent::EditorImageBuilderPluginComponent()
    {
        // AZ Components should only initialize their members to null and empty in constructor
        // after construction, they may be deserialized from file.
    }

    EditorImageBuilderPluginComponent::~EditorImageBuilderPluginComponent()
    {
    }

    void EditorImageBuilderPluginComponent::Init()
    {
    }

    void EditorImageBuilderPluginComponent::Activate()
    {
        //load qt plugins for some image file formats support
        AzQtComponents::PrepareQtPaths();

        // Activate is where you'd perform registration with other objects and systems.
        // Since we want to register our builder, we do that here:
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Gradient Image Builder";

        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tif", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tiff", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.png", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.bmp", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.jpg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.jpeg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tga", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.gif", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));

        builderDescriptor.m_busId = EditorImageBuilderWorker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&EditorImageBuilderWorker::CreateJobs, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&EditorImageBuilderWorker::ProcessJob, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        m_imageBuilder.BusConnect(builderDescriptor.m_busId);

        EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterBuilderInformation, builderDescriptor);
    }

    void EditorImageBuilderPluginComponent::Deactivate()
    {
        m_imageBuilder.BusDisconnect();
    }

    void EditorImageBuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        ImageSettings::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorImageBuilderPluginComponent, AZ::Component>()->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    EditorImageBuilderWorker::EditorImageBuilderWorker()
    {
    }

    EditorImageBuilderWorker::~EditorImageBuilderWorker()
    {
    }

    void EditorImageBuilderWorker::ShutDown()
    {
        // it is important to note that this will be called on a different thread than your process job thread
        m_isShuttingDown = true;
    }

    // this happens early on in the file scanning pass
    // this function should consistently always create the same jobs, and should do no checking whether the job is up to date or not - just be consistent.
    void EditorImageBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true, true, true);

        // Check file for _GSI suffix/pattern which assumes processing will occur whether or not settings are provided
        AZStd::string patternPath = fullPath;
        AZStd::to_upper(patternPath.begin(), patternPath.end());
        bool patternMatched = patternPath.rfind("_GSI.") != AZStd::string::npos;

        // Determine if a settings file has been provided
        AZStd::string settingsPath = fullPath + "." + s_gradientImageSettingsExtension;
        bool settingsExist = AZ::IO::SystemFile::Exists(settingsPath.data());

        // If the settings file is modified the image must be reprocessed
        AssetBuilderSDK::SourceFileDependency sourceFileDependency;
        sourceFileDependency.m_sourceFileDependencyPath = settingsPath;
        response.m_sourceFileDependencyList.push_back(sourceFileDependency);

        // If no settings file was provided then skip the file, unless the file name matches the convenience pattern
        if (!patternMatched && !settingsExist)
        {
            //do nothing if settings aren't provided
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        AZStd::unique_ptr<ImageSettings> settingsPtr;
        if (settingsExist)
        {
            settingsPtr.reset(AZ::Utils::LoadObjectFromFile<ImageSettings>(settingsPath));
        }

        // If the settings file didn't load then skip the file, unless the file name matches the convenience pattern
        if (!patternMatched && !settingsPtr)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create gradient image conversion job for %s.\nFailed loading settings %s.\n", fullPath.data(), settingsPath.data());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        // If settings loaded but processing is disabled then skip the file
        if (settingsPtr && !settingsPtr->m_shouldProcess)
        {
            //do nothing if settings disable processing
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        // Get the extension of the file
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.data(), ext, false);
        AZStd::to_upper(ext.begin(), ext.end());

        // We process the same file for all platforms
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = ext + " Compile (Gradient Image)";
            descriptor.SetPlatformIdentifier(info.m_identifier.data());
            descriptor.m_critical = false;
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        return;
    }

    // later on, this function will be called for jobs that actually need doing.
    // the request will contain the CreateJobResponse you constructed earlier, including any keys and values you placed into the hash table
    void EditorImageBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled gradient image conversion job for %s.\nCancellation requested.\n", request.m_fullPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled gradient image conversion job for %s.\nShutdown requested.\n", request.m_fullPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // Do conversion and get exported file's path
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Performing gradient image conversion job for %s\n", request.m_fullPath.data());

        //try to open the image
        QImage qimage(request.m_fullPath.data());
        if (qimage.isNull())
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed gradient image conversion job for %s.\nFailed loading source image %s.\n", request.m_fullPath.data(), request.m_fullPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        //convert to compatible pixel format
        QImage::Format format = qimage.format();
        if (qimage.format() != QImage::Format_Grayscale8)
        {
            qimage = qimage.convertToFormat(QImage::Format_Grayscale8);
        }

        //generate export file name
        QDir dir(request.m_tempDirPath.data());
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        AZStd::string fileName;
        AZStd::string outputPath;
        AzFramework::StringFunc::Path::GetFileName(request.m_fullPath.data(), fileName);
        fileName += AZStd::string(".") + s_gradientImageExtension;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.data(), fileName.data(), outputPath, true, true, true);
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Output path for gradient image conversion: %s\n", outputPath.data());

        //create a new image asset
        ImageAsset imageAsset;
        imageAsset.m_imageFormat = 0;
        imageAsset.m_imageWidth = qimage.width();
        imageAsset.m_imageHeight = qimage.height();
        imageAsset.m_imageData = AZStd::vector<AZ::u8>(imageAsset.m_imageWidth * imageAsset.m_imageHeight, 0);

        //copy image data
        for (AZ::u32 y = 0; y < imageAsset.m_imageHeight; ++y)
        {
            for (AZ::u32 x = 0; x < imageAsset.m_imageWidth; ++x)
            {
                imageAsset.m_imageData[y * imageAsset.m_imageWidth + x] = qimage.pixelColor(x, y).red();
            }
        }

        //save asset
        if (!AZ::Utils::SaveObjectToFile(outputPath, AZ::DataStream::ST_XML, &imageAsset))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed gradient image conversion job for %s.\nFailed saving output file %s.\n", request.m_fullPath.data(), outputPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // Report the image-import result
        AssetBuilderSDK::JobProduct jobProduct(outputPath);
        jobProduct.m_productAssetType = azrtti_typeid<ImageAsset>();
        jobProduct.m_productSubID = 2;
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Completed gradient image conversion job for %s.\nSucceeded saving output file %s.\n", request.m_fullPath.data(), outputPath.data());
    }

    AZ::Uuid EditorImageBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{7520DF20-16CA-4CF6-A6DB-D96759A09EE4}");
    }

} // namespace GradientSignal