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
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>
#include <CloudGemDefectReporter/DefectReporterDataStructures.h>

namespace CloudGemDefectReporter
{
    class LogCollectingComponent
        : public AZ::Component,
          public CloudGemDefectReporterNotificationBus::Handler
    {
    public:
        LogCollectingComponent();
        AZ_COMPONENT(LogCollectingComponent, "{ccec2960-fbf5-11e7-8c3f-9a214cf093ae}");

        static void Reflect(AZ::ReflectContext* context);

    protected:        
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterNotificationBus::Handler interface implementation
        virtual void OnCollectDefectReporterData(int reportID) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::string GetTempFilePath() const;
        AZStd::string SaveTempLogFile(const FileBuffer& logBuffer);
        FileBuffer GetLastNBytesFromBuffer(const FileBuffer& logBuffer, int numBytes) const;
        FileBuffer GetLastNBytesFromGameLog(int numBytes) const;
        const char* GetLogFilePath() const;
        const char* GetTempLogFileDir() const;
        bool CreateTempLogDirIfNotExists();

        int m_inMetricsLogSizeInBytes{1024*10}; // 10k
        int m_attachmentLogSizeInBytes{1024*1024}; // 1M
    };
}
