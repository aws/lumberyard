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

#include <LyIdentity/LyIdentity_EXPORTS.h>

#include <memory>

#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
    namespace Auth
    {
        class PersistentCognitoIdentityProvider;
    } // namespace Auth
} // namespace Aws

namespace Ly
{
    namespace Identity
    {
        class LY_IDENTITY_API IdentityModel
        {
        public:
            IdentityModel(
                const Aws::String& accountId,
                const Aws::String& identityPoolId,
                const Aws::String& applicationId);

            Aws::String GetAccountId() const
            {
                return m_accountId;
            }

            Aws::String GetIdentityPoolId() const
            {
                return m_identityPoolId;
            }

            Aws::String GetApplicationId() const
            {
                return m_applicationId;
            }

            static IdentityModel GetDEV();
            static IdentityModel GetPROD();
            static IdentityModel GetTEST();

        private:
            const Aws::String m_accountId;
            const Aws::String m_identityPoolId;
            const Aws::String m_applicationId;
        };

        LY_IDENTITY_API IdentityModel GetDeveloperCognitoSettings();
        LY_IDENTITY_API std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider> BuildLumberyardIdentityProvider();
    } // namespace Identity
} // namespace Ly

