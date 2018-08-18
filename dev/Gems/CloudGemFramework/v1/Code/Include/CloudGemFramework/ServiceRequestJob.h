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

#include <AzCore/std/functional.h>
#include <CloudGemFramework/ServiceClientJob.h>
#include <CloudGemFramework/JsonObjectHandler.h>
#include <CloudGemFramework/JsonWriter.h>
#include <CloudGemFramework/Error.h>
#include <CloudGemFramework/ServiceRequestJobConfig.h>
#include <CloudGemFramework/RequestBuilder.h>
#include <CloudGemFramework/ServiceJobUtil.h>

#include <aws/core/http/HttpResponse.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <AzCore/Component/TickBus.h>

namespace CloudGemFramework
{

    /// Base class for service requests. To use, derive a class and
    /// then provide that class as the argument to the ServiceRequestJob
    /// template class.
    ///
    /// This class provide defaults, but many of these need to be 
    /// overridden in the derived type for most requests, and ServiceTraits
    /// must be overriden for all requests. Use the SERVICE_REQUEST
    /// macro to implement the common overrides.
    class ServiceRequest
    {

    public:

        /// Macro used in derived classes to perform the common overrides.
        #define SERVICE_REQUEST(SERVICE_NAME, METHOD, PATH) \
                using ServiceTraits = SERVICE_NAME##ServiceTraits; \
                static const char* Path() { return PATH; } \
                static HttpMethod Method() { return METHOD; }

        /// ServiceTraits must be overridden by the derived type.
        using ServiceTraits = void;

        using HttpMethod = Aws::Http::HttpMethod;

        /// Must be overriden if the request method is not GET.
        static HttpMethod Method()
        {
            return HttpMethod::HTTP_GET;
        }

        /// Must be overriden if the request requries an URL path.
        /// By default the service url alone will be used.
        static const char* Path()
        {
            return ""; // no path
        }

        /// Type used for request parameters. If the request has 
        /// parameters, define a parameters type and use it to 
        /// override the parameters member.
        struct NoParameters
        {

            bool BuildRequest(CloudGemFramework::RequestBuilder& request)
            {
                return true; // ok
            }

        };

        /// Stores parameter values. Must be overridden if the
        /// request has parameters.
        NoParameters parameters;
            
        /// Type used for result data. If the request has result data,
        /// define a result type and use it to override the result member.
        struct EmptyResult
        {

            bool OnJsonKey(const char* key, CloudGemFramework::JsonReader& reader)
            {
                return reader.Ignore();
            }

        };
            
        /// Stores result data. Must be overridden if the request has 
        /// result data.
        EmptyResult result;

        /// Stores error information should the request fail. There is no 
        /// need to override this member and doing so will waste a little
        /// memory.
        Error error;

        /// Determines if the AWS credentials, as supplied by the credentialsProvider from
        /// the ServiceReqestJobConfig object (which defaults to the player's credentials), 
        /// are used to sign the request. The default is true. Override this and return false 
        /// if you have a public API and want to avoid the overhead of signing requests.
        bool UseAWSCredentials() {
            return true;
        }

    };

    /// Base class for Cloud Gem service request jobs.
    template<class RequestType>
    class ServiceRequestJob
        : public ServiceClientJob<typename RequestType::ServiceTraits>
        , public RequestType
    {

    public:

        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(ServiceRequestJob, AZ::SystemAllocator, 0);

        /// Aliases for the configuration types used for this job class.
        using IConfig = IServiceRequestJobConfig;
        using Config = ServiceRequestJobConfig<RequestType>;

        /// An alias for this type, useful in derived classes.
        using ServiceRequestJobType = ServiceRequestJob<RequestType>;
        using ServiceClientJobType = ServiceClientJob<typename RequestType::ServiceTraits>;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(ServiceClientJobType::GetDefaultConfig());
        }

        ServiceRequestJob(bool isAutoDelete, IConfig* config = GetDefaultConfig())
            : ServiceClientJobType{isAutoDelete, config}
            , m_requestUrl{config->GetRequestUrl()}
        {
            if (RequestType::UseAWSCredentials())
            {
                if (!HasCredentials(config))
                {
                    m_missingCredentials = true;
                    return;
                }

                m_AWSAuthSigner.reset(
                    new Aws::Client::AWSAuthV4Signer{
                        config->GetCredentialsProvider(),
                        "execute-api",
                        DetermineRegionFromRequestUrl()
                    }
                );
            }
        }

        bool HasCredentials(IConfig* config)
        {
            if (config == nullptr)
            {
                return false;
            }

            if (!config->GetCredentialsProvider())
            {
                return false;
            }

            auto awsCredentials = config->GetCredentialsProvider()->GetAWSCredentials();

            return !(awsCredentials.GetAWSAccessKeyId().empty() || awsCredentials.GetAWSSecretKey().empty());
        }

