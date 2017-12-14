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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>

class QGraphicsView;

namespace GraphCanvas
{
    class ViewParams
    {
    public:
        AZ_TYPE_INFO(ViewParams, "{D016BF86-DFBB-4AF0-AD26-27F6AB737740}");
        double m_scale = 1.0;
        
        float m_anchorPointX = 0.0f;
        float m_anchorPointY = 0.0f;
    };

    typedef AZ::EntityId ViewId;
    
    //! ViewRequests
    //! Requests that are serviced by the \ref View component.
    class ViewRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewId;
        
        //! Set the scene that the view should render.
        virtual void SetScene(const AZ::EntityId&) = 0;

        //! Get the scene the view is displaying.
        virtual AZ::EntityId GetScene() const = 0;

        //! Clear the scene that the view is rendering, so it will be empty.
        virtual void ClearScene() = 0;

        //! Map a view coordinate to the scene coordinate space.
        virtual AZ::Vector2 MapToScene(const AZ::Vector2& /*viewPoint*/) = 0;

        //! Map a scene coordinate to the view coordinate space.
        virtual AZ::Vector2 MapFromScene(const AZ::Vector2& /*scenePoint*/) = 0;
        
        //! Sets the View params of the view
        virtual void SetViewParams(const ViewParams& /*viewParams*/) = 0;

        //! Frame the selection into view.
        virtual void FrameSelectionIntoView() = 0;

        virtual QGraphicsView* AsQGraphicsView() = 0;
    };

    using ViewRequestBus = AZ::EBus<ViewRequests>;
    
    class ViewNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewId;
        
        //! Signalled whenever the view parameters of a view changes
        virtual void OnViewParamsChanged(const ViewParams& viewParams) {};
    };

    using ViewNotificationBus = AZ::EBus<ViewNotifications>;

    class ViewSceneNotifications
        : public AZ::EBusTraits
    {
    public:
        // Key here is the scene that the view is currently displaying
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Signalled whenever the alt keyboard modifier changes
        virtual void OnAltModifier(bool enabled) {};
    };

    using ViewSceneNotificationBus = AZ::EBus<ViewSceneNotifications>;

}