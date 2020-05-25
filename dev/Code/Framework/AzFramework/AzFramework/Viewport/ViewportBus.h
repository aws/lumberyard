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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AzFramework
{
    class ViewportRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        
        static void Reflect(AZ::ReflectContext* context);

        virtual ~ViewportRequests() {}

        virtual AZ::Vector2 GetViewportSize() const = 0;
        virtual AZ::Vector3 UnprojectViewportToWorldDirection(const AZ::Vector2& viewportPos) = 0;
        virtual AZ::Vector2 ProjectWorldToViewportPosition(const AZ::Vector3& worldPos) = 0;
    };

    using ViewportRequestBus = AZ::EBus<ViewportRequests>;
}