        /// Returns true if no error has occured.
        bool WasSuccess()
        {
            return RequestType::error.type.empty();
        }

        /// Override AZ:Job defined method to reset request state when
        /// the job object is reused.
        void Reset(bool isClearDependent) override
        {
            RequestType::parameters = decltype(RequestType::parameters)();
            RequestType::result = decltype(RequestType::result)();
            RequestType::error = decltype(RequestType::error)();
            ServiceClientJobType::Reset(isClearDependent);
        }

    protected:

        /// The URL created by appending the API path to the service URL.
        /// The path may contain {param} format parameters. The 
        /// RequestType::parameters.BuildRequest method is responsible
        /// for replacing these parts of the url.
        const Aws::String& m_requestUrl;

        std::shared_ptr<Aws::Client::AWSAuthV4Signer> m_AWSAuthSigner{nullptr};

        // Our configuration which is passed in contains our AWS Credentials - If this request requires credentials 
        // we check in the constructor and set this bool to indicate if we're not valid before placing the credentials
        // in our m_AWSAuthSigner
        bool m_missingCredentials{ false };
        /// Called to prepare the request. By default no changes
        /// are made to the parameters object. Override to defer the preperation
        /// of parametrs until your running on the job's worker thread,
        /// instead of setting parameters before calling Start.
        ///
        /// \return true if the request was prepared successfully.
        virtual bool PrepareRequest()
        {
            return IsValid();
        }

        virtual bool IsValid() const
        {
            return m_requestUrl.length() && !m_missingCredentials;
        }

        /// Called when a request completes without error.
        virtual void OnSuccess()
        {
        }

        /// Called when an error occurs.
        virtual void OnFailure()
        {
        }

        // So our derived functions which do not auto delete can clean up
        virtual void DoCleanup()
        {
            
        }
    private:

        bool BuildRequest(RequestBuilder& request) override
        {
            bool ok = PrepareRequest();
            if(ok)
            {
                request.SetHttpMethod(RequestType::Method());
                request.SetRequestUrl(m_requestUrl);
                request.SetAWSAuthSigner(m_AWSAuthSigner);
                ok = RequestType::parameters.BuildRequest(request);
                if(!ok)
                {
                    RequestType::error.type = Error::TYPE_CONTENT_ERROR;
                    RequestType::error.message = request.GetErrorMessage();
                    OnFailure();
                }
            }
            return ok;
        }

