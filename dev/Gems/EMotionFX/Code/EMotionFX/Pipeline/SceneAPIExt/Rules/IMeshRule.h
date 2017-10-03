#pragma once

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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class IMeshRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(IMeshRule, "{299934A2-22EC-48AF-AB2B-953AFF8E0B19}", AZ::SceneAPI::DataTypes::IRule);

                ~IMeshRule() override = default;

                virtual bool GetOptimizeTriangleList() const = 0;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
