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

#include <unordered_map>
#include <unordered_set>
#include <map>

#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/s3/S3Client.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/sns/SNSClient.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sts/STSClient.h>
#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#pragma warning(pop)

#include <LmbrAWS/IAWSClientManager.h>
#include <Configuration/MaglevConfigImpl.h>
#include <Configuration/TokenRetrievingPersistentIdentityProvider.h>
#include <Util/Async/CryAwsExecutor.h>

#include <LmbrAWS/ILmbrAWS.h>
///////////////////////////////////////// WARNING /////////////////////////////////////////
// Support for the auth_token cvar has been deprecated and disabled by default.
// Auth tokens should not be stored in console variables to avoid the possibility of them
// being included when console variables are logged or included in bug reports.
// Instead, call gEnv->lmbrAWS->GetClientManager()->Login(providerName, code).
// Uncommenting the following will re-enable the deprecated auth_token cvar.
//
// #define AUTH_TOKEN_CVAR_ENABLED
///////////////////////////////////////////////////////////////////////////////////////////

namespace Aws
{
    namespace Utils
    {
        namespace Json
        {
            class JsonValue;
        }
    }
    namespace Auth
    {
        class CognitoCachingCredentialsProvider;
        class CognitoCachingCredentialsProvider;
        class PersistentCognitoIdentityProvider;
    }
}

namespace LmbrAWS
{
    static const char* CLIENT_MGR_IMPL_TAG = "LmbrAWS::ClientManagerImpl";
    class ClientManagerImpl
        : public IClientManager
    {
    public:
        struct ParsedAuthToken
        {
            Aws::String provider;
            Aws::String code;
        };

        ClientManagerImpl()
            : m_cognitoIdentityClientAnonymous(nullptr)
            , m_cognitoIdentityClientAuthenticated(nullptr)
            , m_anonCredsProvider(nullptr)
            , m_authIdentityProvider(nullptr)
            , m_authCredsProvider(nullptr)
        {
            auto executor = Aws::MakeShared<CryAwsExecutor>(CLIENT_MGR_IMPL_TAG);
            // initialize default client settings
            m_defaultClientSettings.executor = executor;
            m_defaultClientSettings.credentialProvider = Aws::MakeShared<Aws::Auth::AnonymousAWSCredentialsProvider>(CLIENT_MGR_IMPL_TAG);

            m_defaultClientSettings.connectTimeoutMs = 30000;
            m_defaultClientSettings.requestTimeoutMs = 30000;

            CheckRootCAPaths();

            // Initialize anonymous client settings to just be a copy of the default.
            m_anonymousClientSettings = m_defaultClientSettings;

            // initialize default editor client settings
            m_editorClientSettings.credentialProvider = Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(CLIENT_MGR_IMPL_TAG);
            m_editorClientSettings.executor = executor;

            // initialize Lambda client settings overrides
            m_lambdaManager.GetDefaultClientSettingsOverrides().connectTimeoutMs = 30000;
            m_lambdaManager.GetDefaultClientSettingsOverrides().requestTimeoutMs = 30000;
        }

        virtual ~ClientManagerImpl() {}

        // Parameters

        virtual ConfigurationParameters& GetConfigurationParameters() final override
        {
            return m_parameters;
        }

        // Default Client Settings

        virtual ClientSettings& GetDefaultClientSettings() final override
        {
            return m_defaultClientSettings;
        }

        virtual ClientSettings& GetAnonymousClientSettings() final override
        {
            return m_anonymousClientSettings;
        }

        // Client Settings Overrides

        virtual ClientSettingsOverrides::Collection& GetClientSettingsOverridesCollection() final override
        {
            return m_clientSettingsOverrides;
        }

        // Service Specific Managers

        virtual DynamoDB::Manager& GetDynamoDBManager() final override
        {
            return m_dynamoDBManager;
        }

        virtual CognitoIdentity::Manager& GetCognitoIdentityManager() final override
        {
            return m_CognitoIdentityManager;
        }

        virtual CognitoIdentityProvider::Manager& GetCognitoIdentityProviderManager() final override
        {
            return m_cognitoIdentityProviderManager;
        }

        virtual S3::Manager& GetS3Manager() final override
        {
            return m_S3Manager;
        }

        virtual Lambda::Manager& GetLambdaManager() final override
        {
            return m_lambdaManager;
        }

        virtual SNS::Manager& GetSNSManager() final override
        {
            return m_SNSManager;
        }

        virtual SQS::Manager& GetSQSManager() final override
        {
            return m_SQSManager;
        }

        virtual STS::Manager& GetSTSManager() final override
        {
            return m_STSManager;
        }

        virtual void AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy) override;

