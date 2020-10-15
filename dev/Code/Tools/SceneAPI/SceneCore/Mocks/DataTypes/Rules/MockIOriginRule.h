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

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <DataTypes/Rules/IOriginRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIOriginRule
                : public IOriginRule
            {
            public:
                AZ_RTTI(MockIOriginRule, "{A15190EC-6D19-4265-8661-653253B1C1DB}", IOriginRule);

                virtual ~MockIOriginRule() = default;

                MOCK_CONST_METHOD0(GetOriginNodeName, const AZStd::string&());
                MOCK_CONST_METHOD0(UseRootAsOrigin, bool());
                MOCK_CONST_METHOD0(GetRotation, const AZ::Quaternion&());
                MOCK_CONST_METHOD0(GetTranslation, const AZ::Vector3&());
                MOCK_CONST_METHOD0(GetScale, float());
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
