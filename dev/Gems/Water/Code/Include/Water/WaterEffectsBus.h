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
#include <AzCore/Math/Vector3.h>

namespace Water
{
    /// Requests to Cry Water Rendering Effects.
    class WaterEffectsRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ///  Generate a water ripple using Cry Effects.
        ///  @param position to generate the ripple at.
        ///  @param scale of the ripple.
        ///  @param strength of the ripple.
        virtual void GenerateWaterRipple(const AZ::Vector3& position, float scale, float strength) = 0;

    };

    using WaterEffectsRequestBus = AZ::EBus<WaterEffectsRequests>;

} // namespace Water
