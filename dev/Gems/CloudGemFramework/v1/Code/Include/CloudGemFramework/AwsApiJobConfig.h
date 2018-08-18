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

#include <boost/optional.hpp>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/http/Scheme.h>
#include <aws/core/Region.h>
#include <aws/core/http/HttpTypes.h>
#include <CloudGemFramework/CloudGemFrameworkBus.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

namespace AZ
{
    class JobContext;
}

namespace Aws 
{
    namespace Auth
    {
        class AWSCredentialsProvider;
    }
    
    namespace Utils
    {

        namespace RateLimits
        {
            class RateLimiterInterface;
        }

        namespace Threading
        {
            class Executor;
        }

    }

    namespace Client
    {
        class RetryStrategy;
        struct ClientConfiguration;
    }

    namespace Http
    {
        class HttpClient;
    }

}

namespace CloudGemFramework
{

    /// Provides configuration for an AwsApiJob.
    class IAwsApiJobConfig
    {

    public:
        virtual ~IAwsApiJobConfig() = default;

        virtual AZ::JobContext* GetJobContext() = 0;
        virtual void OnAfterIdentityUpdate() = 0;

    };

    /// Encapsulates all the properties that can be used to configured the 
    /// operation of AWS jobs.
    class AwsApiJobConfig
        : public virtual IAwsApiJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(AwsApiJobConfig, AZ::SystemAllocator, 0);

        using InitializerFunction = AZStd::function<void(AwsApiJobConfig& config)>;

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
        AwsApiJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : m_defaultConfig{defaultConfig}
        {
            if(initializer)
            {
                initializer(*this);
            }
        }

        /// Type used to encapsulate override values.
        template<typename T>
        using Override = boost::optional<T>;

        // TODO: document the individual configuration settings
        Override<AZ::JobContext*> jobContext;
        Override<std::shared_ptr<Aws::Auth::AWSCredentialsProvider>> credentialsProvider;
        Override<Aws::String> userAgent;
        Override<Aws::Http::Scheme> scheme;
        Override<Aws::String> region;
        Override<unsigned> maxConnections;
        Override<long> requestTimeoutMs;
        Override<long> connectTimeoutMs;
        Override<std::shared_ptr<Aws::Client::RetryStrategy>> retryStrategy;
        Override<Aws::String> endpointOverride;
        Override<Aws::String> proxyHost;
        Override<unsigned> proxyPort;
        Override<Aws::String> proxyUserName;
        Override<Aws::String> proxyPassword;
        Override<std::shared_ptr<Aws::Utils::Threading::Executor>> executor;
        Override<bool> verifySSL;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface>> writeRateLimiter;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface>> readRateLimiter;
        Override<Aws::Http::TransferLibType> httpLibOverride;
        Override<bool> followRedirects;
        Override<Aws::String> caFile;

        /// Applys settings changes made after first use.
        virtual void ApplySettings();

        // IAwsApiJobConfig implementations

        AZ::JobContext* GetJobContext() override;

        /// Clears configuration related to player identity.
        /// This does not clear everything because some configuration is set only once, such as the region from the mappings.
        void OnAfterIdentityUpdate() override;

        /// Get a ClientConfiguration object initialized using the current settings.
        /// The base settings object's overrides are applied first, then this objects 
        /// overrides are applied. By default all ClientConfiguration members will 
        /// have default values (as set by the 
        Aws::Client::ClientConfiguration GetClientConfiguration() const;

    protected:

        /// Ensures that ApplySettings has been called.
        void EnsureSettingsApplied()
        {
            if(!m_settingsApplied)
            {
                ApplySettings();
            }
        }

        /// Helper function for applying Override typed members.
        template<typename T>
        static void CheckAndSet(const Override<T>& src, T& dst) 
        {
            if (src.is_initialized())
            {
                dst = src.get();
            }
        }

        /// Call Visit for m_defaultConfig, then call visitor for this object. Is
        /// templated so that it can be used to visit derived types as 
        /// long as they define an m_defaultConfig of that type.
        template<class ConfigType>
        void Visit(AZStd::function<void(const ConfigType&)> visitor) const
        {
            if(ConfigType::m_defaultConfig)
            {
                ConfigType::m_defaultConfig->Visit(visitor);
            }
            visitor(*this);
        }

        /// Get the CredentialsProvider from this settings object, if set, or
        /// from the base settings object. By default a nullptr is returned.
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() const;

    private:

        /// The settings object with values overridden by this settings object, or
        /// nullptr if this settings object is the root.
        const AwsApiJobConfig* const m_defaultConfig;

        /// True after ApplySettings is called.
        bool m_settingsApplied{false};

        /// Set when ApplySettings is called.
        AZ::JobContext* m_jobContext{nullptr};

        std::shared_ptr<Aws::Http::HttpClient> CreateHttpClient() const;

        // Copy and assignment not allowed
        AwsApiJobConfig(const AwsApiJobConfig& base) = delete;
        AwsApiJobConfig& operator=(AwsApiJobConfig& other) = delete;

    };

    template<class ConfigType>
    class AwsApiJobConfigHolder
        : public InternalCloudGemFrameworkNotificationBus::Handler
        , public CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler
    {
    public:

        ~AwsApiJobConfigHolder()
        {
            InternalCloudGemFrameworkNotificationBus::Handler::BusDisconnect();
            CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler::BusDisconnect();
        }

        ConfigType* GetConfig(AwsApiJobConfig* defaultConfig = nullptr, typename ConfigType::InitializerFunction initializer = nullptr)
        {
            if(!m_config)
            {
                InternalCloudGemFrameworkNotificationBus::Handler::BusConnect();
                CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler::BusConnect();
                m_config.reset(aznew ConfigType(defaultConfig, initializer));
            }
            return m_config.get();
        }

    private:

        void OnCloudGemFrameworkDeactivated() override
        {
            m_config.reset();
        }

        void OnBeforeIdentityUpdate() override
        {

        }

        void OnAfterIdentityUpdate() override
        {
            if (m_config) {
                m_config->OnAfterIdentityUpdate();
            }
        }

        AZStd::unique_ptr<ConfigType> m_config;

    };

} // namespace CloudGemFramework