        virtual void RemoveTokenRetrievalStrategy(const char* provider) override;

        virtual bool Login(const char* authProvider, const char* authCode, const char* refreshToken = nullptr, long long tokenExpiration = 0) final override;

        virtual void Logout() override;

        // Apply Configuration

        virtual bool ApplyConfiguration() final override
        {
            EBUS_EVENT(ClientManagerNotificationBus, OnBeforeConfigurationChange);

            bool retVal = true;

            retVal &= ResetPlayerIdentity();
            retVal &= m_cognitoIdentityProviderManager.ApplyConfiguration();
            retVal &= m_dynamoDBManager.ApplyConfiguration();
            retVal &= m_S3Manager.ApplyConfiguration();
            retVal &= m_lambdaManager.ApplyConfiguration();
            retVal &= m_SNSManager.ApplyConfiguration();
            retVal &= m_SQSManager.ApplyConfiguration();
            retVal &= m_STSManager.ApplyConfiguration();
            
            EBUS_EVENT(ClientManagerNotificationBus, OnAfterConfigurationChange);

            return retVal;
        }

        // Editor Clients

        virtual ClientSettings& GetEditorClientSettings() final override
        {
            return m_editorClientSettings;
        }

        virtual void ApplyEditorConfiguration() final override
        {
            if (gEnv->IsEditorGameMode())
            {
                ResetPlayerIdentity();
            }

            m_cognitoIdentityProviderManager.ApplyEditorConfiguration();
            m_dynamoDBManager.ApplyEditorConfiguration();
            m_S3Manager.ApplyEditorConfiguration();
            m_lambdaManager.ApplyEditorConfiguration();
            m_SNSManager.ApplyEditorConfiguration();
            m_SQSManager.ApplyEditorConfiguration();
            m_STSManager.ApplyEditorConfiguration();
        }

        void CheckRootCAPaths()
        {
            LmbrAWS::RequestRootCAFileResult requestResult;
            AZStd::string rootCA;
            EBUS_EVENT_RESULT(requestResult, AwsApiInitRequestBus, LmbrRequestRootCAFile, rootCA);
            if (rootCA.length())
            {
                AZ_TracePrintf("CloudCanvas", "Setting rootCA paths to %s with request result %d", rootCA.c_str(), requestResult);
                m_defaultClientSettings.caFile = rootCA.c_str();
                m_anonymousClientSettings.caFile = rootCA.c_str();
            }
        }
        virtual bool LoadLogicalMappingsFromFile(const string& mappingsFileName) override;

        virtual bool LoadLogicalMappingsFromJson(const Aws::Utils::Json::JsonValue& mappingsJsonData) override;

        virtual void SetLogicalMapping(const string& type, const string& logicalName, const string& physicalName) override;

        virtual void SetLogicalUserPoolClientMapping(const string& logicalName, const string& clientName, const string& clientId, const string& clientSecret) override;

        virtual bool IsProtectedMapping() override { return m_isProtectedMapping; }

        virtual void  SetProtectedMapping(bool isProtected) override { m_isProtectedMapping = isProtected; }

        virtual bool IgnoreProtection() override { return m_ignoreProtection; }
        virtual void SetIgnoreProtection(bool ignore) override { m_ignoreProtection = ignore; }

        virtual bool GetRefreshTokenForProvider(string& refreshToken, const string& provider) override;

#ifdef AUTH_TOKEN_CVAR_ENABLED // See security warning at the top of this file before enabling the auth_token cvar.
        static bool ParseAuthTokensFromConsoleVariable(const char* cVarValue, ParsedAuthToken& parsedAuthToken);
#endif // AUTH_TOKEN_CVAR_ENABLED

    private:
        Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > InitializeTokenRetrievalStrategies(const std::shared_ptr<Aws::Lambda::LambdaClient>& client, const char* lambdaName);
        bool ResetPlayerIdentity();

