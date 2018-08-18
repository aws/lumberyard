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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace CloudGemDefectReporter
{
    const int INVALID_ID = -1;

    //Maximum number of pre-signed posts that can be requested per call
    //You can find the value in Gems/CloudGemDefectReporter/v1/AWS/common-code/Constant/defect_reporter_constants.py
    //It should be changed if the config value is changed
    constexpr int s_maxNumURLPerRequest = 20;
    //Maximum number of pre-signed post calls that can be requested
    constexpr int s_maxNumPresignedPostRequests = 1;
    constexpr int s_maxNumAttachmentsToSubmit = s_maxNumURLPerRequest * s_maxNumPresignedPostRequests;

    struct AttachmentDesc
    {
        AZ_TYPE_INFO(AttachmentDesc, "{2F46CDDB-062B-4E01-83DD-DC144017A825}");
        AZ_CLASS_ALLOCATOR(AttachmentDesc, AZ::SystemAllocator, 0);

        AZStd::string m_name;       // user friendly name
        AZStd::string m_type;       // MIME type
        AZStd::string m_path;       // local path to file
        AZStd::string m_extension;  // file extension
        bool m_autoDelete = false;  // set true to have the file deleted automatically once 
                                    // uploaded to the defect service

        static void Reflect(AZ::ReflectContext* context);
    };


    struct MetricDesc
    {
        AZ_TYPE_INFO(MetricDesc, "{FACDA580-B92F-44E4-BE98-D40A71FA2DFE}");
        AZ_CLASS_ALLOCATOR(MetricDesc, AZ::SystemAllocator, 0);

        AZStd::string m_key;        // unique key for the metrics attribute
        AZStd::string m_data;       // JSON string which will be sent to metrics
                                    // Note that the string can be of maximum 255000 bytes to allow
                                    // it to fit in a metrics event
        double m_doubleVal{ 0 };

        static void Reflect(AZ::ReflectContext* context);
    };

    struct FileBuffer
    {
        FileBuffer(char* pBuffer_, int numBytes_) :
            pBuffer(pBuffer_),
            numBytes(numBytes_)
        {
        }

        FileBuffer() :
            pBuffer(nullptr),
            numBytes(0)
        {
        }

        ~FileBuffer()
        {
            if (deleteBuffer && pBuffer)
            {
                delete[] pBuffer;
            }
        }

        char* pBuffer;
        int numBytes;
        bool deleteBuffer{ false };
    };

    // data structure to contain all the information needed to create a defect report
    // typically this will be generated and maintained by the defect reporter,
    // however it's also the structure passed to the UI for display and editing and
    // can be manipulated and sent back to the defect reporter with edits
    class DefectReport
    {
    public:
        using MetricsList = AZStd::vector<MetricDesc>;
        using AttachmentList = AZStd::vector<AttachmentDesc>;

        AZ_TYPE_INFO(DefectReport, "{9FBA6CE8-18D1-450F-BE6D-511330F4D5E0}");
        AZ_CLASS_ALLOCATOR(DefectReport, AZ::SystemAllocator, 0);

        DefectReport() : m_reportID{ INVALID_ID } {}
        DefectReport(int reportID) : m_reportID{ reportID } {}
        DefectReport(int reportID, MetricsList metrics, AttachmentList attachments) :
            m_reportID{reportID},
            m_metrics{ metrics },
            m_attachments{ attachments }
        {}
        virtual ~DefectReport() {}


        int GetReportID() const { return m_reportID; }
        void SetReportID(int reportID) { m_reportID = reportID; }
        const MetricsList& GetMetrics() const { return m_metrics; }
        const AttachmentList& GetAttachments() const { return m_attachments; }

        static void Reflect(AZ::ReflectContext* context);

    protected:
        int m_reportID;
        MetricsList m_metrics;
        AttachmentList m_attachments;
    };
}
