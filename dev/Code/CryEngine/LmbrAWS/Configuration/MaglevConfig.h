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

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/Region.h>

namespace LmbrAWS
{
    class MaglevConfig
    {
    public:
        MaglevConfig()
            : m_region(Aws::Region::US_EAST_1) {}
        virtual ~MaglevConfig() = default;
        inline const Aws::String& GetAnonymousIdentityPoolId() const { return m_anonymousIdentityPoolId; }
        inline void SetAnonymousIdentityPoolId(const Aws::String& value) { m_anonymousIdentityPoolId = value; }
        inline const Aws::String& GetAuthenticatedIdentityPoolId() const { return m_authenticatedIdentityPoolId; }
        inline void SetAuthenticatedIdentityPoolId(const Aws::String& value) { m_authenticatedIdentityPoolId = value; }
        inline const Aws::String& GetAwsAccountId() const { return m_awsAccountId; }
        inline void SetAwsAccountId(const Aws::String& value) { m_awsAccountId = value; }
        inline Aws::String GetDefaultRegion() const { return m_region; }
        inline void SetDefaultRegion(const Aws::String& value) { m_region = value; }
        inline const Aws::Vector<Aws::String>& GetAuthProvidersToSupport() const { return m_authProviders; }
        inline void SetAuthProvidersToSupport(const Aws::Vector<Aws::String>& value) { m_authProviders = value; }
        virtual void SaveConfig() = 0;
        virtual void LoadConfig() = 0;

    protected:
        Aws::String m_anonymousIdentityPoolId;
        Aws::String m_authenticatedIdentityPoolId;
        Aws::String m_awsAccountId;
        Aws::String m_region;
        Aws::Vector<Aws::String> m_authProviders;
    };
}