        class ConfigurationParamtersImpl
            : public ConfigurationParameters
        {
        public:

            virtual ParameterNames GetNames() final override
            {
                ParameterNames result;
                result.reserve(m_map.size());
                for (auto pair : m_map)
                {
                    result.push_back(pair.first);
                }
                return result;
            }

            virtual void SetParameter(const ParameterName& name, ParameterValue value) final override
            {
                m_map[name] = std::move(value);
            }

            virtual const ParameterValue& GetParameter(const ParameterName& name) final override
            {
                auto it = m_map.find(name);
                if (it == m_map.end())
                {
                    static const ParameterValue empty;
                    return empty;
                }
                else
                {
                    return it->second;
                }
            }

            virtual void ClearParameter(const ParameterName& name) final override
            {
                m_map.erase(name);
            }

            virtual void ExpandParameters(ParameterValue& value) final override
            {
                std::unordered_set<ParameterName> seen = {};
                ExpandParameters(value, seen);
            }


        private:

            void ExpandParameters(ParameterValue& value, std::unordered_set<ParameterName>& seen)
            {
                int start, end, pos = 0;
                while ((start = value.find_first_of('$', pos)) != -1)
                {
                    end = value.find_first_of('$', start + 1);
                    if (end == -1)
                    {
                        // unmatched $ at end of string, leave it there
                        pos = value.size();
                    }
                    else if (end == start + 1)
                    {
                        // $$, remove the second $
                        value.replace(start + 1, 1, "");
                        pos = start + 1;
                    }
                    else
                    {
                        // $foo$
                        ParameterName name = value.substr(start + 1, (end - start) - 1);
                        if (seen.count(name) == 0) // protect against recursive parameter values
                        {
                            ParameterValue temp = GetParameter(name);
                            seen.insert(name);
                            ExpandParameters(temp, seen);
                            seen.erase(name);
                            value.replace(start, (end - start) + 1, temp);
                            pos = start + temp.length();
                        }
                        else
                        {
                            pos = end + 1;
                        }
                    }
                }
            }

            static bool contains(std::vector<ParameterName>& seen, ParameterName& name)
            {
                for (auto& seenName : seen)
                {
                    if (seenName == name)
                    {
                        return true;
                    }
                }
                return false;
            }

            std::map<ParameterName, ParameterValue> m_map;
        };

        template<typename TResourceSettings>
        static const ResourceName& GetResourceNameFromSettings(const TResourceSettings& settings);

        template<typename TResourceSettings>
        static void InitializeSettings(const SettingsName& settingsName, TResourceSettings& settings)
        {
        }

        template<typename TSettings>
        class SettingsCollectionImpl
            : public TSettings::Collection
        {
        public:

            virtual SettingsNames GetNames() final override
            {
                SettingsNames result;
                result.reserve(m_map.size());
                for (auto pair : m_map)
                {
                    result.push_back(pair.first);
                }
                return result;
            }

            virtual TSettings& GetSettings(const SettingsName& name) final override
            {
                auto it = m_map.find(name);
                if (it == m_map.end())
                {
                    auto pair = m_map.insert({name, TSettings {}
                            });
                    TSettings& settings = pair.first->second;
                    InitializeSettings(name, settings);
                    return settings;
                }
                else
                {
                    return it->second;
                }
            }

            virtual void RemoveSettings(const SettingsName& name) final override
            {
                m_map.erase(name);
            }

        private:

            std::map<SettingsName, TSettings> m_map;
        };

