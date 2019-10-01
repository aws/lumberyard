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
#include <AzCore/Component/Component.h>
#include <AzCore/NativeUI/NativeUIRequests.h>

namespace NativeUI
{
    class SystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SystemComponent, "{CD368DEB-6D12-443E-9DDC-2025E34517D4}");

        static void Reflect(AZ::ReflectContext* context);

        AZ_DEPRECATED(void Init() override, "NativeUI Gem is deprecated. Use AZCore NativeUI Ebus instead.");
        AZ_DEPRECATED(void Activate() override, "NativeUI Gem is deprecated. Use AZCore NativeUI Ebus instead.");
        AZ_DEPRECATED(void Deactivate() override, "NativeUI Gem is deprecated. Use AZCore NativeUI Ebus instead.");
    };
}