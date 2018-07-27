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

#include <AzCore/base.h>
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "Road.h"

namespace RoadsAndRivers
{
    /**
     * Component to make roads
     */
    class RoadComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(RoadComponent, "{3700DA70-407D-47A6-B788-4654EDDA32A1}");
        ~RoadComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        template <typename T>
        void SetRoad(T&& road)
        {
            m_road = AZStd::forward<T>(road);
        }

    private:
        Road m_road;
    };
}