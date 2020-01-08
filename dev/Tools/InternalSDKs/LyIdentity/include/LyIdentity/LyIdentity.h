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
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
// The LyIdentity_shared library does not use AzCore, therefore the AZ_PUSH_DISABLE_WARNING macro is not available
#pragma warning(push)
#pragma warning(disable : 4251 4996)
#include <aws/core/utils/memory/stl/AWSString.h>
#pragma warning(pop)

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

