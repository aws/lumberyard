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

#include <Tests/TestAssetCode/TestMotionAssets.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>

namespace EMotionFX
{
    SkeletalMotion* TestMotionAssets::GetJackWalkForward()
    {
        const AZStd::string base64Data = "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";

        AZStd::vector<AZ::u8> data;
        AzFramework::StringFunc::Base64::Decode(data, base64Data.c_str(), base64Data.size());
        return GetImporter().LoadSkeletalMotion(data.begin(), data.size());
    }
}
