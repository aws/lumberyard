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
#include <AzCore/Module/Module.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <CrySystemBus.h>
#include <IGem.h>

namespace CloudsGem
{
    /*! The CloudsSystemComponent performs initialization/shutdown tasks in coordination with other system components. */
    class CloudsSystemComponent
        : public AZ::Component
        , private CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(CloudsSystemComponent, "{10952273-903A-4B2F-9C64-EF75193B941A}");

        static void Reflect(AZ::ReflectContext* context);

        CloudsSystemComponent() = default;
        ~CloudsSystemComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SkyCloudsService", 0xd06a5bea));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SkyCloudsService", 0xd06a5bea));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required) { }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent) { }

    private:

        AZ_DISABLE_COPY_MOVE(CloudsSystemComponent);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;
    };
}
