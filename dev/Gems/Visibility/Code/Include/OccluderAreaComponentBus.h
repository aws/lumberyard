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
    class OccluderAreaRequests
        : public AZ::ComponentBus
    {
        public:
            
            virtual ~OccluderAreaRequests() {}

            virtual void SetDisplayFilled(const bool value) = 0;
            virtual bool GetDisplayFilled() = 0;
            virtual void SetCullDistRatio(const float value) = 0;
            virtual float GetCullDistRatio() = 0;
            virtual void SetUseInIndoors(const bool value) = 0;
            virtual bool GetUseInIndoors() = 0;
            virtual void SetDoubleSide(const bool value) = 0;
            virtual bool GetDoubleSide() = 0;
    };

    using OccluderAreaRequestBus = AZ::EBus<OccluderAreaRequests>;

} // namespace Visibility
