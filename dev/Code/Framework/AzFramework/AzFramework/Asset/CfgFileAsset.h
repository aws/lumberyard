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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace AzFramework
{
    class CfgFileAsset
    {
    public:
        AZ_TYPE_INFO(CfgFileAsset, "{117A80A5-206B-4D85-9445-33B446D94C35}")
        static const char* GetFileFilter()
        {
            return "*.cfg";
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<AzFramework::CfgFileAsset>, "{B12AE8FE-0113-4129-96C6-9CA622ECE1D2}")
}