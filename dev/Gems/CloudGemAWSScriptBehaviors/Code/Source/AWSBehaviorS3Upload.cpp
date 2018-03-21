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

#include "AWSBehaviorS3Upload.h"

/// To use a specific AWS API request you have to include each of these.
#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#pragma warning(pop)
#include <fstream>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

namespace CloudGemAWSScriptBehaviors
{
    static const char* UPLOAD_CLASS_TAG = "AWS:Primitive:AWSBehaviorS3Upload";

    AWSBehaviorS3Upload::AWSBehaviorS3Upload() :
        m_bucketName(),
        m_keyName(),
        m_localFileName()
    {

    }

    void AWSBehaviorS3Upload::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorS3Upload>()
                ->Field("bucketName", &AWSBehaviorS3Upload::m_bucketName)
                ->Field("keyName", &AWSBehaviorS3Upload::m_keyName)
                ->Field("localFileName", &AWSBehaviorS3Upload::m_localFileName)
                ->Field("contentType", &AWSBehaviorS3Upload::m_contentType)
                ->Version(1);
        }
    }

    void AWSBehaviorS3Upload::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorS3Upload>("AWSBehaviorS3Upload")
            ->Method("Upload", &AWSBehaviorS3Upload::Upload, nullptr, "S3 upload operation on AWS")
            ->Property("bucketName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Upload::m_bucketName))
            ->Property("keyName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Upload::m_keyName))
            ->Property("localFileName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Upload::m_localFileName))
            ->Property("contentType", nullptr, BehaviorValueSetter(&AWSBehaviorS3Upload::m_contentType));

        behaviorContext->EBus<AWSBehaviorS3UploadNotificationsBus>("AWSBehaviorS3UploadNotificationsBus")
            ->Handler<AWSBehaviorS3UploadNotificationsBusHandler>();
    }

    void AWSBehaviorS3Upload::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorS3Upload>("AWSBehaviorS3Upload", "Wraps AWS S3 functionality")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Upload::m_bucketName, "BucketName", "The name of the bucket to use")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Upload::m_keyName, "KeyName", "What to name the object being uploaded. If you do not update this as you go, the existing object will be overwritten")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Upload::m_localFileName, "LocalFileName", "Name of local file to upload.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Upload::m_contentType, "ContentType", "The mime content-type to use for the uploaded objects such as text/html, video/mpeg, video/avi, or application/zip.  This type is then stored in the S3 record and can be used to help identify what type of data you're looking at or retrieving later.");
    }

    void AWSBehaviorS3Upload::Upload()
    {
        if (m_localFileName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, "Please specify a local file name");
            return;
        }

        if (m_bucketName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, "Please specify a bucket name");
            return;
        }

        if (m_keyName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, "Please specify a key name");
            return;
        }

        if (m_contentType.empty())
        {
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, "Please specify a content type");
            return;
        }

        AZStd::string bucketName;
        EBUS_EVENT_RESULT(bucketName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, m_bucketName.c_str());

        using S3UploadRequestJob = AWS_API_REQUEST_JOB(S3, PutObject);
        S3UploadRequestJob::Config config(S3UploadRequestJob::GetDefaultConfig());
        AZStd::string region;
        EBUS_EVENT_RESULT(region, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "region");
        config.region = region.c_str();

        auto job = S3UploadRequestJob::Create(
            [](S3UploadRequestJob* job) // OnSuccess handler
        {
            AZStd::string content("File Uploaded");
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnSuccess, content.c_str());
        },
            [](S3UploadRequestJob* job) // OnError handler
        {
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, job->error.GetMessage().c_str());
        },
            &config
            );

        Aws::String awsLocalFileName(m_localFileName.c_str());
        std::shared_ptr<Aws::IOStream> fileToUpload = Aws::MakeShared<Aws::FStream>(UPLOAD_CLASS_TAG, awsLocalFileName.c_str(), std::ios::binary | std::ios::in);

        if (fileToUpload->good())
        {
            job->request.SetKey(Aws::String(m_keyName.c_str()));
            job->request.SetBucket(Aws::String(bucketName.c_str()));
            job->request.SetContentType(Aws::String(m_contentType.c_str()));
            job->request.SetBody(fileToUpload);
            job->Start();
        }
        else
        {
            Aws::StringStream ss;
            ss << strerror(errno);
            EBUS_EVENT(AWSBehaviorS3UploadNotificationsBus, OnError, ss.str().c_str());
        }
    }
}