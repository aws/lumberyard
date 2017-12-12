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
#include <AzCore/Math/PolygonPrism.h>
#include "AzCore/Math/VertexContainerInterface.h"

namespace LmbrCentral
{
    /**
     * Services provided by the Polygon Prism Shape Component.
     */
    class PolygonPrismShapeComponentRequests
        : public AZ::ComponentBus
        , public AZ::VertexContainerInterface<AZ::Vector2>
    {
    public:
        virtual ~PolygonPrismShapeComponentRequests() {}

        virtual AZ::ConstPolygonPrismPtr GetPolygonPrism() = 0;

        /**
         * @brief Sets height of polygon shape.
         * @param height The height of the polygon shape.
         */
        virtual void SetHeight(float height) = 0;
    };

    /**
     * Bus to service the Polygon Prism Shape component event group.
     */
    using PolygonPrismShapeComponentRequestsBus = AZ::EBus<PolygonPrismShapeComponentRequests>;

    /**
     * Listener for polygon prism changes.
     */
    class PolygonPrismShapeComponentNotification
        : public AZ::ComponentBus
    {
    public:
        virtual ~PolygonPrismShapeComponentNotification() {}

        /**
         * Called when the polyon prism shape has changed.
         */
        virtual void OnPolygonPrismShapeChanged() {}
    };

    /**
     * Bus to service the polygon prism shape component notification group.
     */ 
    using PolygonPrismShapeComponentNotificationBus = AZ::EBus<PolygonPrismShapeComponentNotification>;
} // namespace LmbrCentral