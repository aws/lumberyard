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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <qglobal.h> // For qreal
#include <qnamespace.h> // For Qt::DockArea
#include <QColor>
#include <QString>
#include <QMap>
#include <QPointer>
#include <QRect>
#include <QWidget>
#include <QPolygon>

class QScreen;
class QMainWindow;
class QPainter;

namespace AzQtComponents
{
    struct AZ_QT_COMPONENTS_API FancyDockingDropZoneConstants
    {
        // Constant for the opacity of the screen grab for the dock widget being dragged
        qreal draggingDockWidgetOpacity;

        // Constant for the opacity of the normal drop zones
        qreal dropZoneOpacity;

        // Constant for the default drop zone size (in pixels)
        int dropZoneSizeInPixels;

        // Constant for the dock width/height size (in pixels) before we need to start
        // scaling down the drop zone sizes, or else they will overlap with the center
        // tab icon or each other
        int minDockSizeBeforeDropZoneScalingInPixels;

        // Constant for the factor by which we must scale down the drop zone sizes once
        // the dock width/height size is too small
        qreal dropZoneScaleFactor;

        // Constant for the percentage to scale down the inner drop zone rectangle for the center tab drop zone
        qreal centerTabDropZoneScale;

        // Constant for the percentage to scale down the center tab drop zone for the center tab icon
        qreal centerTabIconScale;

        // Constant for the drop zone hotspot default color
        QColor dropZoneColor;

        // Constant for the drop zone border color
        QColor dropZoneBorderColor;

        // Constant for the border width in pixels separating the drop zones
        int dropZoneBorderInPixels;

        // Constant for the border width in pixels separating the drop zones
        int absoluteDropZoneSizeInPixels;

        // Constant for the delay (in milliseconds) before a drop zone becomes active
        // once it is hovered over
        int dockingTargetDelayMS;

        // Constant for the rate at which we will update (fade in) the drop zone opacity
        // when hovered over (in milliseconds)
        int dropZoneHoverFadeUpdateIntervalMS;

        // Constant for the incremental opacity increase for the hovered drop zone
        // that will fade in to the full drop zone opacity in the desired time
        qreal dropZoneHoverFadeIncrement;

        // Constant for the path to the center drop zone tabs icon
        QString centerDropZoneIconPath;

        FancyDockingDropZoneConstants();

        FancyDockingDropZoneConstants(const FancyDockingDropZoneConstants&) = delete;
        FancyDockingDropZoneConstants& operator=(const FancyDockingDropZoneConstants&) = delete;
    };

    struct FancyDockingDropZoneState
    {
        // The drop zone area mapped to the QPolygon in which we can drop QDockWidget for that zone
        QMap<Qt::DockWidgetArea, QPolygon> dropZones;

        // The QMainWindow or QDockWidget on which we are going to drop
        QPointer<QWidget> dropOnto;

        Qt::DockWidgetArea dropArea;

        // Used in conjunction with the above timer, the opacity of a drop zone
        // when hovered over will fade in dynamically
        qreal dropZoneHoverOpacity = 0.0f;

        QColor dropZoneColorOnHover;

        // The absolute drop zone rectangle and drop area
        bool onAbsoluteDropZone = false;
        QRect absoluteDropZoneRect;
        Qt::DockWidgetArea absoluteDropZoneArea;

        // The outer and inner rectangles of our current drop zones
        QRect dockDropZoneRect;
        QRect innerDropZoneRect;

        bool dragging = false;
    };

    // Splitting this out into a separate widget so that we can have one per screen
    // and so that they can be detached from any other widgets.
    // This seems to be the only reliable way to get the drop zone rendering to
    // work across multiple monitors under a number of different scenarios
    // such as multiple monitors with different scale factors and different
    // monitors selected as the primary monitor (which can matter)
    class AZ_QT_COMPONENTS_API FancyDockingDropZoneWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit FancyDockingDropZoneWidget(QMainWindow* mainWindow, QWidget* coordinatesRelativeTo, QScreen* screen, FancyDockingDropZoneState* dropZoneState);
        ~FancyDockingDropZoneWidget();

        QScreen* GetScreen();

        void Start();
        void Stop();

        static bool CheckModifierKey();

    protected:
        void paintEvent(QPaintEvent* ev) override;
        void closeEvent(QCloseEvent* ev) override;

    private:
        void paintDropZone(const Qt::DockWidgetArea area, QPolygon dropZoneShape, QPainter& painter);
        void fillAbsoluteDropZone(QPainter& painter);
        void paintDropBorderLines(QPainter& painter);

        QMainWindow* m_mainWindow;
        QWidget* m_relativeTo;
        QScreen* m_screen;

        FancyDockingDropZoneState* m_dropZoneState;
    };
} // namespace AzQtComponents