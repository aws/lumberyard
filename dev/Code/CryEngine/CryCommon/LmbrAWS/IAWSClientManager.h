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

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <boost/optional.hpp>
#include <aws/core/client/ClientConfiguration.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/EBus/EBus.h>
#include <platform.h>

namespace Aws
{
    namespace Auth
    {
        class AWSCredentialsProvider;
    } // namespace Auth

    namespace DynamoDB
    {
        class DynamoDBClient;
    } // namespace DynamoDB

    namespace CognitoIdentity
    {
        class CognitoIdentityClient;
    } // namespace CognitoIdentity

    namespace CognitoIdentityProvider
    {
        class CognitoIdentityProviderClient;
    } // namespace CognitoIdentityProvider

    namespace S3
    {
        class S3Client;
    } // namespace S3

    namespace Lambda
    {
        class LambdaClient;
    } // namespace Lambda

    namespace SNS
    {
        class SNSClient;
    } // namespace SNS

    namespace SQS
    {
        class SQSClient;
    } // namespace SQS

    namespace STS
    {
        class STSClient;
    } // namespace STS
} // namespace Aws

namespace Aws
{
    namespace Utils
    {
        namespace Json
        {
            class JsonValue;
        }
    }
}

namespace LmbrAWS
{
    class TokenRetrievalStrategy;
}

namespace LmbrAWS
{
    ///////////////////////////////////////////////////////////////////////////
    // Configuration Parameters

    using ParameterName = string;
    using ParameterNames = std::vector<ParameterName>;
    using ParameterValue = string;

    // Defines the interface used for configuration parameters.
    class ConfigurationParameters
    {
    public:

        // Get the names of all parameters.
        virtual ParameterNames GetNames() = 0;

        // Sets the value of a connection settings parameter.
        //
        // May contain $param-name$ substrings as described for
        // ExpandParameters.
        virtual void SetParameter(const ParameterName& name, ParameterValue value) = 0;

        // Gets the value of a connection settings parameter. The value "" is
        // returned if the specified parameter has no value. The $param-name$
        // substrings in the value are not replaced by this method.
        virtual const ParameterValue& GetParameter(const ParameterName& name) = 0;

        // Clears the value of a connection settings parameter.
        virtual void ClearParameter(const ParameterName& name) = 0;

        // Scans the provided value for substrings of the form $param-name$
        // and replaces them with the corresponding parameter value. Any
        // $param-name$ substrings in the parameter values are expended
        // recursivly.
        virtual void ExpandParameters(ParameterValue& value) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Settings Collection

    //TODO When client manager is rewritten to not have hard coded resource types
    // please change these to AZStd::string and AZStd::vector
    using SettingsName = string;
    using SettingsNames = std::vector<SettingsName>;

    // Defines the interface used for collections of configuration data.
    template<typename TSettings>
    class SettingsCollection
    {
    public:

        // Get the names of all the settings.
        virtual SettingsNames GetNames() = 0;

        // Get the settings associated with the specified name. The settings are created if they
        // do not exist.
        virtual TSettings& GetSettings(const SettingsName& name) = 0;

        // Remove the settings associated with the specified name.
        virtual void RemoveSettings(const SettingsName& name) = 0;
    };
} // namespace LmbrAWS

namespace std {
    template <>
    struct hash<LmbrAWS::SettingsName>
    {
        std::size_t operator()(const LmbrAWS::SettingsName& k) const
        {
            // Casting from const char* to const unsigned char*. Should
            // be ok for computing a hash. _Hash_seq is what the standard
            // library uses for std::string.
#ifdef PLATFORM_WINDOWS
            return _Hash_seq(reinterpret_cast<const unsigned char*>(k.c_str()), k.length());
#else
            return std::hash<std::string>()(k.c_str());
#endif
        }
    };
}

namespace LmbrAWS
{

    ///////////////////////////////////////////////////////////////////////////
    // Client Settings

    // Encapsulates all AWS client settings.
    //
    // TODO: can ClientConfiguration be extended to include credentialProvider?
    // It isn't clear why these are two seperate parameters passed to the
    // client constructors.
    struct ClientSettings
        : Aws::Client::ClientConfiguration
    {
        ClientSettings()
        {
            userAgent += "/Cloud Canvas";
        }

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> credentialProvider;
    };

    // Type used to encapsulate override values.
    template<typename T>
    using Override = boost::optional<T>;

    // Contains overrides of ClientSettings properties.
    struct ClientSettingsOverrides
    {
        using Ptr = std::shared_ptr<ClientSettingsOverrides>;
        using Collection = SettingsCollection<ClientSettingsOverrides>;

