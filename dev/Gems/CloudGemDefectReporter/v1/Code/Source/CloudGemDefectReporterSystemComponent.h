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

#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>
#include <CrySystemBus.h>
#include <AzCore/Component/TickBus.h>

#include "DefectReportManager.h"
#include <InputRequestBus.h>

namespace CloudGemDefectReporter
{
    class CloudGemDefectReporterSystemComponent
        : public AZ::Component
        , protected CloudGemDefectReporterRequestBus::Handler
        , protected CrySystemEventBus::Handler
        , protected AZ::SystemTickBus::Handler
        , protected CloudGemDefectReporterNotificationBus::Handler  // to grab screenshots as they are special
    {
    public:
        AZ_COMPONENT(CloudGemDefectReporterSystemComponent, "{D57FC674-9E20-4EE0-9D83-AC0102AFC2BC}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterRequestBus interface implementation
        virtual int TriggerDefectReport(bool immediate) override;
        virtual int GetHandlerID(int reportID) override;
        virtual void ReportData(int reportID,
            int handlerID,
            AZStd::vector<MetricDesc> metrics,
            AZStd::vector<AttachmentDesc> attachments) override;
        virtual void TriggerUserReportEditing() override;
        virtual AZStd::vector<int> GetAvailableReportIDs() override;
        virtual DefectReport GetReport(int reportID) override;
        virtual void UpdateReport(DefectReport report) override;
        virtual void AddAnnotation(int reportID, AZStd::string annotation) override;
        virtual AZStd::string GetInputRecord(AZStd::string processedEventName) override;
        virtual void GetClientConfiguration() override;
        virtual void AddCustomField(int reportID, AZStd::string key, AZStd::string value) override;
        virtual void RemoveReport(int reportID) override;
        virtual void PostReports(AZStd::vector<int> reportIDs) override;
        virtual void FlushReports(AZStd::vector<int> reportIDs) override;
        virtual void AttachmentUploadComplete(AZStd::string attachmentPath, bool autoDelete) override;
        virtual void BackupCompletedReports() override;
        virtual void IsSubmittingReport(bool status) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    
        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus interface implementation
        virtual void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SystemTickBus interface implementation
        virtual void OnSystemTick() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterNotificationBus interface implementation
        // This is used to handle the screen shot capture, which is always part of the report
        // and is fairly specialized in how it's reported, so the code is here rather than
        // in an optional handler
        virtual void OnCollectDefectReporterData(int reportID) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        DefectReportManager m_reportManager;
        int m_prevAvailableReports = -1;
        int m_prevPendingReports = -1;
        AZStd::vector<AZStd::string> pendingScreenShots;

        void updateScreenShotSettings();
        bool CheckSoftCap();
    };
}
