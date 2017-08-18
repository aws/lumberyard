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
#ifndef AZCORE_OBJECTSTREAM_COMPONENT_H
#define AZCORE_OBJECTSTREAM_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    /**
     *
     */
    class ObjectStreamComponent
        : public Component
    {
    public:
        AZ_COMPONENT(AZ::ObjectStreamComponent, "{22FC6380-C34F-4a59-86B4-21C0276BCEE3}")

        ObjectStreamComponent();

    private:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);
    };
}

#endif // AZCORE_OBJECTSTREAM_COMPONENT_H
#pragma once