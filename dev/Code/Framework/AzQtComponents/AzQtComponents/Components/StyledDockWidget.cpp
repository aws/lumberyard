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

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>

#include <QMainWindow>
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QGuiApplication>
#include <QWindow>
#include <QStylePainter>
#include <QStyleOptionFrame>

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 1) && defined(Q_OS_WIN32)
#include <QtGui/private/qwindow_p.h>
#endif

namespace AzQtComponents
{
    static bool forceSkipTitleBarOverdraw()
    {
#ifdef Q_OS_WIN
        static bool isWin10 = QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS10;
        if (!isWin10)
        {
            // non-win10 never uses title bar overdraw
            return true;
        }

        return false;
#else
        // Non-windows never uses title bar overdraw
        return true;
#endif
    }

    StyledDockWidget::StyledDockWidget(const QString& name, QWidget* parent)
        : StyledDockWidget(name, false, parent)
    {
    }

    StyledDockWidget::StyledDockWidget(const QString& name, bool skipTitleBarDrawing, QWidget* parent)
        : QDockWidget(name, parent)
        , m_skipTitleBarOverdraw(skipTitleBarDrawing || forceSkipTitleBarOverdraw())
    {
        init();
    }

    StyledDockWidget::StyledDockWidget(QWidget* parent)
        : StyledDockWidget(QString(), parent)
    {
    }

    void StyledDockWidget::init()
    {
        if (doesTitleBarOverdraw())
        {
            TitleBarOverdrawHandler::getInstance()->addTitleBarOverdrawWidget(this);
        }

        connect(this, &QDockWidget::topLevelChanged, this, &StyledDockWidget::onFloatingChanged);
        createCustomTitleBar();
    }

    StyledDockWidget::~StyledDockWidget()
    {
    }

    /**
     * Checks if this dock widget is the only visible dock widget in a floating
     * main window
     */
    bool StyledDockWidget::isSingleFloatingChild()
    {
        // Check if our parent is a fancy docking QMainWindow with no central
        // widget, which means it is one of the floating main windows
        QMainWindow* parentMainWindow = qobject_cast<QMainWindow*>(parentWidget());
        if (parentMainWindow && parentMainWindow->property("fancydocking_owner").isValid() && !parentMainWindow->centralWidget())
        {
            bool singleFloating = true;
            for (QDockWidget* dockWidget : parentMainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
            {
                if (dockWidget->isVisible() && dockWidget != this)
                {
                    singleFloating = false;
                    break;
                }
            }

            return singleFloating;
        }

        return false;
    }

    void StyledDockWidget::closeEvent(QCloseEvent* event)
    {
        // give the sub-widget a chance to veto the close; necessary for the UI Editor, among other things
        QCloseEvent closeEvent;
        QCoreApplication::sendEvent(widget(), &closeEvent);

        // If widget accepted the close event, we delete the dockwidget, which will also delete the child widget in case it doesn't have Qt::WA_DeleteOnClose
        if (!closeEvent.isAccepted())
        {
            // Widget doesn't want to close
            event->ignore();
            return;
        }

        Q_EMIT aboutToClose();
        QDockWidget::closeEvent(event);
    }

    void StyledDockWidget::showEvent(QShowEvent* event)
    {
        if (auto titleBar = qobject_cast<TitleBar*>(titleBarWidget()))
        {
            // When docked, we don't have a window frame, so draw the left and right border
            titleBar->setDrawSideBorders(!isFloating());
        }

        if (isFloating())
        {
            fixFramelessFlags();
        }

        QDockWidget::showEvent(event);
    }

    bool StyledDockWidget::nativeEvent(const QByteArray& eventType, void* message, long* result)
    {
        return WindowDecorationWrapper::handleNativeEvent(eventType, message, result, this);
    }

    /**
     * Override of event handler so that we can ignore the NonClientAreaXXX
     * events on our dock widgets.  This fixes an issue where the QDockWidget
     * only respects the movable feature on mouse press, not on non client area
     * events (e.g. resizing).  We disable the movable feature on our dock widgets
     * so that we can use our own custom docking solution instead of the default qt
     * docking, but we need to override these events that get triggered when resizing
     * otherwise it will activate the default qt docking.
     */
    bool StyledDockWidget::event(QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::NonClientAreaMouseMove:
        case QEvent::NonClientAreaMouseButtonPress:
        case QEvent::NonClientAreaMouseButtonRelease:
        case QEvent::NonClientAreaMouseButtonDblClick:
            return true;
        }

        return QDockWidget::event(event);
    }