        template<typename TClient, typename TServiceManager>
        class ManagedClientProviderImplBase
            : public virtual ManagedClientProvider<TClient>
        {
        public:

            ManagedClientProviderImplBase(IClientManager& clientManager, TServiceManager& serviceManager, const SettingsName& settingsName)
                : m_clientManager{clientManager}
                , m_serviceManager{serviceManager}
                , m_settingsName(settingsName)
            {
            }

            virtual ~ManagedClientProviderImplBase()
            {
            }

            virtual const std::shared_ptr<TClient>& GetClient() const final override
            {
                return m_client;
            }

            virtual bool IsReady() const final override
            {
                return (bool)m_client; // ready if client != nullptr
            }

            virtual bool Configure(bool shouldForceAnonymous = false) = 0;

        protected:

            void ApplyClientSettingsOverrides(ClientSettings& settings)
            {
                ApplyClientSettingsOverrides(m_serviceManager.GetDefaultClientSettingsOverrides(), settings);

                if (!m_settingsName.empty())
                {
                    const ClientSettingsOverrides& overrides = m_clientManager.GetClientSettingsOverridesCollection().GetSettings(m_settingsName);
                    ApplyClientSettingsOverrides(overrides, settings);
                }
            }

            void ApplyClientSettingsOverrides(const ClientSettingsOverrides& overrides, ClientSettings& settings)
            {
                for (ParameterValue baseOverridesName : overrides.baseOverridesNames)
                {
                    m_clientManager.GetConfigurationParameters().ExpandParameters(baseOverridesName);
                    const ClientSettingsOverrides& baseOverrides = m_clientManager.GetClientSettingsOverridesCollection().GetSettings(baseOverridesName);
                    ApplyClientSettingsOverrides(baseOverrides, settings);
                }

                CheckAndSet(overrides.userAgent, settings.userAgent);
                CheckAndSet(overrides.scheme, settings.scheme);
                CheckAndSet(overrides.region, settings.region);
                CheckAndSet(overrides.maxConnections, settings.maxConnections);
                CheckAndSet(overrides.requestTimeoutMs, settings.requestTimeoutMs);
                CheckAndSet(overrides.connectTimeoutMs, settings.connectTimeoutMs);
                CheckAndSet(overrides.retryStrategy, settings.retryStrategy);
                CheckAndSet(overrides.endpointOverride, settings.endpointOverride);
                CheckAndSet(overrides.proxyHost, settings.proxyHost);
                CheckAndSet(overrides.proxyPort, settings.proxyPort);
                CheckAndSet(overrides.proxyUserName, settings.proxyUserName);
                CheckAndSet(overrides.proxyPassword, settings.proxyPassword);
                CheckAndSet(overrides.executor, settings.executor);
                CheckAndSet(overrides.verifySSL, settings.verifySSL);
                CheckAndSet(overrides.writeRateLimiter, settings.writeRateLimiter);
                CheckAndSet(overrides.readRateLimiter, settings.readRateLimiter);
                CheckAndSet(overrides.httpLibOverride, settings.httpLibOverride);
                CheckAndSet(overrides.followRedirects, settings.followRedirects);
            }

            void ConfigureClient(const ClientSettings& clientSettings)
            {
                m_client = Aws::MakeShared<TClient>(CLIENT_MGR_IMPL_TAG, clientSettings.credentialProvider, clientSettings);
            }

            template<typename T>
            static void CheckAndSet(const Override<T>& src, T& dst)
            {
                if (src.is_initialized())
                {
                    dst = src.get();
                }
            }

            void NotifyConfigurationChanged()
            {

            }

            IClientManager& m_clientManager;
            TServiceManager& m_serviceManager;
            const SettingsName m_settingsName;
            std::shared_ptr<TClient> m_client;
        };

        template<typename TClient, typename TServiceManager>
        class ManagedClientProviderImpl
            : public ManagedClientProviderImplBase<TClient, TServiceManager>
        {
            using TBase = ManagedClientProviderImplBase<TClient, TServiceManager>;

        public:

            ManagedClientProviderImpl(IClientManager& clientManager, TServiceManager& serviceManager, const SettingsName& settingsName)
                : TBase(clientManager, serviceManager, settingsName)
            {
            }

            virtual bool Configure(bool shouldForceAnonymous = false) override
            {
                ClientSettings clientSettings = shouldForceAnonymous ? TBase::m_clientManager.GetAnonymousClientSettings() 
                                                                     : TBase::m_clientManager.GetDefaultClientSettings(); // makes a copy
                TBase::ApplyClientSettingsOverrides(clientSettings);
                TBase::ConfigureClient(clientSettings);
                TBase::NotifyConfigurationChanged();
                return true;
            }
        };

        #pragma warning( push )
        #pragma warning( disable: 4250 )
        // warning C4250: 'ClientManagerImpl::EditorClientProviderImpl<...>' : inherits '...GetClient' via dominance
        // This is the expected and desired behavior and the warning is superfluous.
        // http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors

        template<typename TClient, typename TServiceManager>
        class EditorClientProviderImpl
            : public ManagedClientProviderImplBase<TClient, TServiceManager>
            , public EditorClientProvider<TClient>
        {
            using TBase = ManagedClientProviderImplBase<TClient, TServiceManager>;

        public:

            EditorClientProviderImpl(IClientManager& clientManager, TServiceManager& serviceManager, const SettingsName& settingsName, std::function<void(void*)> onDelete)
                : TBase(clientManager, serviceManager, settingsName)
                , m_onDelete{onDelete}
            {
            }

            virtual ~EditorClientProviderImpl()
            {
                m_onDelete(this);
            }

            virtual const Override<Aws::String>& GetRegionOverride() const final override
            {
                return m_region;
            }

            virtual void SetRegionOverride(Override<Aws::String> region) final override
            {
                m_region = region;
                Configure();
            }

            virtual bool Configure(bool /*shouldForceAnonymous*/ = false) override
            {
                ClientSettings clientSettings = TBase::m_clientManager.GetEditorClientSettings(); // makes a copy
                TBase::ApplyClientSettingsOverrides(clientSettings);
                if (m_region)
                {
                    clientSettings.region = m_region.get();
                }
                TBase::ConfigureClient(clientSettings);
                TBase::NotifyConfigurationChanged();
                return true;
            }

        protected:

            Override<Aws::String> m_region;
            std::function<void(void*)> m_onDelete;
        };

