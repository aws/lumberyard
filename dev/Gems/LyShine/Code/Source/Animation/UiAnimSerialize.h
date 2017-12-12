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

#include <AzCore/Serialization/SerializeContext.h>
#include <Range.h>
#include <AnimKey.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Range, "{515CF4CF-4992-4139-BDE5-42A887432B45}");
    AZ_TYPE_INFO_SPECIALIZE(IKey, "{680BD51E-C106-4BBF-9A6F-CD551E00519F}");
    AZ_TYPE_INFO_SPECIALIZE(IBoolKey, "{DBF8044F-6E64-403D-807D-F3152F640703}");
}


namespace UiAnimSerialize
{
    //! Define Animation types for the AZ Serialize system
    void ReflectUiAnimTypes(AZ::SerializeContext* context);
}
