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
#include <StdAfx.h>
#include <Configuration/MaglevConfigImpl.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/IO/SystemFile.h> //for AZ_MAX_PATH_LEN

using namespace Aws::Utils::Json;
using namespace Aws;

namespace LmbrAWS
{

    static const char* ANONYMOUS_IDENTITY_POOL_ID = "AnonymousIdentityPoolId";
    static const char* AUTHENTICATED_IDENTITY_POOL_ID = "AuthenticatedIdentityPoolId";
    static const char* AWS_ACCOUNT_ID = "AwsAccountId";
    static const char* DEFAULT_REGION = "DefaultRegion";
    static const char* AUTH_PROVIDERS_TO_SUPPORT = "AuthProvidersToSupport";

    MaglevConfigImpl::MaglevConfigImpl(const AZStd::shared_ptr<MaglevConfigPersistenceLayer>& persistenceLayer)
        : m_persistenceLayer(persistenceLayer)
    {
    }

    void MaglevConfigImpl::SaveConfig()
    {
        std::lock_guard<std::mutex> locker(m_persistenceMutex);
        JsonValue && jsonValue = m_persistenceLayer->LoadJsonDoc();

        jsonValue.WithString(ANONYMOUS_IDENTITY_POOL_ID, m_anonymousIdentityPoolId);
        jsonValue.WithString(AUTHENTICATED_IDENTITY_POOL_ID, m_authenticatedIdentityPoolId);
        jsonValue.WithString(AWS_ACCOUNT_ID, m_awsAccountId);
        jsonValue.WithString(DEFAULT_REGION, m_region);

        Utils::Array<JsonValue> authProviders(m_authProviders.size());

        for (unsigned i = 0; i < m_authProviders.size(); ++i)
        {
            authProviders[i].AsString(m_authProviders[i]);
        }

        jsonValue.WithArray(AUTH_PROVIDERS_TO_SUPPORT, std::move(authProviders));

        m_persistenceLayer->PersistJsonDoc(jsonValue);
    }

    void MaglevConfigImpl::LoadConfig()
    {
        std::lock_guard<std::mutex> locker(m_persistenceMutex);
        JsonValue && jsonValue = m_persistenceLayer->LoadJsonDoc();

        //reset everything when we reload.
        m_anonymousIdentityPoolId = "";
        m_authenticatedIdentityPoolId = "";
        m_awsAccountId = "";
        m_region = Aws::Region::US_EAST_1;
        m_authProviders.clear();

        if (jsonValue.ValueExists(ANONYMOUS_IDENTITY_POOL_ID))
        {
            m_anonymousIdentityPoolId = jsonValue.GetString(ANONYMOUS_IDENTITY_POOL_ID);
        }

        if (jsonValue.ValueExists(AUTHENTICATED_IDENTITY_POOL_ID))
        {
            m_authenticatedIdentityPoolId = jsonValue.GetString(AUTHENTICATED_IDENTITY_POOL_ID);
        }

        if (jsonValue.ValueExists(AWS_ACCOUNT_ID))
        {
            m_awsAccountId = jsonValue.GetString(AWS_ACCOUNT_ID);
        }

        if (jsonValue.ValueExists(DEFAULT_REGION))
        {
            m_region = jsonValue.GetString(DEFAULT_REGION).c_str();
        }

        if (jsonValue.ValueExists(AUTH_PROVIDERS_TO_SUPPORT))
        {
            Utils::Array<JsonValue>&& authProvidersList = jsonValue.GetArray(AUTH_PROVIDERS_TO_SUPPORT);
            for (unsigned i = 0; i < authProvidersList.GetLength(); ++i)
            {
                m_authProviders.push_back(authProvidersList[i].AsString());
            }
        }
    }

    JsonValue MaglevConfigPersistenceLayerCryPakImpl::LoadJsonDoc() const
    {
        ICryPak* cryPak = FindCryPak();

        if (cryPak)
        {
            string filePath = GetConfigFilePath();

            char fullFilePath[AZ_MAX_PATH_LEN] = { 0 };

            strcpy_s(fullFilePath, AZ_MAX_PATH_LEN, "@assets@/");
            strcat_s(fullFilePath, AZ_MAX_PATH_LEN, filePath.c_str());

            AZ::IO::HandleType readHandle = cryPak->FOpen(fullFilePath, "rt");
            if (readHandle != AZ::IO::InvalidHandle)
            {
                size_t fileSize = cryPak->FGetSize(readHandle);
                if (fileSize > 0)
                {
                    char* fileData = static_cast<char*>(azmalloc(fileSize));
                    size_t read = cryPak->FReadRawAll(fileData, fileSize, readHandle);
                    cryPak->FClose(readHandle);
                    JsonValue jsonValue(Aws::String((const char*)fileData));
                    azfree(fileData);

                    if (jsonValue.WasParseSuccessful())
                    {
                        return jsonValue;
                    }
                }
            }
        }

        return JsonValue();
    }

    void MaglevConfigPersistenceLayerCryPakImpl::PersistJsonDoc(const JsonValue& value)
    {
        ICryPak* cryPak = FindCryPak();

        if (cryPak)
        {
            // if we're writing, we need to save to the development folder instead:
            string filePath("@devassets@/");
            filePath += GetConfigFilePath();

            AZ::IO::HandleType writeHandle = cryPak->FOpen(filePath.c_str(), "wt");
            if (writeHandle != AZ::IO::InvalidHandle)
            {
                auto jsonString = value.WriteReadable();
                cryPak->FWrite(jsonString.c_str(), jsonString.length(), writeHandle);
                cryPak->FClose(writeHandle);
            }
        }
    }

    void MaglevConfigPersistenceLayerCryPakImpl::Purge()
    {
        ICryPak* cryPak = FindCryPak();

        if (cryPak)
        {
            cryPak->RemoveFile(GetConfigFilePath().c_str());
        }
    }

    string MaglevConfigPersistenceLayerCryPakImpl::GetConfigFilePath() const
    {
        return "maglevConfig.cfg";
    }

    ICryPak* MaglevConfigPersistenceLayerCryPakImpl::FindCryPak() const
    {
        return gEnv->pCryPak;
    }
}