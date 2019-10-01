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

#include <AzCore/Module/Module.h>

namespace UserLoginDefault
{
    // Deprecated
    class UserLoginDefaultGem : public AZ::Module
    {
    public:
        AZ_RTTI(UserLoginDefaultGem, "{1E91EADA-7278-485B-BCB7-2ABC530B6369}", AZ::Module);
    };
} // namespace UserLoginDefault

AZ_DECLARE_MODULE_CLASS(UserLoginDefault_c9c25313197f489a8e9e8e6037fc62eb, UserLoginDefault::UserLoginDefaultGem)