        template<typename TClient, typename TResourceManager, typename TResourceSettings>
        class ResourceClientProviderImpl
            : public ManagedClientProviderImplBase<TClient, TResourceManager>
            , public ResourceClientProvider<TClient>
        {
            using TBase = ManagedClientProviderImplBase<TClient, TResourceManager>;

        public:

            ResourceClientProviderImpl(IClientManager& clientManager, TResourceManager& resourceManager, const SettingsName& settingsName)
                : TBase(clientManager, resourceManager, settingsName)
            {
            }

            virtual const ResourceName& GetResourceName() const final override
            {
                return m_resourceName;
            }

            virtual bool Configure(bool shouldForceAnonymous = false) override
            {
                bool retVal = true;

                TResourceSettings& resourceSettings = TBase::m_serviceManager.GetResourceClientSettingsCollection().GetSettings(TBase::m_settingsName);

                ClientSettings clientSettings = shouldForceAnonymous ? TBase::m_clientManager.GetAnonymousClientSettings()
                                                                     : TBase::m_clientManager.GetDefaultClientSettings(); // makes a copy
                TBase::ApplyClientSettingsOverrides(TBase::m_serviceManager.GetDefaultClientSettingsOverrides(), clientSettings);
                TBase::ApplyClientSettingsOverrides(resourceSettings.clientSettingsOverrides, clientSettings);
                TBase::ConfigureClient(clientSettings);

                ParameterValue resourceName = GetResourceNameFromSettings(resourceSettings);
                TBase::m_clientManager.GetConfigurationParameters().ExpandParameters(resourceName);
                m_resourceName = resourceName;

                if(m_resourceName == TBase::m_settingsName)
                {
                    // Does it look like a "ResourceGroup.Resource" name?
                    int firstDot = m_resourceName.find('.');
                    int lastDot = m_resourceName.rfind('.');
                    if (firstDot == lastDot && firstDot != -1)
                    {
                        retVal = false;
                        gEnv->pLog->LogError("(Cloud Canvas) Using unmapped AWS resource name \"%s\". Verify the player has access to the resource and that the resource mappings are up to date.", m_resourceName.c_str());
                    }
                }

                TBase::NotifyConfigurationChanged();
                return retVal;
            }

        protected:

            ResourceName m_resourceName;
        };

        #pragma warning( pop ) // disable 4250

        template<typename TClient>
        class ServiceManagerImpl
            : public virtual ServiceManager<TClient>
        {
            using TServiceManager = ServiceManagerImpl<TClient>;
            using TManagedClientProvider = ManagedClientProviderImpl<TClient, TServiceManager>;
            using TEditorClientProvider = EditorClientProviderImpl<TClient, TServiceManager>;

        public:

            ServiceManagerImpl(IClientManager& clientManager)
                : m_clientManager{clientManager}
            {
            }

            virtual ClientSettingsOverrides& GetDefaultClientSettingsOverrides() final override
            {
                return m_defaultClientSettingsOverrides;
            }

            virtual ManagedClient<TClient> CreateManagedClient() final override
            {
                static const SettingsName s_emptySettingsName;
                return CreateManagedClient(s_emptySettingsName);
            }

            virtual ManagedClient<TClient> CreateManagedClient(const SettingsName& settingsName) final override
            {
                // Game clients reuse the same provider.
                std::shared_ptr<TManagedClientProvider> provider;
                auto it = m_managedClientProviders.find(settingsName);
                if (it == m_managedClientProviders.end())
                {
                    provider = Aws::MakeShared<TManagedClientProvider>(CLIENT_MGR_IMPL_TAG, m_clientManager, *this, settingsName);
                    m_managedClientProviders.insert({ settingsName, provider });
                    provider->Configure();
                }
                else
                {
                    provider = it->second;
                }
                return ManagedClient<TClient>(provider);
            }

            virtual EditorClient<TClient> CreateEditorClient() final override
            {
                static const SettingsName s_emptySettingsName;
                return CreateEditorClient(s_emptySettingsName);
            }

            virtual EditorClient<TClient> CreateEditorClient(const SettingsName& settingsName) final override
            {
                // Editor clients each get their own provider because it stores the
                // current region, which should apply to all copies of the client.
                std::shared_ptr<TEditorClientProvider> provider = Aws::MakeShared<TEditorClientProvider>(CLIENT_MGR_IMPL_TAG,
                        m_clientManager, *this, settingsName,
                        [this, settingsName](void* provider){ RemoveEditorClientProvider(settingsName, provider); });
                m_editorClientProviders.insert({ settingsName, provider });

                provider->Configure();

                return EditorClient<TClient>{
                           provider
                };
            }

            // ConnectionManagerImpl extends this to configure its connections.
            virtual bool ApplyConfiguration()
            {
                if (!m_clientManager.IgnoreProtection() && m_clientManager.IsProtectedMapping())
                {
                    return false;
                }

                bool retVal = true; 

                for (auto pair : m_managedClientProviders)
                {
                    std::shared_ptr<TManagedClientProvider> provider = pair.second;
                    retVal &= provider->Configure();
                }
                return retVal;
            }

            // ConnectionManagerImpl extends this to configure its connections.
            virtual void ApplyEditorConfiguration()
            {
                for (auto pair : m_editorClientProviders)
                {
                    std::shared_ptr<TEditorClientProvider> provider = pair.second.lock();
                    if (provider)
                    {
                        provider->Configure();
                    }
                }
            }

        protected:

            IClientManager& m_clientManager;

            void RemoveEditorClientProvider(const SettingsName& settingsName, void* provider)
            {
                auto range = m_editorClientProviders.equal_range(settingsName);
                auto it = range.first;
                while (it != range.second)
                {
                    std::shared_ptr<TEditorClientProvider> shared = it->second.lock();
                    if (shared.get() == provider)
                    {
                        m_editorClientProviders.erase(it);
                        break;
                    }
                    ++it;
                }
            }

            ClientSettingsOverrides m_defaultClientSettingsOverrides;

            // Game clients have a one provider per settings name. The provider is resued
            // for every client. Once created, we keep the providers around indefinatly,
            // even when no client is using it. This optimization is based on the assumption
            // that a game won't request clients for many hundreds of different settings and
            // is likely use any clients it creates througout the game's execution.
            std::unordered_map<SettingsName, std::shared_ptr<TManagedClientProvider> > m_managedClientProviders;

            // Editor clients have one provider per client because the provider has state
            // specific to the client. We remove providers from the map when they are no
            // longer being used by any client.
            std::unordered_multimap<SettingsName, std::weak_ptr<TEditorClientProvider> > m_editorClientProviders;
        };

