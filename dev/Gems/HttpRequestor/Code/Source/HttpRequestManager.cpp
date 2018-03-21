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
#include "HttpRequestor_precompiled.h"

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/client/ClientConfiguration.h>

#include <AWSNativeSDKInit/AWSNativeSDKInit.h>

#include "HttpRequestManager.h"

namespace HttpRequestor
{
    const char* Manager::s_loggingName = "GemHttpRequestManager";

    Manager::Manager()
    {
        AZStd::thread_desc desc;
        desc.m_name = s_loggingName;
        m_runThread = true;
        // Shutdown will be handled by the InitializationManager - no need to call in the destructor
        AWSNativeSDKInit::InitializationManager::InitAwsApi();
        auto function = AZStd::bind(&Manager::ThreadFunction, this);
        m_thread = AZStd::thread(function, &desc);
    }

    Manager::~Manager()
    {
        // NativeSDK Shutdown does not need to be called here - will be taken care of by the InitializationManager
        m_runThread = false;
        m_requestConditionVar.notify_all();
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        m_thread.detach();
    }

    void Manager::AddRequest(Parameters && httpRequestParameters)
    {
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_requestMutex);
            m_requestsToHandle.push(AZStd::move(httpRequestParameters));
        }
        m_requestConditionVar.notify_all();
    }

    void Manager::AddTextRequest(TextParameters && httpTextRequestParameters)
    {
	    {
		    AZStd::lock_guard<AZStd::mutex> lock(m_requestMutex);
		    m_textRequestsToHandle.push(AZStd::move(httpTextRequestParameters));
	    }
	    m_requestConditionVar.notify_all();
    }

    void Manager::ThreadFunction()
    {
        // Run the thread as long as directed
        while (m_runThread)
        {
            HandleRequestBatch();
        }
    }

    void Manager::HandleRequestBatch()
    {
        // Lock mutex and wait for work to be signalled via the condition variable
        AZStd::unique_lock<AZStd::mutex> lock(m_requestMutex);
        m_requestConditionVar.wait(lock);

        // Swap queues
        AZStd::queue<Parameters> requestsToHandle;
        requestsToHandle.swap(m_requestsToHandle);

	    AZStd::queue<TextParameters> textRequestsToHandle;
	    textRequestsToHandle.swap(m_textRequestsToHandle);

        // Release lock
        lock.unlock();

        // Handle requests
        while (!requestsToHandle.empty())
        {
            HandleRequest(requestsToHandle.front());
            requestsToHandle.pop();
        }

	    while (!textRequestsToHandle.empty())
	    {
		    HandleTextRequest(textRequestsToHandle.front());
		    textRequestsToHandle.pop();
	    }
    }

    void Manager::HandleRequest(const Parameters& httpRequestParameters)
    {
        std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

        auto httpRequest = Aws::Http::CreateHttpRequest(httpRequestParameters.GetURI(), httpRequestParameters.GetMethod(), Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

        for (const auto & it : httpRequestParameters.GetHeaders())
        {
            httpRequest->SetHeaderValue(it.first.c_str(), it.second.c_str());
        }

        if( httpRequestParameters.GetBodyStream() != nullptr)
        {
            httpRequest->AddContentBody(httpRequestParameters.GetBodyStream());
        }
        
        auto httpResponse = httpClient->MakeRequest(*httpRequest);

        if (!httpResponse)
        {
            httpRequestParameters.GetCallback()(Aws::Utils::Json::JsonValue(), Aws::Http::HttpResponseCode::INTERNAL_SERVER_ERROR);
            return;
        }

        if (httpResponse->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
        {
            httpRequestParameters.GetCallback()(Aws::Utils::Json::JsonValue(), httpResponse->GetResponseCode());
            return;
        }

        Aws::Utils::Json::JsonValue json(httpResponse->GetResponseBody());
        if (json.WasParseSuccessful())
        {
            httpRequestParameters.GetCallback()(AZStd::move(json), httpResponse->GetResponseCode());
        }
        else
        {
            httpRequestParameters.GetCallback()(Aws::Utils::Json::JsonValue(), Aws::Http::HttpResponseCode::INTERNAL_SERVER_ERROR);
        }
    }

    void Manager::HandleTextRequest(const TextParameters & httpRequestParameters)
    {
        std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

        auto httpRequest = Aws::Http::CreateHttpRequest(httpRequestParameters.GetURI(), httpRequestParameters.GetMethod(), Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
        
        for (const auto & it : httpRequestParameters.GetHeaders())
        {
            httpRequest->SetHeaderValue(it.first.c_str(), it.second.c_str());
        }

        if (httpRequestParameters.GetBodyStream() != nullptr)
        {
            httpRequest->AddContentBody(httpRequestParameters.GetBodyStream());
        }

        auto httpResponse = httpClient->MakeRequest(*httpRequest);

        if (!httpResponse)
        {
            httpRequestParameters.GetCallback()(AZStd::string(), Aws::Http::HttpResponseCode::INTERNAL_SERVER_ERROR);
            return;
        }

        if (httpResponse->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
        {
            httpRequestParameters.GetCallback()(AZStd::string(), httpResponse->GetResponseCode());
            return;
        }

        // load up the raw output into a string
        // TODO(aaj): it feels like there should be some limit maybe 1 MB?
	    std::istreambuf_iterator<char> eos;
        AZStd::string data(std::istreambuf_iterator<char>(httpResponse->GetResponseBody()), eos);
	    httpRequestParameters.GetCallback()(AZStd::move(data), httpResponse->GetResponseCode());
    }
}