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

#include "AWSBehaviorS3Presign.h"

#include <CloudGemFramework/AwsApiClientJobConfig.h>
#include <CloudGemFramework/AwsApiRequestJob.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

/// To use a specific AWS API request you have to include each of these.
#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/s3/S3Client.h>
#pragma warning(pop)

#include <aws/core/http/HttpTypes.h>

using namespace Aws::Http;

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorS3Presign::AWSBehaviorS3Presign() :
        m_bucketName(),
        m_keyName(),
        m_requestMethod()
    {

    }

    void AWSBehaviorS3Presign::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorS3Presign>()
                ->Field("bucketName", &AWSBehaviorS3Presign::m_bucketName)
                ->Field("keyName", &AWSBehaviorS3Presign::m_keyName)
                ->Field("requestMethod", &AWSBehaviorS3Presign::m_requestMethod)
                ->Version(1);
        }
    }

    void AWSBehaviorS3Presign::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorS3Presign>("AWSBehaviorS3Presign")
            ->Method("Presign", &AWSBehaviorS3Presign::Presign, nullptr, "S3 presign operation on AWS")
            ->Property("bucketName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Presign::m_bucketName))
            ->Property("keyName", nullptr, BehaviorValueSetter(&AWSBehaviorS3Presign::m_keyName))
            ->Property("requestMethod", nullptr, BehaviorValueSetter(&AWSBehaviorS3Presign::m_requestMethod));

        behaviorContext->EBus<AWSBehaviorS3PresignNotificationsBus>("AWSBehaviorS3PresignNotificationsBus")
            ->Handler<AWSBehaviorS3PresignNotificationsBusHandler>();
    }

    void AWSBehaviorS3Presign::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorS3Presign>("AWSBehaviorS3Presign", "Wraps AWS S3 presign functionality")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Presign::m_bucketName, "BucketName", "The name of the bucket to use")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorS3Presign::m_keyName, "KeyName", "What to name the key to presign against")
            ->DataElement("ComboBox", &AWSBehaviorS3Presign::m_requestMethod, "RequestMethod", "The method to presign against")
            ->Attribute("StringList", AZStd::vector<AZStd::string> {"PUT", "POST", "DELETE", "GET"});
    }

    void AWSBehaviorS3Presign::Presign()
    {
        if (m_bucketName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3PresignNotificationsBus, OnError, "Please specify a bucket name");
            return;
        }

        if (m_keyName.empty())
        {
            EBUS_EVENT(AWSBehaviorS3PresignNotificationsBus, OnError, "Please specify a key name");
            return;
        }

        HttpMethod httpMethod;

        if (m_requestMethod == "PUT")
        {
            httpMethod = HttpMethod::HTTP_PUT;
        }
        else if (m_requestMethod == "POST")
        {
            httpMethod = HttpMethod::HTTP_POST;
        }
        else if (m_requestMethod == "DELETE")
        {
            httpMethod = HttpMethod::HTTP_DELETE;
        }
        else if (m_requestMethod == "GET")
        {
            httpMethod = HttpMethod::HTTP_GET;
        }
        else
        {
            EBUS_EVENT(AWSBehaviorS3PresignNotificationsBus, OnError, "Invalid request method");
            return;
        }

        AZStd::string bucketName;
        EBUS_EVENT_RESULT(bucketName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, m_bucketName.c_str());

        CloudGemFramework::AwsApiClientJobConfig<Aws::S3::S3Client> clientConfig;
        AZStd::string region;
        EBUS_EVENT_RESULT(region, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "region");
        clientConfig.region = region.c_str();
        auto s3Client = clientConfig.GetClient();

        Aws::String url = s3Client->GeneratePresignedUrl(bucketName.c_str(), m_keyName.c_str(), httpMethod);
        if (!url.empty())
        {
            EBUS_EVENT(AWSBehaviorS3PresignNotificationsBus, OnSuccess, url.c_str());
        }
        else
        {
            EBUS_EVENT(AWSBehaviorS3PresignNotificationsBus, OnError, "Unable to generate Url");
        }
    }
}