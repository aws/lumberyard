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

#include <CloudGemFramework/AwsApiJobConfig.h>

namespace Aws
{

    namespace Client
    {
        struct ClientConfiguration;
    }

    namespace Http
    {
        class HttpClient;
    }

}

namespace CloudGemFramework
{

    /// Provides configuration for AWS jobs using a specific client type.
    template<class ClientType>
    class IAwsApiClientJobConfig
        : public virtual IAwsApiJobConfig
    {

    public:

        virtual std::shared_ptr<ClientType> GetClient() = 0;

    };

    #ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable: 4250 )
    // warning C4250: 'AwsApiJob::AwsApiClientJobConfig<Aws::Http::HttpClient>' : inherits 'AwsApiJob::AwsApiJobConfig::AwsApiJob::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.
    
    #endif

    /// Configuration for AWS jobs using a specific client type.
    template<class ClientType>
    class AwsApiClientJobConfig
        : public AwsApiJobConfig
        , public virtual IAwsApiClientJobConfig<ClientType>
    {
    
    public:

        AZ_CLASS_ALLOCATOR(AwsApiClientJobConfig, AZ::SystemAllocator, 0);

        using AwsApiClientJobConfigType = AwsApiClientJobConfig<ClientType>;
        using InitializerFunction = AZStd::function<void(AwsApiClientJobConfigType& config)>;

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
        AwsApiClientJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : AwsApiJobConfig{defaultConfig}
        {
            if(initializer)
            {
                initializer(*this);
            }
        }

        virtual ~AwsApiClientJobConfig() = default;

        /// Gets a client initialized used currently applied settings. If
        /// you change any settings after first use, you must call 
        /// ApplySettings before those changes will take effect.
        std::shared_ptr<ClientType> GetClient() override
        {
            EnsureSettingsApplied();
            return m_client;
        }

    protected:

        void ApplySettings() override
        {
            AwsApiJobConfig::ApplySettings();
            m_client = CreateClient();
        }

        /// Create a client configured using this object's settings. ClientType
        /// can be any of the AWS API service clients (e.g. LambdaClient, etc.).
        std::shared_ptr<ClientType> CreateClient() const
        {
            return std::make_shared<ClientType>(GetCredentialsProvider(), GetClientConfiguration());
        }

    private:

        /// Set by ApplySettings
        std::shared_ptr<ClientType> m_client;

    };

    #ifdef _MSC_VER 
    #pragma warning( pop ) // C4250
    #endif

} // namespace CloudGemFramework
