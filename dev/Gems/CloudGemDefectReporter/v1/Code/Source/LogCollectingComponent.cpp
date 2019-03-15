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

#include <LogCollectingComponent.h>
#include "DefectReportManager.h"
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace CloudGemDefectReporter
{

    LogCollectingComponent::LogCollectingComponent()
    {

    }

    void LogCollectingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LogCollectingComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LogCollectingComponent>("LogCollectingComponent", "Provides game log information when OnCollectDefectReporterData is triggered")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LogCollectingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReporterLogCollectingService"));
    }


    void LogCollectingComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void LogCollectingComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void LogCollectingComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LogCollectingComponent"));
    }

    void LogCollectingComponent::Init()
    {

    }

    void LogCollectingComponent::Activate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
    }

    void LogCollectingComponent::Deactivate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
    }

    FileBuffer LogCollectingComponent::GetLastNBytesFromBuffer(const FileBuffer& logBuffer, int numBytes) const
    {
        if (numBytes <= 0)
        {
            return FileBuffer();
        }

        if (numBytes >= logBuffer.numBytes)
        {
            return FileBuffer(logBuffer.pBuffer, logBuffer.numBytes);
        }
        else
        {
            char* pBuffer = logBuffer.pBuffer + logBuffer.numBytes - numBytes;
            return FileBuffer(pBuffer, numBytes);
        }
    }

    void LogCollectingComponent::OnCollectDefectReporterData(int reportID)
    {        
        FileBuffer logBuffer = GetLastNBytesFromGameLog(m_attachmentLogSizeInBytes);
        logBuffer.deleteBuffer = true;

        if (logBuffer.pBuffer == nullptr)
        {
            return;
        }

        AZStd::string tempLogFilePath = SaveTempLogFile(logBuffer);
        if (tempLogFilePath.empty())
        {
            return;
        }

        int handlerId = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerId, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        AZStd::vector<MetricDesc> metrics;
        {
            FileBuffer inMetricsLog = GetLastNBytesFromBuffer(logBuffer, m_inMetricsLogSizeInBytes);

            MetricDesc metricDesc;
            metricDesc.m_key = "log";
            metricDesc.m_data = AZStd::string((char*)inMetricsLog.pBuffer, inMetricsLog.numBytes);

            metrics.emplace_back(metricDesc);
        }
        
        AZStd::vector<AttachmentDesc> attachments;
        {
            AttachmentDesc attachmentDesc;
            attachmentDesc.m_autoDelete = true;
            attachmentDesc.m_name = "log";
            attachmentDesc.m_path = tempLogFilePath;
            attachmentDesc.m_type = "text/plain";
            attachmentDesc.m_extension = "txt";

            attachments.emplace_back(attachmentDesc);
        }

        CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::ReportData, reportID, handlerId, metrics, attachments);
    }

    FileBuffer LogCollectingComponent::GetLastNBytesFromGameLog(int numBytes) const
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();

        AZ::IO::HandleType fileHandle;

        if (!fileIO->Open(GetLogFilePath(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open log file");
            return FileBuffer();
        }

        char* pBuffer = new char[numBytes];
        fileIO->Seek(fileHandle, -numBytes, AZ::IO::SeekType::SeekFromEnd);
        AZ::u64 bytesRead = 0;
        fileIO->Read(fileHandle, pBuffer, numBytes, false, &bytesRead);

        return FileBuffer(pBuffer, bytesRead);
    }

    const char* LogCollectingComponent::GetLogFilePath() const
    {
        if (gEnv->IsEditor())
        {
            return "@log@/Editor.log";
        }
        return "@log@/Game.log";
    }

    const char* LogCollectingComponent::GetTempLogFileDir() const
    {
        return "@log@/LogBackups";
    }

    AZStd::string LogCollectingComponent::SaveTempLogFile(const FileBuffer& logBuffer)
    {        
        if (!CreateTempLogDirIfNotExists())
        {
            AZ_Warning("CloudCanvas", false, "Temp log folder doesn't exist");
            return "";
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        AZStd::string tempFilePath = GetTempFilePath();

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(tempFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp log file");
            return "";
        }

        fileIO->Write(fileHandle, logBuffer.pBuffer, logBuffer.numBytes);

        fileIO->Close(fileHandle);

        return tempFilePath;
    }

    AZStd::string LogCollectingComponent::GetTempFilePath() const
    {
        AZStd::string id;
        {
            AZ::Uuid uuid = AZ::Uuid::Create();
            uuid.ToString(id);
        }

        return AZStd::string::format("%s/%s", GetTempLogFileDir(), id.c_str());
    }

    bool LogCollectingComponent::CreateTempLogDirIfNotExists()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        if (!fileIO->Exists(GetTempLogFileDir()))
        {
            if (!fileIO->CreatePath(GetTempLogFileDir()))
            {
                AZ_Warning("CloudCanvas", false, "Failed to create temp log directory");
                return false;
            }
        }

        return true;
    }
}