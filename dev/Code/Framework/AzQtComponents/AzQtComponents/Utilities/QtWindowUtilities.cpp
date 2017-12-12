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

#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QPainter>
#include <QDockWidget>
#include <QCursor>
#include <QScreen>

namespace AzQtComponents
{
    QRect GetTotalScreenGeometry()
    {
        QDesktopWidget* desktopWidget = QApplication::desktop();

        QRect totalScreenRect;
        int numScreens = desktopWidget->screenCount();
        for (int i = 0; i < numScreens; ++i)
        {
            totalScreenRect = totalScreenRect.united(desktopWidget->screenGeometry(i));
        }

        return totalScreenRect;
    }

    void EnsureWindowWithinScreenGeometry(QWidget* widget)
    {
        // manipulate the window, not the input widget. Might be the same thing
        widget = widget->window();

        // get the edges of the screens
        QRect screenGeometry = GetTotalScreenGeometry();

        // check if we cross over any edges
        QRect geometry = widget->geometry();

        const bool mustBeEntirelyInside = true;
        if (screenGeometry.contains(geometry, mustBeEntirelyInside))
        {
            return;
        }

        if ((geometry.x() + geometry.width()) > screenGeometry.right())
        {
            geometry.moveTo(screenGeometry.right() - geometry.width(), geometry.top());
        }

        if (geometry.x() < screenGeometry.left())
        {
            geometry.moveTo(screenGeometry.left(), geometry.top());
        }

        if ((geometry.y() + geometry.height()) > screenGeometry.bottom())
        {
            geometry.moveTo(geometry.x(), screenGeometry.bottom() - geometry.height());
        }

        if (geometry.y() < screenGeometry.top())
        {
            geometry.moveTo(geometry.x(), screenGeometry.top());
        }

        widget->setGeometry(geometry);
    }

    void SetClipRegionForDockingWidgets(QWidget* widget, QPainter& painter, QMainWindow* mainWindow)
    {
        QRegion clipRegion(widget->rect());

        for (QDockWidget* childDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
        {
            if (childDockWidget->isFloating() && childDockWidget->isVisible())
            {
                QRect globalDockRect = childDockWidget->geometry();
                QRect localDockRect(widget->mapFromGlobal(globalDockRect.topLeft()), globalDockRect.size());

                clipRegion = clipRegion.subtracted(QRegion(localDockRect));
            }
        }

        painter.setClipRegion(clipRegion);
        painter.setClipping(true);
    }

    void SetCursorPos(const QPoint& point)
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        bool finished = false;
        for (int screenIndex = 0; !finished && screenIndex < screens.size(); ++screenIndex)
        {
            QScreen* screen = screens[screenIndex];
            if (screen->geometry().contains(point))
            {
                QCursor::setPos(screen, point);
                finished = true;
            }
        }
    }
    
    void SetCursorPos(int x, int y)
    {
        SetCursorPos(QPoint(x, y));
    }

} // namespace AzQtComponents