        void ProcessResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response) override
        {
            if (ServiceClientJobType::IsCancelled())
            {
                RequestType::error.type = Error::TYPE_NETWORK_ERROR;
                RequestType::error.message = "Job cancelled while waiting for a response.";
            }
            else if(!response)
            {
                RequestType::error.type = Error::TYPE_NETWORK_ERROR;
                RequestType::error.message = "An unknown error occured while making the request.";
            }
            else
            {

#ifdef _DEBUG
                // we assume application/json; charset=utf-8
                const Aws::String& contentType = response->GetContentType();
                AZ_Error(
                    ServiceClientJobType::COMPONENT_DISPLAY_NAME, 
                    contentType.find("application/json") != AZStd::string::npos && 
                        (contentType.find("charset") == AZStd::string::npos || 
                            contentType.find("utf-8") != AZStd::string::npos),
                    "Service response content type is not application/json; charset=utf-8: %s", contentType.c_str()
                );
#endif

                int responseCode = static_cast<int>(response->GetResponseCode());
                Aws::IOStream& responseBody = response->GetResponseBody();
#ifdef _DEBUG
                std::istreambuf_iterator<AZStd::string::value_type> eos;
                AZStd::string responseContent = AZStd::string{std::istreambuf_iterator<AZStd::string::value_type>(responseBody),eos};
                AZ_Printf(ServiceClientJobType::COMPONENT_DISPLAY_NAME, "Processing %d response: %s.", responseCode, responseContent.c_str());
                responseBody.clear();
                responseBody.seekg(0);
#endif
                JsonInputStream stream{responseBody};

                if(responseCode >= 200 && responseCode <= 299)
                {
                    ReadResponseObject(stream);
                }
                else
                {
                    ReadErrorObject(responseCode, stream);
                }
                    
            }

            if (WasSuccess())
            {
                OnSuccess();
            }
            else
            {

                AZStd::string requestContent;
                AZStd::string responseContent;
                if(response)
                {

                    // Get request and response content. Not attempting to do any charset
                    // encoding/decoding, so the display may not be correct but it should 
                    // work fine for the usual ascii and utf8 content.

                    std::istreambuf_iterator<AZStd::string::value_type> eos;

                    std::shared_ptr<Aws::IOStream> requestStream = response->GetOriginatingRequest().GetContentBody();
                    if (requestStream)
                    {
                        requestStream->clear();
                        requestStream->seekg(0);
                        requestContent = AZStd::string{std::istreambuf_iterator<AZStd::string::value_type>(*requestStream.get()),eos};
                        // Replace the character "%" with "%%" to prevent the error when printing the string that contains the percentage sign
                        AZStd::size_t start_pos = requestContent.find("%", 0);
                        while (start_pos != AZStd::string::npos) {
                            requestContent.replace(start_pos, 1, "%%");
                            start_pos = requestContent.find("%", start_pos + 2);
                        }
                    }

                    Aws::IOStream& responseStream = response->GetResponseBody();
                    responseStream.clear();
                    responseStream.seekg(0);
                    responseContent = AZStd::string{std::istreambuf_iterator<AZStd::string::value_type>(responseStream),eos};

                }

                AZStd::string message = AZStd::string::format("An %s error occurred when performing %s %s on service %s using %s: %s\n\nRequest Content:\n%s\n\nResponse Content:\n%s\n\n",
                    RequestType::error.type.c_str(),
                    ServiceRequestJobType::HttpMethodToString(RequestType::Method()),
                    RequestType::Path(),
                    RequestType::ServiceTraits::ServiceName,
                    response ? response->GetOriginatingRequest().GetURIString().c_str() : "NULL",
                    RequestType::error.message.c_str(),
                    requestContent.c_str(),
                    responseContent.c_str()
                );

                // This is determined by AZ::g_maxMessageLength defined in in dev\Code\Framework\AzCore\AzCore\Debug\Trace.cpp.
                // It has the value 4096, but there is the timestamp, etc., to account for so we reduce it by a few characters.
                const int MAX_MESSAGE_LENGTH = 4096 - 128;

                if (message.size() > MAX_MESSAGE_LENGTH)
                {
                    int offset = 0;
                    while (offset < message.size())
                    {
                        int count = (offset + MAX_MESSAGE_LENGTH < message.size()) ? MAX_MESSAGE_LENGTH : message.size() - offset;
                        AZ_Warning(ServiceClientJobType::COMPONENT_DISPLAY_NAME, false, message.substr(offset, count).c_str());
                        offset += MAX_MESSAGE_LENGTH;
                    }

                }
                else
                {
                     AZ_Warning(ServiceClientJobType::COMPONENT_DISPLAY_NAME, false, message.c_str());
                }

                OnFailure();

            }

        }

        void ReadErrorObject(int responseCode, JsonInputStream& stream)
        {
            AZStd::string parseErrorMessage;
            bool ok = JsonReader::ReadObject(stream, RequestType::error, parseErrorMessage);
            ok = ok && !RequestType::error.message.empty();
            ok = ok && !RequestType::error.type.empty();
            if(!ok)
            {
                if (responseCode < 400)
                {
                    // Not expected to make it here: 100 (info), 200 (success), or 300 (redirect).
                    RequestType::error.type = Error::TYPE_CONTENT_ERROR;
                    RequestType::error.message = AZStd::string::format("Unexpected response code %i received. %s", responseCode, stream.GetContent().c_str());
                }
                else if (responseCode < 500)
                {
                    RequestType::error.type = Error::TYPE_CLIENT_ERROR;
                    switch (responseCode)
                    {
                    case 401:
                    case 403:
                        RequestType::error.message = AZStd::string::format("Access denied (%i). %s", responseCode, stream.GetContent().c_str());
                        break;
                    case 404:
                        RequestType::error.message = AZStd::string::format("Not found (%i). %s", responseCode, stream.GetContent().c_str());
                        break;
                    case 405:
                        RequestType::error.message = AZStd::string::format("Method not allowed (%i). %s", responseCode, stream.GetContent().c_str());
                        break;
                    case 406:
                        RequestType::error.message = AZStd::string::format("Content not acceptable (%i). %s", responseCode, stream.GetContent().c_str());
                        break;
                    default:
                        RequestType::error.message = AZStd::string::format("Client error (%i). %s", responseCode, stream.GetContent().c_str());
                        break;
                    }
                }
                else if (responseCode < 600)
                {
                    RequestType::error.type = Error::TYPE_SERVICE_ERROR;
                    RequestType::error.message = AZStd::string::format("Service error (%i). %s", responseCode, stream.GetContent().c_str());
                }
                else
                {
                    // Anything above 599 isn't valid HTTP.
                    RequestType::error.type = Error::TYPE_CONTENT_ERROR;
                    RequestType::error.message = AZStd::string::format("Unexpected response code %i received. %s", responseCode, stream.GetContent().c_str());
                }

            }
        }

        /// Parses a JSON object from a stream and writes the values found to a
        /// provided object. ResultObjectType should implement the following function:
        ///
        ///     void OnJsonKey(const char* key, JsonObjectHandler& handler)
        ///
        /// This function will be called for each of the object's properties. It should 
        /// call one of the Expect methods on the state object to identify the expected
        /// property type and provide a location where the property value can be stored.
        void ReadResponseObject(JsonInputStream& stream)
        {
            JsonKeyHandler objectKeyHandler = JsonReader::GetJsonKeyHandler(RequestType::result);
            JsonKeyHandler responseKeyHandler = GetResponseObjectKeyHandler(objectKeyHandler);
            bool ok = JsonReader::ReadObject(stream, responseKeyHandler, RequestType::error.message);
            if(!ok)
            {
                RequestType::error.type = Error::TYPE_CONTENT_ERROR;
            }
        }

        /// Creates the JsonKeyHandler function used by ReadResultObject to 
        /// process the response body received from the service. The 
        /// responce content is determined by the response mapping template 
        /// used to configure API Gateway. The response is expected to be a 
        /// JSON object with, at minimum, an "result" property. The "result"
        /// property's value is the JSON returned by the Service Lambda 
        /// Function. 
        ///
        /// You can add new response properties using an x-amazon-cloud-canvas-lambda-dispatch
        /// object in the swagger.json file. You can provide a handler for
        /// those properties by implementing GetResponseObjectKeyHandler. For
        /// example, it may be useful to return the API Gateway generated 
        /// request id, which can help when trying to diagnose problems.
        JsonKeyHandler GetResponseObjectKeyHandler(JsonKeyHandler resultKeyHandler)
        {

            return [resultKeyHandler](const char* key, JsonReader& reader) 
            {
                if(strcmp(key, "result") == 0) return reader.Accept(resultKeyHandler);
                return reader.Ignore();
            };

        }

        Aws::String DetermineRegionFromRequestUrl()
        {
            // This assumes that API Gateway URLs have the form:
            //
            //   https://{rest-api-id}.execute-api.{region}.amazonaws.com/{stage}/{path}
            //

            int i;

            i = m_requestUrl.find('.');
            if (i != -1)
            {
                i = m_requestUrl.find('.', i + 1);
                if (i != -1)
                {
                    int offset = i + 1;
                    i = m_requestUrl.find('.', offset);
                    if (i != -1)
                    {
                        int count = i - offset;
                        return m_requestUrl.substr(offset, count);
                    }
                }
            }

            AZ_Error(
                ServiceClientJobType::COMPONENT_DISPLAY_NAME,
                false,
                "Service request url %s does not have the expected format. Cannot determine region from the url.",
                m_requestUrl.c_str()
            );

            return "us-east-1";

        }

    public:

        using OnSuccessFunction = AZStd::function<void(ServiceRequestJob* job)>;
        using OnFailureFunction = AZStd::function<void(ServiceRequestJob* job)>;

        /// A derived class that calls lambda functions on job completion.
        class Function : public ServiceRequestJobType
        {

        public:

            // To use a different allocator, extend this class and use this macro.
            AZ_CLASS_ALLOCATOR(Function, AZ::SystemAllocator, 0);

            Function(OnSuccessFunction onSuccess, OnFailureFunction onFailure = OnFailureFunction{}, IConfig* config = GetDefaultConfig())
                : ServiceRequestJob(false, config) // No auto delete - we'll take care of it
                , m_onSuccess{onSuccess}
                , m_onFailure{onFailure}
            {
            }

        protected:

            OnSuccessFunction m_onSuccess;
            OnFailureFunction m_onFailure;

            void OnSuccess() override
            {
                AZStd::function<void()> callbackHandler = [this]()
                {
                    if (m_onSuccess)
                    {
                        m_onSuccess(this);
                    }
                    delete this;
                };
                EBUS_QUEUE_FUNCTION(AZ::TickBus, callbackHandler);
            }

            void OnFailure() override
            {
                AZStd::function<void()> callbackHandler = [this]()
                {
                    if (m_onFailure)
                    {
                        m_onFailure(this);
                    }
                    delete this;
                };
                EBUS_QUEUE_FUNCTION(AZ::TickBus, callbackHandler);
            }

            // We don't use auto delete - this allows us to make sure things get cleaned up in cases where we can't call success or failure
            void DoCleanup() override
            {
                AZStd::function<void()> callbackHandler = [this]()
                {
                    delete this;
                };
                EBUS_QUEUE_FUNCTION(AZ::TickBus, callbackHandler);
            }
        };

        template<class Allocator = AZ::SystemAllocator>
        static ServiceRequestJob* Create(OnSuccessFunction onSuccess, OnFailureFunction onFailure = OnFailureFunction{}, IConfig* config = GetDefaultConfig())
        {
             return azcreate(Function, (onSuccess, onFailure, config), Allocator);
        }

    };

} // namespace CloudGemFramework


