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

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IEFXMotionScaleRule
                : public IRule
            {
            public:
                AZ_RTTI(IEFXMotionScaleRule, "{EFBCC10B-5406-4BD6-B259-BCCD2A24893D}", IRule);

                virtual ~IEFXMotionScaleRule() override = default;

                virtual float GetScaleFactor() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
