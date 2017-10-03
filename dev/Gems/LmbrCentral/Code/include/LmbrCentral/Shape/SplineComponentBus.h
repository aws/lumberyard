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
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainerInterface.h>

namespace LmbrCentral
{
    /**
     * Services provided by the Spline Component
     */
    class SplineComponentRequests
        : public AZ::ComponentBus
        , public AZ::VertexContainerInterface<AZ::Vector3>
    {
    public:
        virtual ~SplineComponentRequests() {}

        virtual AZ::ConstSplinePtr GetSpline() = 0;
        virtual void ChangeSplineType(AZ::u64 splineType) = 0;
        virtual void SetClosed(bool bClosed) = 0;
    };

    /**
     * Bus to service the Spline component event group
     */ 
    using SplineComponentRequestBus = AZ::EBus<SplineComponentRequests>;

    /**
     * Listener for spline changes.
     */
    class SplineComponentNotification
        : public AZ::ComponentBus
    {
    public:
        virtual ~SplineComponentNotification() {}

        /**
         * Called when the spline has changed.
         */
        virtual void OnSplineChanged() {}
    };

    /**
     * Bus to service the spline component notification group
     */ 
    using SplineComponentNotificationBus = AZ::EBus<SplineComponentNotification>;

} // namespace LmbrCentral