        #pragma warning( push )
        #pragma warning( disable: 4250 )
        // warning C4250: 'ClientManagerImpl::ServiceResourceManagerImpl<...>' : inherits '...CreateManagedClient' via dominance
        // This is the expected and desired behavior and the warning is superfluous.
        // http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors

        template<typename TClient, typename TResourceClient, typename TResourceClientSettings>
        class ServiceResourceManagerImpl
            : public ServiceManagerImpl<TClient>
        {
            using TBase = ServiceManagerImpl<TClient>;
            using TResourceManager = ServiceResourceManagerImpl<TClient, TResourceClient, TResourceClientSettings>;
            using TResourceClientProvider = ResourceClientProviderImpl<TClient, TResourceManager, TResourceClientSettings>;

        public:

            ServiceResourceManagerImpl(IClientManager& clientManager)
                : ServiceManagerImpl<TClient>(clientManager)
            {
            }

            typename TResourceClientSettings::Collection& GetResourceClientSettingsCollection()
            {
                return m_resourceClientSettings;
            }

            TResourceClient CreateResourceClient(const SettingsName& settingsName, bool shouldForceAnonymous = false)
            {
                // Game clients reuse the same provider.
                std::shared_ptr<TResourceClientProvider> provider;
                auto it = m_resourceClientProviders.find(settingsName);
                if (it == m_resourceClientProviders.end())
                {
                    provider = Aws::MakeShared<TResourceClientProvider>(CLIENT_MGR_IMPL_TAG, TBase::m_clientManager, *this, settingsName);
                    m_resourceClientProviders.insert({ settingsName, provider });
                    provider->Configure(shouldForceAnonymous);
                }
                else
                {
                    provider = it->second;
                }
                return TResourceClient(provider);
            }

            virtual bool ApplyConfiguration() override
            {
                bool retVal = ServiceManagerImpl<TClient>::ApplyConfiguration();

                // All client providers are now stale and should be recreated the next time they're needed.
                // Otherwise, stale identity IDs may be used.
                m_resourceClientProviders.clear();

                return retVal;
            }

        protected:

            SettingsCollectionImpl<TResourceClientSettings> m_resourceClientSettings;

            // Current implemenation keeps the resource objects around indefinatly.
            // They are used for each request for a given settings name. We don't
            // expect a game to request many different resources over a given run
            // so this shouldn't be a problem. If it does become a problem, we'll
            // need to change this map to keep a weak_ptr to the resource and
            // have the resource's remove themselves from the map when they are
            // no longer referenced.
            std::unordered_map<SettingsName, std::shared_ptr<TResourceClientProvider> > m_resourceClientProviders;
        };

