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

namespace Visibility
{
    /**
    * Request bus for the VisArea component
    */
    class VisAreaComponentBus
        : public AZ::ComponentBus
    {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus Traits overrides
            // Using Defaults
            //////////////////////////////////////////////////////////////////////////

            virtual ~VisAreaComponentBus() {}
            
            virtual void SetHeight(const float value) = 0;
            virtual float GetHeight() = 0;
            virtual void SetDisplayFilled(const bool value) = 0;
            virtual bool GetDisplayFilled() = 0;
            virtual void SetAffectedBySun(const bool value) = 0;
            virtual bool GetAffectedBySun() = 0;
            virtual void SetViewDistRatio(const float value) = 0;
            virtual float GetViewDistRatio() = 0;
            virtual void SetOceanIsVisible(const bool value) = 0;
            virtual bool GetOceanIsVisible() = 0;
            virtual void SetVertices(const AZStd::vector<AZ::Vector3>& value) = 0;
            virtual const AZStd::vector<AZ::Vector3>& GetVertices() = 0;
    };

    using VisAreaComponentRequestBus = AZ::EBus<VisAreaComponentBus>;

} // namespace Visibility
