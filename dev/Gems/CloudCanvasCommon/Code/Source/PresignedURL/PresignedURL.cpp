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

#include <CloudCanvasCommon_precompiled.h>

#include <PresignedURL/PresignedURL.h>

#include <FileTransferSupport/FileTransferSupport.h>
#include <AzCore/EBus/EBus.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#endif

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

#include <fstream>

namespace CloudCanvas
{

    PresignedURLManager::PresignedURLManager()
    {

    }

    PresignedURLManager::~PresignedURLManager()
    {

    }

    AZ::Job* PresignedURLManager::RequestDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id)
    {
        return CreateDownloadSignedURLJob(signedURL, fileName, id);
    }

    void PresignedURLManager::RequestDownloadSignedURL(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id)
    {
        AZ::Job* downloadJob = CreateDownloadSignedURLJob(signedURL, fileName, id);
        if (downloadJob)
        {
            downloadJob->Start();
        }
    }

    AZ::Job* PresignedURLManager::CreateDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) const
    {
        if (!signedURL.length())
        {
            AZ_TracePrintf("PresignedURL","Received blank URL for file %s", fileName.c_str());
            return nullptr;
        }
        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);

        AZ_TracePrintf("PresignedURL","Requesting download from URL: %s to %s", signedURL.c_str(), fileName.c_str());

        AZStd::string outputFile = FileTransferSupport::GetResolvedFile(fileName, {});
        if (!FileTransferSupport::CheckWritableMakePath(outputFile))
        {
            AZ_TracePrintf("PresignedURL","Can't write to %s (base %s)", outputFile.c_str(),fileName.c_str());
            return nullptr;
        }
        AZ::Job* job{ nullptr };

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
         job = AZ::CreateJobFunction([signedURL, outputFile, id]()
        {
            Aws::Client::ClientConfiguration presignedConfig;
            // This timeout value is not always used consistently across http clients - it can mean "how long between packets" or 
            // "How long does the entire request take" - we're transferring files over sometimes spotty networks and don't want 
            // this arbitrarily limited.
            presignedConfig.requestTimeoutMs = 0;

            // On mobile/Slower networks we've found that the default connect timeout isn't sufficient, but we want to have
            // at least a reasonable limit.
            presignedConfig.connectTimeoutMs = 30000;

            AZStd::string caFile;
            CloudCanvas::RequestRootCAFileResult requestResult;
            EBUS_EVENT_RESULT(requestResult, CloudCanvasCommon::CloudCanvasCommonRequestBus, RequestRootCAFile, caFile);
            if (caFile.length())
            {
                AZ_TracePrintf("PresignedURL", "PresignedURL using caFile %s with request result %d", caFile.c_str(), requestResult);
                presignedConfig.caFile = caFile.c_str();
            }

            std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(presignedConfig);

            Aws::String requestURL{ signedURL.c_str() };
            auto httpRequest(Aws::Http::CreateHttpRequest(requestURL, Aws::Http::HttpMethod::HTTP_GET, nullptr));

            httpRequest->SetResponseStreamFactory([outputFile]() { return Aws::New<Aws::FStream>("TRANSFER", outputFile.c_str(), std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc); });

            auto httpResponse = httpClient->MakeRequest(*httpRequest, nullptr);

            AZStd::string returnString;
            if (!httpResponse)
            {
                AZ_TracePrintf("PresignedURL","No Response Received from request!  (Internal SDK Error)");
                if (!id.IsValid())
                {
                    EBUS_EVENT(CloudCanvas::PresignedURLResultBus, GotPresignedURLResult, signedURL, 0, returnString, outputFile);
                }
                else
                {
                    EBUS_EVENT_ID(id, CloudCanvas::PresignedURLResultBus, GotPresignedURLResult, signedURL, 0, returnString, outputFile);
                }
                return;
            }

            int responseCode = static_cast<int>(httpResponse->GetResponseCode());
            if (responseCode != static_cast<int>(Aws::Http::HttpResponseCode::OK))
            {
                auto& body = httpResponse->GetResponseBody();
                Aws::StringStream readableOut;
                readableOut << body.rdbuf();
                returnString = readableOut.str().c_str();
                AZ_TracePrintf("CloudCanvas", "PresignedURL Failed with Code %d to write to %s", responseCode,outputFile.c_str());
            }
            else
            {
                AZ_TracePrintf("CloudCanvas", "PresignedURL downloaded: %s", outputFile.c_str());
            }
            if (!id.IsValid())
            {
                EBUS_EVENT(CloudCanvas::PresignedURLResultBus, GotPresignedURLResult, signedURL, responseCode, returnString, outputFile);
            }
            else
            {
                EBUS_EVENT_ID(id, CloudCanvas::PresignedURLResultBus, GotPresignedURLResult, signedURL, responseCode, returnString, outputFile);
            }

        }, true, jobContext);
#endif
        return job;
    }
} // namespace CloudCanvas