    void StyledDockWidget::paintEvent(QPaintEvent*)
    {
        // By default QDockWidget::paintEvent only draws the frame if the dock widget doesn't have
        // a custom title bar and does not have native window decorations. As a result, QDockWidget
        // cannot be styled using QSS when floating.
        // UI 1.0 paint code moved to EditorProxyStyle::drawPrimitive.
        QStylePainter p(this);
        QStyleOptionFrame framOpt;
        framOpt.init(this);
        p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
    }

    bool StyledDockWidget::doesTitleBarOverdraw() const
    {
        return !m_skipTitleBarOverdraw;
    }

    bool StyledDockWidget::skipTitleBarOverdraw() const
    {
        return m_skipTitleBarOverdraw;
    }

    void StyledDockWidget::fixFramelessFlags()
    {
        // This ensures we have native frames (but no native titlebar)
        QWindow* w = windowHandle();
        if (doesTitleBarOverdraw() && w && (w->flags() & Qt::FramelessWindowHint) && isFloating())
        {
            w->setFlags(WindowDecorationWrapper::specialFlagsForOS() | Qt::Tool);
        }
    }

    void StyledDockWidget::onFloatingChanged(bool floating)
    {
        if (floating)
        {
            fixFramelessFlags();
        }

        // If we have a custom title bar, then we need to enable the dragging
        // to reposition our dock widget if floating is enabled, change it
        // to be drawn in simple mode, and update the buttons
        // @FIXME This should only be executed if fancy docking is enabled,
        // this extra check can be removed once fancy docking is changed
        // to the default, as opposed to being disabled by default
        if (objectName().startsWith("_fancydocking_"))
        {
            TitleBar* titleBar = customTitleBar();
            if (titleBar)
            {
                titleBar->setDragEnabled(floating);
                titleBar->setDrawSimple(floating);
                if (floating)
                {
                    titleBar->setButtons({ DockBarButton::MaximizeButton, DockBarButton::CloseButton });
                }
            }
        }
    }

    void StyledDockWidget::createCustomTitleBar()
    {
        QWidget* tw = titleBarWidget();
        if (tw)
        {
            tw->deleteLater();
        }

        TitleBar* titleBar = new TitleBar(this);
        titleBar->setTearEnabled(true);
        titleBar->setDrawSideBorders(false);
        QObject::connect(titleBar, &TitleBar::undockAction, this, &StyledDockWidget::undock);
        setTitleBarWidget(titleBar);
    }

    /** static */
    void StyledDockWidget::drawFrame(QPainter& p, QRect rect, bool drawTop)
    {
        p.save();
        p.setPen(QColor(33, 34, 35));

        rect.adjust(0, p.pen().width(), 0, 0);
        if (drawTop)
        {
            p.drawLine(QLine(rect.topLeft(), rect.topRight()));
        }

        p.drawLine(QLine(rect.topLeft(), rect.bottomLeft()));
        p.drawLine(QLine(rect.topRight(), rect.bottomRight()));
        p.drawLine(QLine(rect.bottomLeft(), rect.bottomRight()));

        p.restore();
    }

    TitleBar* StyledDockWidget::customTitleBar() const
    {
        return qobject_cast<TitleBar*>(titleBarWidget());
    }

#include <Components/StyledDockWidget.moc>
} // namespace AzQtComponents
