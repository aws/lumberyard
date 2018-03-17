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

#include <CloudGemAWSScriptBehaviors_precompiled.h>

#include "AWSBehaviorS3Download.h"

#include <CloudCanvas/CloudCanvasMappingsBus.h>

/// To use a specific AWS API request you have to include each of these.
#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#pragma warning(pop)
#include <fstream>

namespace CloudGemAWSScriptBehaviors
{
    static const char* DOWNLOAD_CLASS_TAG = "AWS:Primitive:AWSBehaviorS3Download";

    AWSBehaviorS3Download::AWSBehaviorS3Download() :
        m_bucketName(),
        m_keyName(),
        m_localFileName()
    {

    }

    void AWSBehaviorS3Download::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorS3Download>()
                ->Field("bucketName", &AWSBehaviorS3Download::m_bucketName)
                ->Field("keyName", &AWSBehaviorS3Download::m_keyName)
                ->Field("localFileName", &AWSBehaviorS3Download::m_localFileName)
                ->Version(1);
        }
    }

    void AWSBehaviorS3Download::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorS3Download>("AWSBehaviorS3Download")
            ->Method("Download", &AWSBehaviorS3Download::Download, nullptr, "S3 download operation on AWS")
            ->Property("bucketName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Download::m_bucketName))
            ->Property("keyName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Download::m_keyName))
            ->Property("localFileName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Download::m_localFileName));

        behaviorContext->EBus<AWSBehaviorS3DownloadNotificationsBus>("AWSBehaviorS3DownloadNotificationsBus")
            ->Handler<AWSBehaviorS3DownloadNotificationsBusHandler>();
    }

    void AWSBehaviorS3Download::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorS3Download>("AWSBehaviorS3Download", "Wraps AWS S3 download functionality")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Download::m_bucketName, "BucketName", "The name of the bucket to use")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Download::m_keyName, "KeyName", "The name of the file on S3 to be downloaded")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Download::m_localFileName, "LocalFileName", "The local file name to use for the downloaded object");
    }

    void AWSBehaviorS3Download::Download()
    {
        if (m_localFileName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnError, "Please specify a local file name");
            return;
        }

        if (AZ::IO::FileIOBase::GetInstance()->IsReadOnly(m_localFileName.c_str()))
        {
            EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnError, "File is read only");
            return;
        }

        if (m_bucketName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnError, "Please specify a bucket name");
            return;
        }

        if (m_keyName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnError, "Please specify a key name");
            return;
        }
        using S3DownloadRequestJob = AWS_API_REQUEST_JOB(S3, GetObject);
        S3DownloadRequestJob::Config config(S3DownloadRequestJob::GetDefaultConfig());
        AZStd::string region;
        EBUS_EVENT_RESULT(region, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "region");
        config.region = region.c_str();

        auto job = S3DownloadRequestJob::Create(
            [](S3DownloadRequestJob* job) // OnSuccess handler
            {
                EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnSuccess, "File Downloaded");
            },
            [](S3DownloadRequestJob* job) // OnError handler
            {
                EBUS_EVENT(AWSBehaviorS3DownloadNotificationsBus, OnError, job->error.GetMessage().c_str());
            },
            &config
        );

        AZStd::string bucketName;
        EBUS_EVENT_RESULT(bucketName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, m_bucketName.c_str());

        job->request.SetBucket(Aws::String(bucketName.c_str()));
        job->request.SetKey(Aws::String(m_keyName.c_str()));
        Aws::String awsLocalFileName (m_localFileName.c_str());
        job->request.SetResponseStreamFactory([awsLocalFileName]() { return Aws::New<Aws::FStream>(DOWNLOAD_CLASS_TAG, awsLocalFileName.c_str(), std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc); });
        job->Start();
    }
}