        // Identifies a sequence of client settings overrides that will be
        // applied in the order provided, before the overrides provided by
        // this object are applied.
        //
        // The names in this sequence may contain $param-name$ substrings. These
        // will be replaced by the corresponding parameter value, and the result
        // used to lookup the named overrides.
        SettingsNames baseOverridesNames;

        Override<std::shared_ptr<Aws::Auth::AWSCredentialsProvider> > credentialProvider;
        Override<Aws::String> userAgent;
        Override<Aws::Http::Scheme> scheme;
        Override<Aws::String> region;
        Override<unsigned> maxConnections;
        Override<long> requestTimeoutMs;
        Override<long> connectTimeoutMs;
        Override<std::shared_ptr<Aws::Client::RetryStrategy> > retryStrategy;
        Override<Aws::String> endpointOverride;
        Override<Aws::String> proxyHost;
        Override<unsigned> proxyPort;
        Override<Aws::String> proxyUserName;
        Override<Aws::String> proxyPassword;
        Override<std::shared_ptr<Aws::Utils::Threading::Executor> > executor;
        Override<bool> verifySSL;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> > writeRateLimiter;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> > readRateLimiter;
        Override<Aws::Http::TransferLibType> httpLibOverride;
        Override<bool> followRedirects;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Resource Settings

    using ResourceName = string;

    // Base class used for the service specific resource settings.
    struct ResourceClientSettingsBase
    {
        // Overrides for the client settings that apply only to the resource.
        ClientSettingsOverrides clientSettingsOverrides;
    };

    // Defines the interface used by a ManagedClient to access the current
    // configured client instance.
    template<typename TClient>
    class ManagedClientProvider
    {
    public:

        using Ptr = std::shared_ptr<ManagedClientProvider>;

        virtual ~ManagedClientProvider() {}

        virtual const std::shared_ptr<TClient>& GetClient() const
        {
            static const std::shared_ptr<TClient> empty {};
            return empty;
        }

        virtual bool IsReady() const
        {
            return false;
        }

    };

    // Provides access to a configured AWS client. The client's methods can be
    // accessed via the -> operator. Objects of this type can be passed by
    // value (i.e. treat them as you would a std::shared_ptr for an unmanaged
    // AWS client).
    template<typename TClient, typename TProvider = ManagedClientProvider<TClient> >
    class ManagedClient
    {
    public:

        // Used by the ClientManager implemenation to create a ManagedClient.
        ManagedClient(typename TProvider::Ptr provider = GetEmptyProvider())
            : m_provider{provider}
        {
        }

        // Assuming default copy constructors and assignment operators. MSVC++
        // 2013 doesn't generate default move constructors or assignment operators.
        // For simplicity we can live without those for now.

        // Returns the current configured client, or an empty std::shared_ptr
        // if there is no configured client. The client returned by this method
        // will not be updated with new configuration even after calling the
        // ApplyConfiguration method.
        std::shared_ptr<TClient> GetUnmanagedClient() const
        {
            return m_provider->GetClient();
        }

        // Provides access to the currently configured client. Will result in
        // a nullptr reference if the client has not been configured yet.
        std::shared_ptr<TClient> operator->() const
        {
            return m_provider->GetClient();
        }

        // Determines if the client has been configured and is ready for use.
        const bool IsReady() const
        {
            return m_provider->IsReady();
        }


    protected:

        typename TProvider::Ptr m_provider;

