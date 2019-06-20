/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemDefectReporter_precompiled.h"

#include <DxDiagCollectingComponent.h>
#include <DxDiag.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

namespace CloudGemDefectReporter
{

    DxDiagCollectingComponent::DxDiagCollectingComponent()
    {

    }

    void DxDiagCollectingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DxDiagCollectingComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<DxDiagCollectingComponent>("DxDiagCollectingComponent", "Provides DxDiag information when OnCollectDefectReporterData is triggered")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void DxDiagCollectingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReporterDxDiagCollectingService"));
    }


    void DxDiagCollectingComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DxDiagCollectingComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DxDiagCollectingComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DxDiagCollectingComponent"));
    }

    void DxDiagCollectingComponent::Init()
    {

    }

    void DxDiagCollectingComponent::Activate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
    }

    void DxDiagCollectingComponent::Deactivate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
    }

    void DxDiagCollectingComponent::OnCollectDefectReporterData(int reportID)
    {  
        // only upload dxdiag when on windows platform 
#if defined(AZ_PLATFORM_WINDOWS)
        int handlerId = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerId, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);

        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([reportID, handlerId, this]()
        {
            DxDiag dxDiag;
            AZStd::string result = dxDiag.GetResult();

            std::vector<char> dxDiagContent(result.c_str(), result.c_str() + result.size() + 1);
            FileBuffer dxDiagBuffer(&dxDiagContent[0], result.length());

            if (dxDiagBuffer.pBuffer == nullptr)
            {
                return;
            }

            AZStd::string dxDiagFilePath = SaveDxDiagFile(dxDiagBuffer);
            if (dxDiagFilePath.empty())
            {
                return;
            }

            AZStd::vector<MetricDesc> metrics;

            MetricDesc metricDesc;
            metricDesc.m_key = "dxdiag";
            metricDesc.m_data = result;

            metrics.emplace_back(metricDesc);

            AZStd::vector<AttachmentDesc> attachments;
            {
                AttachmentDesc attachmentDesc;
                attachmentDesc.m_autoDelete = true;
                attachmentDesc.m_name = "dxdiag";
                attachmentDesc.m_path = dxDiagFilePath;
                attachmentDesc.m_type = "text/plain";
                attachmentDesc.m_extension = "txt";

                attachments.emplace_back(attachmentDesc);
            }

            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::ReportData, reportID, handlerId, metrics, attachments);
        }, true, jobContext);

        job->Start();
#endif        
    }

    const char* DxDiagCollectingComponent::GetDxDiagFileDir() const
    {
        return "@user@/DxDiag";
    }

    AZStd::string DxDiagCollectingComponent::GetDxDiagFilePath() const
    {
        AZStd::string id;
        {
            AZ::Uuid uuid = AZ::Uuid::Create();
            uuid.ToString(id);
        }

        return AZStd::string::format("%s/%s", GetDxDiagFileDir(), id.c_str());
    }

    bool DxDiagCollectingComponent::CreateDxDiagDirIfNotExists()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        if (!fileIO->Exists(GetDxDiagFileDir()))
        {
            if (!fileIO->CreatePath(GetDxDiagFileDir()))
            {
                AZ_Warning("CloudCanvas", false, "Failed to create DxDiag directory");
                return false;
            }
        }

        return true;
    }

    AZStd::string DxDiagCollectingComponent::SaveDxDiagFile(const FileBuffer& dxDiagBuffer)
    {
        if (!CreateDxDiagDirIfNotExists())
        {
            AZ_Warning("CloudCanvas", false, "DxDiag folder doesn't exist");
            return "";
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        AZStd::string dxDiagFilePath = GetDxDiagFilePath();

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(dxDiagFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open DxDiag file");
            return "";
        }

        fileIO->Write(fileHandle, dxDiagBuffer.pBuffer, dxDiagBuffer.numBytes);

        fileIO->Close(fileHandle);

        return dxDiagFilePath;
    }
}