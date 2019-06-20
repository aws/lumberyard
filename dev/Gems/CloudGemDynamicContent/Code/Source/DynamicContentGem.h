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

#include <AzCore/Module/Module.h>

namespace DynamicContent
{
    class DynamicContentGem : public AZ::Module
    {
    public:
        AZ_RTTI(DynamicContentGem, "{A5268487-BF49-4191-BF31-0D083C9A19D6}", AZ::Module);

        DynamicContentGem();

        ~DynamicContentGem() = default;
        
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    };
} // namespace DynamicContent