        static const typename TProvider::Ptr& GetEmptyProvider()
        {
            static const typename TProvider::Ptr empty = std::make_shared<TProvider>();
            return empty;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Editor Clients

    // Defines the interface used by a EditorClient to access the current
    // configured client instance and region override.
    template<typename TClient>
    class EditorClientProvider
        : public virtual ManagedClientProvider<TClient>
    {
    public:

        using Ptr = std::shared_ptr<EditorClientProvider>;

        virtual const Override<Aws::String>& GetRegionOverride() const
        {
            static const Override<Aws::String> empty {};
            return empty;
        }

        virtual void SetRegionOverride(Override<Aws::String> region)
        {
        }
    };

    // Provides access to an AWS client for use in the editor. The client's
    // methods can be accessed via the -> operator. Objects of this type can
    // be passed by value (i.e. treat them as you would a std::shared_ptr for
    // an unmanaged AWS client).
    template<typename TClient>
    class EditorClient
        : public ManagedClient<TClient, EditorClientProvider<TClient> >
    {
        using Base = ManagedClient<TClient, EditorClientProvider<TClient> >;

    public:

        // Used by the ClientManager implemeantion when creating clients.
        EditorClient(typename EditorClientProvider<TClient>::Ptr provider = Base::GetEmptyProvider())
            : Base(provider)
        {
        }

        // Gets the current region override.
        const boost::optional<Aws::String>& GetRegionOverride() const
        {
            return Base::m_provider->GetRegionOverride();
        }

        // Sets the region override that will be used by the client.
        void SetRegionOverride(boost::optional<Aws::String> region)
        {
            Base::m_provider->SetRegionOverride(region);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Resource Clients

    // Defines the interface used by a ResourceClient to access the current
    // configured client instance and resource name.
    template<typename TClient>
    class ResourceClientProvider
        : public virtual ManagedClientProvider<TClient>
    {
    public:

        using Ptr = std::shared_ptr<ResourceClientProvider>;

        virtual const ResourceName& GetResourceName() const
        {
            static const ResourceName empty {};
            return empty;
        }
    };

    // Base class used for clients associated with a specific AWS resource.
    template<typename TClient>
    class ResourceClientBase
        : public ManagedClient<TClient, ResourceClientProvider<TClient> >
    {
        using Base = ManagedClient<TClient, ResourceClientProvider<TClient> >;

    public:

        // Used by the ClientManager implemenation when creating ResourceClient objects.
        ResourceClientBase(typename ResourceClientProvider<TClient>::Ptr provider)
            : Base(provider)
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Client and Resource Managers

    // Defines the interface for creating ManagedClient and EditorClient
    // objects.
    template<typename TClient>
    class ServiceManager
    {
    public:

        // Get the overrides that are applied to all of this manager's type of clients.
        virtual ClientSettingsOverrides& GetDefaultClientSettingsOverrides() = 0;

        // Create a client that will be configured using the default client settings.
        virtual ManagedClient<TClient> CreateManagedClient() = 0;

        // Create a client that will be configured using the default client settings
        // with specified client settings overrides applied.
        virtual ManagedClient<TClient> CreateManagedClient(const SettingsName& settingsName) = 0;

        // Create a client that will be configured using the editor client settings.
        virtual EditorClient<TClient> CreateEditorClient() = 0;

        // Create a client that will be configured using the editor client settings
        // with specified client settings overrides applied.
        virtual EditorClient<TClient> CreateEditorClient(const SettingsName& settingsName) = 0;
    };


    ///////////////////////////////////////////////////////////////////////////
    // Service Specific Definitions

    namespace DynamoDB
    {
        using UnmanagedClient = Aws::DynamoDB::DynamoDBClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class TableClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            TableClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the DynamoDB table as determined by the resource
            // settings specfied when the client was created.
            const ResourceName& GetTableName() const
            {
                return m_provider->GetResourceName();
            }
        };

        struct TableClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<TableClientSettings>;

            // Name of DynamoDB table used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName tableName;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual TableClientSettings::Collection& GetTableClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual TableClient CreateTableClient(const SettingsName& settingsName) = 0;
        };
    } // namespace DynamoDB

    namespace CognitoIdentity
    {
        using UnmanagedClient = Aws::CognitoIdentity::CognitoIdentityClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class IdentityPoolClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            IdentityPoolClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the CognitoIdentityPoolId as determined by the resource
            // settings specfied when the client was created.
            const ResourceName& GetIdentityPoolId() const
            {
                return m_provider->GetResourceName();
            }
        };

        struct IdentityPoolClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<IdentityPoolClientSettings>;

            // IdentityPoolId used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName identityPoolId;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual IdentityPoolClientSettings::Collection& GetIdentityPoolClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual IdentityPoolClient CreateIdentityPoolClient(const SettingsName& settingsName) = 0;
        };
    } // namespace CognitoIdentity

    namespace CognitoIdentityProvider
    {
        using UnmanagedClient = Aws::CognitoIdentityProvider::CognitoIdentityProviderClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class IdentityProviderClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            IdentityProviderClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the CognitoUserPoolId as determined by the resource
            // settings specfied when the client was created.
            const ResourceName& GetUserPoolId() const
            {
                return m_provider->GetResourceName();
            }
        };

        struct UserPoolApp
        {
            ResourceName clientId;
            ResourceName clientSecret;
        };

        struct IdentityProviderClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<IdentityProviderClientSettings>;

            // UserPoolId used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName userPoolId;