        class ManagerImpl_DynamoDB
            : public DynamoDB::Manager
            , public ServiceResourceManagerImpl<DynamoDB::UnmanagedClient, DynamoDB::TableClient, DynamoDB::TableClientSettings>
        {
        public:

            ManagerImpl_DynamoDB(IClientManager& clientManager)
                : ServiceResourceManagerImpl<DynamoDB::UnmanagedClient, DynamoDB::TableClient, DynamoDB::TableClientSettings>(clientManager)
            {
            }

            virtual DynamoDB::TableClientSettings::Collection& GetTableClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            DynamoDB::TableClient CreateTableClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_CognitoIdentity
            : public CognitoIdentity::Manager
            , public ServiceResourceManagerImpl<CognitoIdentity::UnmanagedClient, CognitoIdentity::IdentityPoolClient, CognitoIdentity::IdentityPoolClientSettings>
        {
        public:

            ManagerImpl_CognitoIdentity(IClientManager& clientManager)
                : ServiceResourceManagerImpl<CognitoIdentity::UnmanagedClient, CognitoIdentity::IdentityPoolClient, CognitoIdentity::IdentityPoolClientSettings>(clientManager)
            {
            }

            virtual CognitoIdentity::IdentityPoolClientSettings::Collection& GetIdentityPoolClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            CognitoIdentity::IdentityPoolClient CreateIdentityPoolClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_CognitoIdentityProvider
            : public CognitoIdentityProvider::Manager
            , public ServiceResourceManagerImpl<CognitoIdentityProvider::UnmanagedClient, CognitoIdentityProvider::IdentityProviderClient, CognitoIdentityProvider::IdentityProviderClientSettings>
        {
        public:

            ManagerImpl_CognitoIdentityProvider(IClientManager& clientManager)
                : ServiceResourceManagerImpl<CognitoIdentityProvider::UnmanagedClient, CognitoIdentityProvider::IdentityProviderClient, CognitoIdentityProvider::IdentityProviderClientSettings>(clientManager)
            {
            }

            virtual CognitoIdentityProvider::IdentityProviderClientSettings::Collection& GetIdentityProviderClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            CognitoIdentityProvider::IdentityProviderClient CreateIdentityProviderClient(const SettingsName& settingsName)
            {
                // Always use anonymous credentials when doing Cognito IDP operations 
                // to avoid things like using expired auth credentials to renew expired auth credentials
                return CreateResourceClient(settingsName, true);
            }
        };

        class ManagerImpl_S3
            : public S3::Manager
            , public ServiceResourceManagerImpl<S3::UnmanagedClient, S3::BucketClient, S3::BucketClientSettings>
        {
        public:

            ManagerImpl_S3(IClientManager& clientManager)
                : ServiceResourceManagerImpl<S3::UnmanagedClient, S3::BucketClient, S3::BucketClientSettings>(clientManager)
            {
            }

            virtual S3::BucketClientSettings::Collection& GetBucketClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            S3::BucketClient CreateBucketClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_Lambda
            : public Lambda::Manager
            , public ServiceResourceManagerImpl<Lambda::UnmanagedClient, Lambda::FunctionClient, Lambda::FunctionClientSettings>
        {
        public:

            ManagerImpl_Lambda(IClientManager& clientManager)
                : ServiceResourceManagerImpl<Lambda::UnmanagedClient, Lambda::FunctionClient, Lambda::FunctionClientSettings>(clientManager)
            {
            }

            virtual Lambda::FunctionClientSettings::Collection& GetFunctionClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            Lambda::FunctionClient CreateFunctionClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_SNS
            : public SNS::Manager
            , public ServiceResourceManagerImpl<SNS::UnmanagedClient, SNS::TopicClient, SNS::TopicClientSettings>
        {
        public:

            ManagerImpl_SNS(IClientManager& clientManager)
                : ServiceResourceManagerImpl<SNS::UnmanagedClient, SNS::TopicClient, SNS::TopicClientSettings>(clientManager)
            {
            }

            virtual SNS::TopicClientSettings::Collection& GetTopicClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            SNS::TopicClient CreateTopicClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_SQS
            : public SQS::Manager
            , public ServiceResourceManagerImpl<SQS::UnmanagedClient, SQS::QueueClient, SQS::QueueClientSettings>
        {
        public:

            ManagerImpl_SQS(IClientManager& clientManager)
                : ServiceResourceManagerImpl<SQS::UnmanagedClient, SQS::QueueClient, SQS::QueueClientSettings>(clientManager)
            {
            }

            virtual SQS::QueueClientSettings::Collection& GetQueueClientSettingsCollection() final override
            {
                return GetResourceClientSettingsCollection();
            }

            SQS::QueueClient CreateQueueClient(const SettingsName& settingsName)
            {
                return CreateResourceClient(settingsName);
            }
        };

        class ManagerImpl_STS
            : public STS::Manager
            , public ServiceManagerImpl<STS::UnmanagedClient>
        {
        public:

            ManagerImpl_STS(IClientManager& clientManager)
                : ServiceManagerImpl<STS::UnmanagedClient>(clientManager)
            {
            }
        };

        #pragma warning( pop ) // disable: 4250

        ClientSettings m_defaultClientSettings;     // May be anonymous or authenticated
        ClientSettings m_anonymousClientSettings;   // Always anonymous
        ClientSettings m_editorClientSettings;
        ConfigurationParamtersImpl m_parameters;
        SettingsCollectionImpl<ClientSettingsOverrides> m_clientSettingsOverrides;

        ManagerImpl_DynamoDB m_dynamoDBManager {
            *this
        };
        ManagerImpl_S3 m_S3Manager {
            *this
        };
        ManagerImpl_Lambda m_lambdaManager {
            *this
        };
        ManagerImpl_SNS m_SNSManager {
            *this
        };
        ManagerImpl_SQS m_SQSManager {
            *this
        };
        ManagerImpl_STS m_STSManager {
            *this
        };
        ManagerImpl_CognitoIdentity m_CognitoIdentityManager {
            *this
        };
        ManagerImpl_CognitoIdentityProvider m_cognitoIdentityProviderManager{
            *this
        };

        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> m_cognitoIdentityClientAnonymous;
        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> m_cognitoIdentityClientAuthenticated;

        std::shared_ptr<Aws::Auth::CognitoCachingCredentialsProvider> m_anonCredsProvider;
        std::shared_ptr<Aws::Auth::CognitoCachingCredentialsProvider> m_authCredsProvider;
        std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider> m_authIdentityProvider;

        Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > m_additionalStrategyMappings;
    
        bool m_isProtectedMapping;
        bool m_ignoreProtection = true;
    };

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<DynamoDB::TableClientSettings>(const DynamoDB::TableClientSettings& settings)
    {
        return settings.tableName;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<CognitoIdentity::IdentityPoolClientSettings>(const CognitoIdentity::IdentityPoolClientSettings& settings)
    {
        return settings.identityPoolId;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<CognitoIdentityProvider::IdentityProviderClientSettings>(const CognitoIdentityProvider::IdentityProviderClientSettings& settings)
    {
        return settings.userPoolId;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<S3::BucketClientSettings>(const S3::BucketClientSettings& settings)
    {
        return settings.bucketName;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<Lambda::FunctionClientSettings>(const Lambda::FunctionClientSettings& settings)
    {
        return settings.functionName;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<SNS::TopicClientSettings>(const SNS::TopicClientSettings& settings)
    {
        return settings.topicARN;
    }

    template<>
    inline const ResourceName& ClientManagerImpl::GetResourceNameFromSettings<SQS::QueueClientSettings>(const SQS::QueueClientSettings& settings)
    {
        return settings.queueName;
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<DynamoDB::TableClientSettings>(const SettingsName& settingsName, DynamoDB::TableClientSettings& settings)
    {
        settings.tableName = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<CognitoIdentity::IdentityPoolClientSettings>(const SettingsName& settingsName, CognitoIdentity::IdentityPoolClientSettings& settings)
    {
        settings.identityPoolId = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<CognitoIdentityProvider::IdentityProviderClientSettings>(const SettingsName& settingsName, CognitoIdentityProvider::IdentityProviderClientSettings& settings)
    {
        settings.userPoolId = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<S3::BucketClientSettings>(const SettingsName& settingsName, S3::BucketClientSettings& settings)
    {
        settings.bucketName = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<Lambda::FunctionClientSettings>(const SettingsName& settingsName, Lambda::FunctionClientSettings& settings)
    {
        settings.functionName = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<SNS::TopicClientSettings>(const SettingsName& settingsName, SNS::TopicClientSettings& settings)
    {
        settings.topicARN = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }

    template<>
    inline void ClientManagerImpl::InitializeSettings<SQS::QueueClientSettings>(const SettingsName& settingsName, SQS::QueueClientSettings& settings)
    {
        settings.queueName = settingsName;
        settings.clientSettingsOverrides.baseOverridesNames.push_back(settingsName);
    }
} // Namespace LmbrAWS
