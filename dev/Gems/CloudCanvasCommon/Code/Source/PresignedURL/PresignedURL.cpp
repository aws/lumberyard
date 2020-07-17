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
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
AZ_POP_DISABLE_WARNING
#endif

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/std/string/conversions.h>

#include <AzCore/std/string/tokenize.h>
#include <AzFramework/AzFramework_Traits_Platform.h>

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

    AZ::Job* PresignedURLManager::RequestDownloadSignedURLJobReceivedHandler(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler)
    {
        return CreateDownloadSignedURLJob(signedURL, fileName, id, std::move(dataReceivedHandler));
    }

    void PresignedURLManager::RequestDownloadSignedURLReceivedHandler(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler)
    {
        AZ::Job* downloadJob = CreateDownloadSignedURLJob(signedURL, fileName, id, std::move(dataReceivedHandler));
        if (downloadJob)
        {
            downloadJob->Start();
        }
    }

    AZ::Job* PresignedURLManager::RequestDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id)
    {
        return RequestDownloadSignedURLJobReceivedHandler(signedURL, fileName, id, {});
    }

    void PresignedURLManager::RequestDownloadSignedURL(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id)
    {
        RequestDownloadSignedURLReceivedHandler(signedURL, fileName, id, {});
    }

    AZStd::unordered_map<AZStd::string, AZStd::string> PresignedURLManager::GetQueryParameters(const AZStd::string& signedURL)
    {
        size_t queryParametersStartPosition = signedURL.find("?");
        if (queryParametersStartPosition == AZStd::string::npos)
        {
            return AZStd::unordered_map<AZStd::string, AZStd::string>();
        }

        AZStd::vector<AZStd::string> queryParameters;
        AZStd::tokenize(signedURL.substr(queryParametersStartPosition), AZStd::string("&"), queryParameters);

        AZStd::unordered_map<AZStd::string, AZStd::string> keyValuePairs;
        for (const AZStd::string& keyValuePairStr : queryParameters)
        {
            AZStd::vector<AZStd::string> queryParameterKeyAndValue;
            AZStd::tokenize(keyValuePairStr, AZStd::string("="), queryParameterKeyAndValue);

            if (queryParameterKeyAndValue.size() != 2)
            {
                AZ_Error("CloudCanvas", false, "Invalid query parameter %s", keyValuePairStr.c_str());
                return AZStd::unordered_map<AZStd::string, AZStd::string>();
            }

            keyValuePairs[queryParameterKeyAndValue[0]] = queryParameterKeyAndValue[1];
        }

        return keyValuePairs;
    }

    AZ::Job* PresignedURLManager::CreateDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler) const
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
        job = AZ::CreateJobFunction([signedURL, outputFile, id, dataReceivedHandler]()
        {
            Aws::Client::ClientConfiguration presignedConfig;
            presignedConfig.enableTcpKeepAlive = AZ_TRAIT_AZFRAMEWORK_AWS_ENABLE_TCP_KEEP_ALIVE_SUPPORTED;
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

            httpRequest->SetDataReceivedEventHandler([dataReceivedHandler](const Aws::Http::HttpRequest* request, Aws::Http::HttpResponse* response, long long amountReceived)
            {
                if (response)
                {
                    if (!response->HasHeader(Aws::Http::CONTENT_LENGTH_HEADER))
                        return;

                    const Aws::String& header = response->GetHeader(Aws::Http::CONTENT_LENGTH_HEADER);
                    AZStd::string contentLength = header.c_str();
                    uint32_t totalLength = (uint32_t)AZStd::stoi(contentLength);

                    if (dataReceivedHandler)
                    {
                        dataReceivedHandler((uint32_t)amountReceived, totalLength);
                    }
                }
            });

            auto httpResponse = httpClient->MakeRequest(httpRequest, nullptr);

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





