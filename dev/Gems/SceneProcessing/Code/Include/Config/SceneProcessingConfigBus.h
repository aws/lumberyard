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

#include <AzCore/std/containers/vector.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class SoftNameSetting;
        class SceneProcessingConfigRequests
            : public EBusTraits
        {

        public:
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
            
            virtual const AZStd::vector<SoftNameSetting*>* GetSoftNames() = 0;
        };

        using SceneProcessingConfigRequestBus = EBus<SceneProcessingConfigRequests>;
    } // namespace SceneProcessingConfig
} // namespace AZ