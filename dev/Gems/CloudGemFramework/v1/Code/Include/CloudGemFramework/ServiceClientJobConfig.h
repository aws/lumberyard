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

#include <CloudGemFramework/ServiceJobConfig.h>
#include <CloudGemFramework/CloudGemFrameworkBus.h>

namespace CloudGemFramework
{

    /// Provides configuration needed by service jobs.
    class IServiceClientJobConfig
        : public virtual IServiceJobConfig
    {

    public:

        virtual const AZStd::string GetServiceUrl() = 0;

    };

    /// Encapsulates what we need to know about a service in order to 
    /// use it with a service job. Use the DEFINE_SERVICE_TRAITS macro
    /// to simplify the definitino of these types.
    ///
    /// \param ServiceTraitsType - must implement the following static
    /// functions:
    ///
    ///    const char* GetServiceName();
    ///
    template<class ServiceTraitsType>
    class ServiceTraits
    {

    public:

        static const char* ServiceName;

    };

    template<class ServiceTraitsType>
    const char* ServiceTraits<ServiceTraitsType>::ServiceName = 
        ServiceTraitsType::GetServiceName();

    #ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable: 4250 )
    // warning C4250: 'CloudGemFramework::ServiceClientJobConfig<ServiceTraitsType>' : inherits 'CloudGemFramework::AwsApiJobConfig::CloudGemFramework::AwsApiJobConfig::GetJobContext' via dominance
    // This is the expected and desired behavior. The warning is superfluous.
    // http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors
    #endif

    /// Provides service job configuration using settings properties.
    template<class ServiceTraitsType>
    class ServiceClientJobConfig
        : public ServiceJobConfig
        , public virtual IServiceClientJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(ServiceClientJobConfig, AZ::SystemAllocator, 0);

        using ServiceClientJobConfigType = ServiceClientJobConfig<ServiceTraitsType>;

        using InitializerFunction = AZStd::function<void(ServiceClientJobConfig& config)>;

        /// Initialize an AwsApiClientJobConfig object.
        ///
        /// \param DefaultConfigType - the type of the config object from which
        /// default values will be taken.
        ///
        /// \param defaultConfig - the config object that provivdes valules when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        ServiceClientJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : ServiceJobConfig{defaultConfig}
        {
            if(initializer)
            {
                initializer(*this);
            }
        }

        /// This implemenation assumes the caller will cache this value as 
        /// needed. See it's use in ServiceRequestJobConfig.
        const AZStd::string GetServiceUrl() override
        {
            AZStd::string serviceUrl;
            EBUS_EVENT_RESULT(serviceUrl, CloudGemFramework::CloudGemFrameworkRequestBus, GetServiceUrl, ServiceTraitsType::ServiceName);
            return serviceUrl;
        }
   
    };

    #ifdef _MSC_VER 
    #pragma warning( pop ) // C4250
    #endif

} // namespace CloudGemFramework


