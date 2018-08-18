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

#include <EMotionFX/Source/AnimGraphNodeId.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/string/conversions.h>


namespace EMotionFX
{
    const AnimGraphNodeId AnimGraphNodeId::InvalidId = AnimGraphNodeId(0);

    AnimGraphNodeId AnimGraphNodeId::Create()
    {
        AZ::u64 result = AZ::Sfmt::GetInstance().Rand64();

        while (AnimGraphNodeId(result) == InvalidId)
        {
            result = AZ::Sfmt::GetInstance().Rand64();
        }

        return AnimGraphNodeId(result);
    }


    AZStd::string AnimGraphNodeId::ToString() const
    {
        return AZStd::string::format("%llu", m_id);
    }


    AnimGraphNodeId AnimGraphNodeId::FromString(const AZStd::string& text)
    {
        const AZ::u64 result = AZStd::stoull(text);

        if (result == 0 || result == ULLONG_MAX)
        {
            return AnimGraphNodeId::InvalidId;
        }

        return result;
    }
} // namespace EMotionFX