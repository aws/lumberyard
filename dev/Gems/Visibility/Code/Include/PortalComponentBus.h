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
#include <AzFramework/Asset/SimpleAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace Visibility
{
    // Request bus for the component
    class PortalRequests
        : public AZ::ComponentBus
    {
        public:
            virtual ~PortalRequests() {}

            /////////////////////////////////////////////////////////////////
            // Property Set/Get functions for the component.
            /////////////////////////////////////////////////////////////////
            virtual void SetHeight(const float value) = 0;
            virtual float GetHeight() = 0;
            virtual void SetDisplayFilled(const bool value) = 0;
            virtual bool GetDisplayFilled() = 0;
            virtual void SetAffectedBySun(const bool value) = 0;
            virtual bool GetAffectedBySun() = 0;
            virtual void SetViewDistRatio(const float value) = 0;
            virtual float GetViewDistRatio() = 0;
            virtual void SetSkyOnly(const bool value) = 0;
            virtual bool GetSkyOnly() = 0;
            virtual void SetOceanIsVisible(const bool value) = 0;
            virtual bool GetOceanIsVisible() = 0;
            virtual void SetUseDeepness(const bool value) = 0;
            virtual bool GetUseDeepness() = 0;
            virtual void SetDoubleSide(const bool value) = 0;
            virtual bool GetDoubleSide() = 0;
            virtual void SetLightBlending(const bool value) = 0;
            virtual bool GetLightBlending() = 0;
            virtual void SetLightBlendValue(const float value) = 0;
            virtual float GetLightBlendValue() = 0;
    };

    using PortalRequestBus = AZ::EBus<PortalRequests>;

} // namespace Visibility