            // Maps Client App names to their corresponding client ID and secret
            std::unordered_map<ResourceName, UserPoolApp> clientApps;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual IdentityProviderClientSettings::Collection& GetIdentityProviderClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual IdentityProviderClient CreateIdentityProviderClient(const SettingsName& settingsName) = 0;
        };
    } // namespace CognitoIdentityProvider

    namespace S3
    {
        using UnmanagedClient = Aws::S3::S3Client;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class BucketClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            BucketClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the S3 bucket as determined by the resource
            // settings specfied when the client was created.
            virtual const ResourceName& GetBucketName()
            {
                return m_provider->GetResourceName();
            }
        };

        struct BucketClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<BucketClientSettings>;

            // Name of S3 bucket used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName bucketName;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual BucketClientSettings::Collection& GetBucketClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual BucketClient CreateBucketClient(const SettingsName& settingsName) = 0;
        };
    } // namespace S3

    namespace Lambda
    {
        using UnmanagedClient = Aws::Lambda::LambdaClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class FunctionClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            FunctionClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the Lambda function as determined by the resource
            // settings specfied when the client was created.
            virtual const ResourceName& GetFunctionName()
            {
                return m_provider->GetResourceName();
            }
        };

        struct FunctionClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<FunctionClientSettings>;

            // SettingsName of Lambda function used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName functionName;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual FunctionClientSettings::Collection& GetFunctionClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual FunctionClient CreateFunctionClient(const SettingsName& settingsName) = 0;
        };
    } // namespace Lambda

    namespace SNS
    {
        using UnmanagedClient = Aws::SNS::SNSClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class TopicClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            TopicClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // ARN of the SNS topic as determined by the resource
            // settings specfied when the client was created.
            virtual const ResourceName& GetTopicARN()
            {
                return m_provider->GetResourceName();
            }
        };

        struct TopicClientSettings
            :  ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<TopicClientSettings>;

            // ARN of SNS topic used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName topicARN;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual TopicClientSettings::Collection& GetTopicClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual TopicClient CreateTopicClient(const SettingsName& settingsName) = 0;
        };
    } // namespace SNS

    namespace SQS
    {
        using UnmanagedClient = Aws::SQS::SQSClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class QueueClient
            : public ResourceClientBase<UnmanagedClient>
        {
        public:

            QueueClient(ResourceClientProvider<UnmanagedClient>::Ptr provider = GetEmptyProvider())
                : ResourceClientBase(provider)
            {
            }

            // Name of the SQS queue as determined by the resource
            // settings specfied when the client was created.
            virtual const ResourceName& GetQueueName()
            {
                return m_provider->GetResourceName();
            }
        };

        struct QueueClientSettings
            : ResourceClientSettingsBase
        {
            using Collection = SettingsCollection<QueueClientSettings>;

            // SettingsName of SQS queue used by the connection.
            //
            // May contain $param-name$ substrings which will be replaced by the
            // specified configuration parameter's value when a connection is
            // created using these settings. Use $$ to include a literal $ in the
            // resource name.
            ResourceName queueName;
        };

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        public:

            // Collection of resource configuration settings.
            virtual QueueClientSettings::Collection& GetQueueClientSettingsCollection() = 0;

            // Create a client that will be configured using the default client settings
            // with specified resource settings applied.
            virtual QueueClient CreateQueueClient(const SettingsName& settingsName) = 0;
        };
    } // namespace SQS

    namespace STS
    {
        using UnmanagedClient = Aws::STS::STSClient;
        using ManagedClient = ::LmbrAWS::ManagedClient<UnmanagedClient>;
        using EditorClient = ::LmbrAWS::EditorClient<UnmanagedClient>;

        class Manager
            : public virtual ServiceManager<UnmanagedClient>
        {
        };
    } // namespace STS

    ///////////////////////////////////////////////////////////////////////////////

    class ClientManagerNotifications : public AZ::EBusTraits
    {
    public:
        // ------------------ EBus Configuration -------------------

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // ------------------ Handler Interface -------------------

        virtual void OnBeforeConfigurationChange() { }
        virtual void OnAfterConfigurationChange() { }
    };

    using ClientManagerNotificationBus = AZ::EBus<ClientManagerNotifications>;

    class ClientManagerNotificationBusHandler
        : public ClientManagerNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ClientManagerNotificationBusHandler, "{F7811A6E-A2B5-4D85-83EE-69A569B6D6E8}", AZ::SystemAllocator,
            OnBeforeConfigurationChange,
            OnAfterConfigurationChange);

        void OnBeforeConfigurationChange() override
        {
            Call(FN_OnBeforeConfigurationChange);
        }

        void OnAfterConfigurationChange() override
        {
            Call(FN_OnAfterConfigurationChange);
        }
    };

    // Manages the configuration and connections for accessing AWS services.
    class IClientManager
    {
    public:

        // Configuration paramters used when processing connection settings and client
        // setting overrides to produce a configured connection.
        virtual ConfigurationParameters& GetConfigurationParameters() = 0;

        // The default client settings used for all AWS connections (may be anonymous or authenticated).
        virtual ClientSettings& GetDefaultClientSettings() = 0;

        // Client settings that are always anonymous.
        virtual ClientSettings& GetAnonymousClientSettings() = 0;

        // Collection of client settings overrides. Resource settings identify these
        // by name to override the default client settings as needed for each type of
        // connection.
        virtual ClientSettingsOverrides::Collection& GetClientSettingsOverridesCollection() = 0;

        // Provides access to DynamoDB client and connection management.
        virtual DynamoDB::Manager& GetDynamoDBManager() = 0;

        // Provides access to CognitoIdentity client and connection management.
        virtual CognitoIdentity::Manager& GetCognitoIdentityManager() = 0;

        // Provides access to CognitoIdentityProvider client and connection management.
        virtual CognitoIdentityProvider::Manager& GetCognitoIdentityProviderManager() = 0;

        // Provides access to S3 client and connection management.
        virtual S3::Manager& GetS3Manager() = 0;

        // Provides access to Lambda client and connection management.
        virtual Lambda::Manager& GetLambdaManager() = 0;

        // Provides access to SNS client and connection management.
        virtual SNS::Manager& GetSNSManager() = 0;

        // Provides access to SQS client and connection management.
        virtual SQS::Manager& GetSQSManager() = 0;

        // Provides access to STS client management.
        virtual STS::Manager& GetSTSManager() = 0;

        // Adds an additional TokenRetrievalStrategy (beyond the defaults) for getting/refreshing tokens based on the provider
        // provider is the authentication provider (e.g. www.amazon.com, graph.facebook.com, accounts.google.com are defaults)
        // strategy defines how tokens will be gotten/refreshed for the given provider
        virtual void AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy) = 0;

        // Removes an additional (non-default) TokenRetrievalStrategy for getting/refreshing tokens based on the provider
        // provider is the authentication provider (e.g. www.amazon.com, graph.facebook.com, accounts.google.com are defaults)
        virtual void RemoveTokenRetrievalStrategy(const char* provider) = 0;

        // Login to the game via auth tokens using something like amazon cognito-identity
        // provider is the authentication provider e.g. www.amazon.com, graph.facebook.com, accounts.google.com etc...
        // code is a bearer token from the authentication process.
        // refreshToken is a refresh token from the authentication process. If none is available, pass in nullptr.
        // tokenExpiration is when the access token will expire
        virtual bool Login(const char* provider, const char* code, const char* refreshToken = nullptr, long long tokenExpiration = 0) = 0;

        // Log out of the authenticated identity used in the game.
        virtual void Logout() = 0;

        // Configure all clients and connections with the current settings.
        virtual bool ApplyConfiguration() = 0;

        // The default client settings for all editor clients.
        virtual ClientSettings& GetEditorClientSettings() = 0;

        // Configure all editor clients with the current settings.
        virtual void ApplyEditorConfiguration() = 0;

        virtual bool LoadLogicalMappingsFromFile(const string& mappingsFileName) = 0;

        virtual bool LoadLogicalMappingsFromJson(const Aws::Utils::Json::JsonValue& mappingsJsonData) = 0;

        virtual void SetLogicalMapping(const string& type, const string& logicalName, const string& physicalName) = 0;

        virtual void SetLogicalUserPoolClientMapping(const string& logicalName, const string& clientName, const string& clientId, const string& clientSecret) = 0;

        // If the protected flag is set in the mapping this will return true indicating
        // the user should be warned that they will be connecting to protected resources
        // (likely live customer facing) and given a chance to not connect
        virtual bool IsProtectedMapping() = 0;
        virtual void SetProtectedMapping(bool isProtected) = 0;
        virtual bool IgnoreProtection() = 0;
        virtual void SetIgnoreProtection(bool ignore) = 0;

        virtual bool GetRefreshTokenForProvider(string& refreshToken, const string& provider) = 0;

    protected:

        IClientManager() {}
        virtual ~IClientManager() {}
    };
} // namespace LmbrAWS
