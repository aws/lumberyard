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

#include <AzCore/Component/ComponentBus.h>
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>

namespace Vegetation
{
    class StaticVegetationBlockerRequests
        : public AreaConfigRequests
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetBoundingBoxPadding() const = 0;
        virtual void SetBoundingBoxPadding(float padding) = 0;

        virtual bool GetShowBoundingBox() const = 0;
        virtual void SetShowBoundingBox(bool value) = 0;

        virtual bool GetShowBlockedPoints() const = 0;
        virtual void SetShowBlockedPoints(bool value) = 0;

        virtual float GetMaxDebugDrawDistance() const = 0;
        virtual void SetMaxDebugDrawDistance(float distance) = 0;
    };

    using StaticVegetationBlockerRequestBus = AZ::EBus<StaticVegetationBlockerRequests>;
}