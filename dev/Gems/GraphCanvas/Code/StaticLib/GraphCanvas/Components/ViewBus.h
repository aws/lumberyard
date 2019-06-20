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

#include <QRect>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>

class QImage;
class QResizeEvent;
class QWheelEvent;

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

    struct ViewLimits
    {
        static constexpr qreal ZOOM_MIN = 0.1;
        static constexpr qreal ZOOM_MAX = 2.0;
        static constexpr qreal ZOOM_RANGE = ZOOM_MAX - ZOOM_MIN;
        static_assert(ZOOM_RANGE != 0, "Zoom Range must be non-zero value.");
    };

    class GraphCanvasGraphicsView;

    //! ViewRequests
    //! Requests that are serviced by the \ref View component.
    class ViewRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewId;

        virtual void SetEditorId(const EditorId& editorId) = 0;
        virtual EditorId GetEditorId() const = 0;

        //! Set the scene that the view should render.
        virtual void SetScene(const AZ::EntityId&) = 0;

        //! Get the scene the view is displaying.
        virtual AZ::EntityId GetScene() const = 0;

        //! Clear the scene that the view is rendering, so it will be empty.
        virtual void ClearScene() = 0;

        virtual AZ::Vector2 GetViewSceneCenter() const = 0;

        //! Map a view coordinate to the scene coordinate space.
        virtual AZ::Vector2 MapToScene(const AZ::Vector2& /*viewPoint*/) = 0;

        //! Map a scene coordinate to the view coordinate space.
        virtual AZ::Vector2 MapFromScene(const AZ::Vector2& /*scenePoint*/) = 0;

        //! Sets the View params of the view
        virtual void SetViewParams(const ViewParams& /*viewParams*/) = 0;

        //! Changes the scene to display only the view area.
        virtual void DisplayArea(const QRectF& viewArea) = 0;

        //! Ensures that the specified area is centered and fully displayed.
        //! Tries to not change the scale value unless necessary.
        virtual void CenterOnArea(const QRectF& viewArea) = 0;

        //! Move the view center to posInSceneCoordinates
        virtual void CenterOn(const QPointF& posInSceneCoordinates) = 0;

        //! Get, in scene coordinates, the QRectF of the area covered by the entire content of the scene
        virtual QRectF GetCompleteArea() = 0;

        //! Send a wheelEvent to the CanvasGraphicsView
        virtual void WheelEvent(QWheelEvent* ev) = 0;

        //! Get, in scene coordinates, the QRectF of the area presented in the view
        virtual QRectF GetViewableAreaInSceneCoordinates() = 0;

        //! Get the view as a CanvasGraphicsView.
        virtual GraphCanvasGraphicsView* AsGraphicsView() = 0;

        //! Renders out the entire graph into a newly created QPixmap.
        virtual QImage* CreateImageOfGraph() = 0;

        //! Renders out the specified area of the graph into a newly created QPixmap.
        virtual QImage* CreateImageOfGraphArea(QRectF area) = 0;

        //! Returns the 'zoom' aka scale of the GraphCanvasGraphicsView object
        virtual qreal GetZoomLevel() const = 0;

        //! Pans the displayed scene by the specific amount in the specified time.
        virtual void PanSceneBy(QPointF repositioning, AZStd::chrono::milliseconds duration) = 0;

        //! Pans the display scene to the specificed point in the specified time.
        virtual void PanSceneTo(QPointF scenePoint, AZStd::chrono::milliseconds duration) = 0;
    };

    using ViewRequestBus = AZ::EBus<ViewRequests>;

    class ViewNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewId;

        //! Signaled whenever the view parameters of a view changes
        virtual void OnViewParamsChanged(const ViewParams& /*viewParams*/) {};

        //! The view was resized
        virtual void OnViewResized(QResizeEvent* /*event*/) {}

        //! The view was scrolled
        virtual void OnViewScrolled() {}

        //! The view was centered on an area using CenterOnArea()
        virtual void OnViewCenteredOnArea() {}

        //! The view was zoomed
        virtual void OnZoomChanged(qreal zoomLevel) {};
    };

    using ViewNotificationBus = AZ::EBus<ViewNotifications>;

    class ViewSceneNotifications
        : public AZ::EBusTraits
    {
    public:
        // Key here is the scene that the view is currently displaying
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Signaled whenever the alt keyboard modifier changes
        virtual void OnAltModifier(bool enabled) {};
    };

    using ViewSceneNotificationBus = AZ::EBus<ViewSceneNotifications>;
}
