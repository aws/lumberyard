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

#include <qglobal.h> // For Q_OS_WIN

#ifdef Q_OS_WIN
# pragma warning(disable: 4127) // warning C4127: conditional expression is constant in qvector.h when including qpainter.h
#endif

#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/ButtonDivider.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/DockBar.h>
#include <AzQtComponents/Components/DockMainWindow.h>

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QWindow>
#include <QPainter>
#include <QMenu>
#include <QDesktopWidget>
#include <QtGui/private/qhighdpiscaling_p.h>

// Constant for the button layout margins when drawing in simple mode (in pixels)
static const int g_simpleBarButtonMarginsInPixels = 1;
// Constant for our title bar color when drawing in simple mode
static const QColor g_simpleBarColor(221, 221, 221);
// Constant for our title bar text color when drawing in simple mode
static const QColor g_simpleBarTextColor(0, 0, 0);

namespace AzQtComponents
{
    // Works like QWidget::window() but also considers a docked QDockWidget.
    // This is used when clicking on the custom titlebar, if the custom titlebar is on a dock widget
    // we don't want to close the host, but only the dock.
    QWidget* actualTopLevelFor(QWidget* w)
    {
        if (!w)
        {
            return nullptr;
        }

        if (w->isWindow())
        {
            return w;
        }

        if (qobject_cast<QDockWidget*>(w))
        {
            return w;
        }

        return actualTopLevelFor(w->parentWidget());
    }

    TitleBar::TitleBar(QWidget* parent)
        : QWidget(parent)
        , m_dockBar(new DockBar(this))
    {
        setFixedHeight(DockBar::Height);
        setButtons({ DockBarButton::CloseButton });
        setMouseTracking(true);

        // We have to do the lowering of the title bar whenever the widget goes from being docked in a QDockWidgetGroupWindow, but
        // there's no reliable way to figure out when that is, so we use a timer instead and just lower it ever time through.
        const int FIX_STACK_ORDER_INTERVAL_IN_MILLISECONDS = 500;
        startTimer(FIX_STACK_ORDER_INTERVAL_IN_MILLISECONDS);

        connect(&m_enableMouseTrackingTimer, &QTimer::timeout, this, &TitleBar::checkEnableMouseTracking);
        m_enableMouseTrackingTimer.setInterval(500);
    }

    TitleBar::~TitleBar()
    {
    }

    void TitleBar::handleClose()
    {
        actualTopLevelFor(this)->close();
    }

    void TitleBar::handleMaximize()
    {
        QWidget* w = window();
        if (isMaximized())
        {
            w->showNormal();
        }
        else
        {
            w->showMaximized();

            // Need to separately resize based on the available geometry for
            // the screen because since floating windows are frameless, on
            // Windows 10 they end up taking up the entire screen when maximized
            // instead of respecting the available space (e.g. taskbar)
            w->setGeometry(QApplication::desktop()->availableGeometry(w));
        }
    }

    void TitleBar::handleMinimize()
    {
        window()->showMinimized();
    }

    bool TitleBar::hasButton(DockBarButton::WindowDecorationButton button) const
    {
        return m_buttons.contains(button);
    }

    void TitleBar::handleMoveRequest()
    {
        delete m_interactiveWindowGeometryChanger;
        if (auto topLevelWidget = window())
        {
            m_interactiveWindowGeometryChanger = new InteractiveWindowMover(topLevelWidget->windowHandle(), this);
        }
    }

    void TitleBar::handleSizeRequest()
    {
        delete m_interactiveWindowGeometryChanger;
        if (auto topLevelWidget = window())
        {
            m_interactiveWindowGeometryChanger = new InteractiveWindowResizer(topLevelWidget->windowHandle(), this);
        }
    }

    void TitleBar::setDrawSideBorders(bool enable)
    {
        if (m_drawSideBorders != enable)
        {
            m_drawSideBorders = enable;
            update();
        }
    }

    /**
     * Change the title bar drawing mode between default and simple
     */
    void TitleBar::setDrawSimple(bool enable)
    {
        if (m_drawSimple != enable)
        {
            m_drawSimple = enable;
            setupButtons();
            update();
        }
    }

    void TitleBar::setDragEnabled(bool enable)
    {
        m_dragEnabled = enable;
    }

    void TitleBar::setTearEnabled(bool enable)
    {
        if (enable != m_tearEnabled)
        {
            m_tearEnabled = enable;
            update();
        }
    }

    QSize TitleBar::sizeHint() const
    {
        // This is needed so double clicking and dragging works out of the box.
        // Only the height value is important.
        return QSize(DockBar::Height, DockBar::Height);
    }

    void TitleBar::setWindowTitleOverride(const QString& titleOverride)
    {
        if (m_titleOverride != titleOverride)
        {
            m_titleOverride = titleOverride;
            update();
        }
    }

    void TitleBar::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        const int buttonsX = m_firstButton ? m_firstButton->x() : width();
        if (m_drawSimple)
        {
            // The simple drawing mode just draws a solid color rectangle with
            // an application icon and the application name as the title
            m_dockBar->DrawSolidBackgroundSegment(painter, rect(), buttonsX, true,
                g_simpleBarColor, g_simpleBarTextColor, QApplication::applicationName());
        }
        else
        {
            // The default drawing mode draws a more complex background, an optional
            // tear icon, and displays the window title
            // The application icon will also be drawn for the main editor window
            m_dockBar->DrawSegment(painter, rect(), buttonsX, m_tearEnabled,
                m_drawSideBorders, m_dockBar->GetColors(!m_forceInactive && window()->isActiveWindow()), title());
        }
    }

    QString TitleBar::title() const
    {
        QString result;
        if (!m_titleOverride.isEmpty())
        {
            // title override takes precedence.
            result = m_titleOverride;
        }
        else if (auto dock = qobject_cast<QDockWidget*>(parentWidget()))
        {
            // A docked QDockWidget has the title of the dockwidget, not of the top-level QWidget

            result = dock->windowTitle();
        }
        else if (auto w = window())
        {
            result = w->windowTitle();
        }

        if (result.isEmpty())
        {
            result = QApplication::applicationName();
        }

        return result;
    }

    void TitleBar::contextMenuEvent(QContextMenuEvent*)
    {
        // If this titlebar is for the main editor window or one of the floating
        // title bars, then use the standard context menu with min/max/close/etc... actions
        StyledDockWidget* dockWidgetParent = qobject_cast<StyledDockWidget*>(parentWidget());
        // Main Window title bar, old title bar in old docking, and new title bar will use standard context menu
        if (!dockWidgetParent || m_drawSimple)
        {
            updateStandardContextMenu();
        }
        // the old title bar in the new docking will use new context menu
        else
        {
            updateDockedContextMenu();
        }

        m_contextMenu->exec(QCursor::pos());
    }

    bool TitleBar::usesCustomTopBorderResizing() const
    {
        // On Win < 10 we're not overlapping the titlebar, removing it works fine there.
        // On Win < 10 we use native resizing of the top border.
        return QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS10;
    }

    void TitleBar::checkEnableMouseTracking()
    {
        // We don't get a mouse release when detaching a dock widget, so much do it with a workaround
        if (!hasMouseTracking())
        {
            if (!isLeftButtonDown() && (!isInDockWidget() || isInFloatingDockWidget()))
            {
                setMouseTracking(true);
                updateMouseCursor(QCursor::pos());
            }
            else
            {
                m_enableMouseTrackingTimer.start();
            }
        }
    }

    QWidget* TitleBar::dockWidget() const
    {
        // If this titlebar is being used in a dock widget, returns that dockwidget
        // The result can either be a QDockWidget or a QDockWidgetGroupWindow
        if (auto dock = qobject_cast<QDockWidget*>(actualTopLevelFor(const_cast<TitleBar*>(this))))
        {
            return dock;
        }
        else if (isInDockWidgetWindowGroup())
        {
            return window();
        }

        return nullptr;
    }

    bool TitleBar::isInDockWidget() const
    {
        return dockWidget() != nullptr || isInDockWidgetWindowGroup();
    }

    bool TitleBar::isInFloatingDockWidget() const
    {
        if (QWidget *win = window())
        {
            return qobject_cast<QDockWidget*>(win)
                || strcmp(win->metaObject()->className(), "QDockWidgetGroupWindow") == 0;
        }

        return false;
    }

    /**
     * Setup our context menu for the main editor window and floating title bars
     */
    void TitleBar::updateStandardContextMenu()
    {
        if (!m_contextMenu)
        {
            m_contextMenu = new QMenu(this);

            m_restoreMenuAction = m_contextMenu->addAction(tr("Restore"));
            QIcon restoreIcon;
            restoreIcon.addFile(":/stylesheet/img/titlebarmenu/restore.png");
            restoreIcon.addFile(":/stylesheet/img/titlebarmenu/restore_disabled.png", QSize(), QIcon::Disabled);

            m_restoreMenuAction->setIcon(restoreIcon);
            connect(m_restoreMenuAction, &QAction::triggered, this, &TitleBar::handleMaximize);

            m_moveMenuAction = m_contextMenu->addAction(tr("Move"));
            connect(m_moveMenuAction, &QAction::triggered, this, &TitleBar::handleMoveRequest);

            m_sizeMenuAction = m_contextMenu->addAction(tr("Size"));
            connect(m_sizeMenuAction, &QAction::triggered, this, &TitleBar::handleSizeRequest);

            m_minimizeMenuAction = m_contextMenu->addAction(tr("Minimize"));
            QIcon minimizeIcon;
            minimizeIcon.addFile(":/stylesheet/img/titlebarmenu/minimize.png");
            minimizeIcon.addFile(":/stylesheet/img/titlebarmenu/minimize_disabled.png", QSize(), QIcon::Disabled);

            m_minimizeMenuAction->setIcon(minimizeIcon);
            connect(m_minimizeMenuAction, &QAction::triggered, this, &TitleBar::handleMinimize);

            m_maximizeMenuAction = m_contextMenu->addAction(tr("Maximize"));
            QIcon maximizeIcon;
            maximizeIcon.addFile(":/stylesheet/img/titlebarmenu/maximize.png");
            maximizeIcon.addFile(":/stylesheet/img/titlebarmenu/maximize_disabled.png", QSize(), QIcon::Disabled);

            m_maximizeMenuAction->setIcon(QIcon(maximizeIcon));
            connect(m_maximizeMenuAction, &QAction::triggered, this, &TitleBar::handleMaximize);

            m_contextMenu->addSeparator();

            m_closeMenuAction = m_contextMenu->addAction(tr("Close"));
            QIcon closeIcon;
            closeIcon.addFile(":/stylesheet/img/titlebarmenu/close.png", QSize());
            closeIcon.addFile(":/stylesheet/img/titlebarmenu/close_disabled.png", QSize(), QIcon::Disabled);

            m_closeMenuAction->setIcon(QIcon(closeIcon));
            m_closeMenuAction->setShortcut(QString("Alt+F4"));
            connect(m_closeMenuAction, &QAction::triggered, this, &TitleBar::handleClose);
        }

        QWidget *topLevelWidget = window();
        if (!topLevelWidget)
        {
            // Defensive, doesn't happen
            return;
        }

        m_restoreMenuAction->setEnabled(hasButton(DockBarButton::MaximizeButton) && isMaximized());
        m_moveMenuAction->setEnabled(!isMaximized());
        const bool isFixedSize = topLevelWidget->minimumSize() == topLevelWidget->maximumSize();
        m_sizeMenuAction->setEnabled(!isFixedSize && !isMaximized());
        m_minimizeMenuAction->setEnabled(hasButton(DockBarButton::MinimizeButton));
        m_maximizeMenuAction->setEnabled(hasButton(DockBarButton::MaximizeButton) && !isMaximized());
        m_closeMenuAction->setEnabled(hasButton(DockBarButton::CloseButton));

    }

    /**
     * Setup our context menu for all docked panes
     */
    void TitleBar::updateDockedContextMenu()
    {
        if (!m_contextMenu)
        {
            m_contextMenu = new QMenu(this);

            // Action to close our dock widget, and leave the text blank since
            // it will be dynamically set using the title of the dock widget
            m_closeMenuAction = m_contextMenu->addAction(QString());
            QObject::connect(m_closeMenuAction, &QAction::triggered, this, &TitleBar::handleClose);

            // Unused in this context, but still here for consistency
            m_closeGroupMenuAction = m_contextMenu->addAction(tr("Close Tab Group"));

            // Separate the close actions from the undock actions
            m_contextMenu->addSeparator();

            // Action to undock our dock widget, and leave the text blank since
            // it will be dynamically set using the title of the dock widget
            m_undockMenuAction = m_contextMenu->addAction(QString());
            QObject::connect(m_undockMenuAction, &QAction::triggered, this, &TitleBar::undockAction);

            // Unused in this context, but still here for consistency
            m_undockGroupMenuAction = m_contextMenu->addAction(tr("Undock Tab Group"));
        }

        // Update the menu labels for the close/undock actions
        QString titleLabel = title();
        m_closeMenuAction->setText(tr("Close %1").arg(titleLabel));
        m_undockMenuAction->setText(tr("Undock %1").arg(titleLabel));

        // Don't enable the undock action if this dock widget is the only pane
        // in a floating window or if it isn't docked in one of our dock main windows
        StyledDockWidget* dockWidgetParent = qobject_cast<StyledDockWidget*>(parentWidget());
        DockMainWindow* dockMainWindow = nullptr;
        if (dockWidgetParent)
        {
            dockMainWindow = qobject_cast<DockMainWindow*>(dockWidgetParent->parentWidget());
        }
        m_undockMenuAction->setEnabled(dockMainWindow && dockWidgetParent && !dockWidgetParent->isSingleFloatingChild());

        // The group actions are always disabled, they are only provided for
        // menu consistency with the tab bar
        m_closeGroupMenuAction->setEnabled(false);
        m_undockGroupMenuAction->setEnabled(false);
    }

    void TitleBar::mousePressEvent(QMouseEvent* ev)
    {
        // use QCursor::pos(); in scenarios with multiple screens and different scale factors,
        // it's much more reliable about actually reporting a global position.
        QPoint globalPos = QCursor::pos();

        if (canResizeTop() && isTopResizeArea(globalPos))
        {
            m_resizingTop = true;
        }
        else if (canDragWindow())
        {
            QWidget *topLevel = window();
            auto topLevelWidth = topLevel->width();
            m_dragPos = topLevel->mapFromGlobal(globalPos);
            m_relativeDragPos = (double)m_dragPos.x() / ((double)topLevel->width());

            topLevelWidth++;
        }
        else
        {
            if (!isInFloatingDockWidget())
            {
                // Workaround Qt internal crash when detaching docked window group. Crashes if tracking enabled
                setMouseTracking(false);
                m_enableMouseTrackingTimer.start();
            }

            QWidget::mousePressEvent(ev);
        }
    }

    void TitleBar::mouseReleaseEvent(QMouseEvent* ev)
    {
        m_resizingTop = false;
        m_pendingRepositioning = false;
        m_dragPos = QPoint();
        QWidget::mouseReleaseEvent(ev);
    }

    QWindow* TitleBar::topLevelWindow() const
    {
        if (QWidget* topLevelWidget = window())
        {
            // TitleBar can be native instead of alien so calling this->window() would return
            // an unrelated window.
            return topLevelWidget->windowHandle();
        }

        return nullptr;
    }

    bool TitleBar::isTopResizeArea(const QPoint &globalPos) const
    {
        if (window() != parentWidget() && !isInDockWidgetWindowGroup())
        {
            // The immediate parent of the TitleBar must be a top level
            // if it's not then we're docked and we're not interested in resizing the top.
            return false;
        }

        if (QWindow* topLevelWin = topLevelWindow())
        {
            QPoint pt = mapFromGlobal(globalPos);
            const bool fixedHeight = topLevelWin->maximumHeight() == topLevelWin->minimumHeight();
            const bool maximized = topLevelWin->windowState() == Qt::WindowMaximized;
            return !maximized && !fixedHeight && pt.y() < DockBar::ResizeTopMargin;
        }

        return false;
    }

    QRect TitleBar::draggableRect() const
    {
        // This is rect() - the button rect, so we can enable aero-snap dragging in that space
        QRect r = rect();
        r.setWidth(m_firstButton->x() - layout()->spacing());
        return r;
    }

    bool TitleBar::canResizeTop() const
    {
        const QWidget *w = window();
        if (!w)
        {
            return false;
        }

        if (isInDockWidget() && !isInFloatingDockWidget())
        {
            // We return false for all embedded dock widgets
            return false;
        }

        return w && usesCustomTopBorderResizing() &&
            w->minimumHeight() < w->maximumHeight() && !w->isMaximized();
    }

    void TitleBar::updateMouseCursor(const QPoint& globalPos)
    {
        if (!usesCustomTopBorderResizing())
        {
            return;
        }

        if (isTopResizeArea(globalPos))
        {
            if (cursor().shape() != Qt::SizeVerCursor)
            {
                m_originalCursor = cursor().shape();
                setCursor(Qt::SizeVerCursor);
            }
        }
        else
        {
            setCursor(m_originalCursor);
        }
    }

    void TitleBar::resizeTop(const QPoint& globalPos)
    {
        QWindow *w = topLevelWindow();
        QRect geo = w->geometry();

        const QRect maxGeo = geo.adjusted(0, -(w->maximumHeight() - w->height()), 0, 0);
        const QRect minGeo = geo.adjusted(0, w->height() - w->minimumHeight(), 0, 0);

        geo.setTop(globalPos.y());
        geo = geo.intersected(maxGeo);
        geo = geo.united(minGeo);

        if (geo != w->geometry())
        {
            w->setGeometry(geo);
        }
    }

    void TitleBar::dragWindow(const QPoint& globalPos)
    {
        QWidget* topLevel = window();
        if (!topLevel)
        {
            return;
        }

        if (isMaximized())
        {
            m_pendingRepositioning = true;

            // have to refresh this value, as the width might have changed
            m_lastLocalPosX = topLevel->mapFromGlobal(globalPos).x() / (topLevel->width() * 1.0);

            handleMaximize();
            return;
        }

        // Workaround QTBUG-47543
        if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS10)
        {
            update();
        }

        if (m_pendingRepositioning)
        {
            // This is the case when moving a maximized window, window is restored and placed at 0,0
            // or whatever is the top-left corner of your screen.
            // We try to maintain the titlebar at the same relative position as when you clicked it.
            // So, if you click on the beginning of a maximized titlebar, it will be restored and
            // your mouse continues to be at the beginning of the titlebar.
            // m_lastLocalPosX holds the place you clicked as a percentage of the width, because
            // width will be smaller when you restore.
            if (topLevel)
            {
                const int offset = m_lastLocalPosX * width();
                topLevel->move(globalPos - QPoint(offset, 0));
                m_dragPos.setX(offset);
            }
        }
        else
        {
            // This is the normal case, we move the window
            QWindow* wind = topLevel->windowHandle();

            if (wind->width() < m_dragPos.x())
            {
                // The window was resized while we were dragging to a screen with different (dpi) scale factor
                // It shrunk, so calculate a new sensible drag pos, because the old is out of screen
                m_dragPos.setX(m_relativeDragPos * wind->width());
            }

            // (Don't cache the margins, they are be different when moving to screens with different scale factors)
            const QPoint newPoint = globalPos - (m_dragPos + QPoint(wind->frameMargins().left(), wind->frameMargins().top()));
            topLevel->move(newPoint);
        }

        m_pendingRepositioning = false;
    }

    void TitleBar::mouseMoveEvent(QMouseEvent* ev)
    {
        // use QCursor::pos(); in scenarios with multiple screens and different scale factors,
        // it's much more reliable about actually reporting a global position.
        QPoint globalPos = QCursor::pos();

        if (isResizingTop())
        {
            resizeTop(globalPos);
        }
        else if (isDraggingWindow())
        {
            dragWindow(globalPos);
        }
        else
        {
            updateMouseCursor(globalPos);
            QWidget::mouseMoveEvent(ev);
            m_pendingRepositioning = false;
        }
    }

    void TitleBar::mouseDoubleClickEvent(QMouseEvent*)
    {
        StyledDockWidget* dockWidgetParent = qobject_cast<StyledDockWidget*>(parentWidget());
        if (m_buttons.contains(DockBarButton::MaximizeButton) || (dockWidgetParent && dockWidgetParent->isFloating()))
        {
            handleMaximize();
        }
    }

    void TitleBar::timerEvent(QTimerEvent*)
    {
        fixEnabled();
    }

    bool TitleBar::isInDockWidgetWindowGroup() const
    {
        // DockWidgetGroupWindow is not exposed as a public API class from Qt, so we have to check for it
        // based on the className instead.
        return window() && strcmp(window()->metaObject()->className(), "QDockWidgetGroupWindow") == 0;
    }

    void TitleBar::fixEnabled()
    {
        StyledDockWidget* dockWidgetParent = qobject_cast<StyledDockWidget*>(parentWidget());
        QWidget* groupWindowParent = dockWidgetParent ? dockWidgetParent->parentWidget() : nullptr;

        // DockWidgetGroupWindow has issues. It renders the TitleBar when floating (and only when it's floating), but renders everything else over top of it.
        // In that case, the TitleBar still gets mouse clicks, so if it's under a menu bar, the menu bar doesn't work.
        // The following is a workaround
        bool shouldBeLowered = false;
        if (isInDockWidgetWindowGroup() && groupWindowParent)
        {
            shouldBeLowered = true;
        }

        // if we're in a group, our title bar should never be over top of anything else
        // But! we don't want to muck with the order in any other case, because it could screw something up.
        // So we only lower it ever.

        if (shouldBeLowered)
        {
            lower();
        }
    }

    /**
     * Handle button press signals from our DockBarButtons based on their type
     */
    void TitleBar::handleButtonClicked(const DockBarButton::WindowDecorationButton type)
    {
        switch (type)
        {
        case DockBarButton::CloseButton:
            handleClose();
            break;
        case DockBarButton::MinimizeButton:
            handleMinimize();
            break;
        case DockBarButton::MaximizeButton:
            handleMaximize();
            break;
        }
    }

    bool TitleBar::isMaximized() const
    {
        return window() && window()->isMaximized();
    }

    void TitleBar::setupButtons()
    {
        qDeleteAll(findChildren<QWidget*>());

        delete layout();
        m_firstButton = nullptr;

        QHBoxLayout* l = new QHBoxLayout(this);

        // If we are drawing in simple mode, we need to set custom margins
        // for the button layout
        if (m_drawSimple)
        {
            l->setContentsMargins(g_simpleBarButtonMarginsInPixels, g_simpleBarButtonMarginsInPixels, g_simpleBarButtonMarginsInPixels, g_simpleBarButtonMarginsInPixels);
        }

        l->setSpacing(DockBar::ButtonsSpacing);
        l->addStretch();

        for (auto buttonType : m_buttons)
        {
            QWidget* w = nullptr;
            if (buttonType == DockBarButton::DividerButton)
            {
                w = new ButtonDivider(this);
            }
            else
            {
                // Use the dark style of buttons for the titlebars on floating
                // dock widget containers since they are a lighter color
                StyledDockWidget* dockWidgetParent = qobject_cast<StyledDockWidget*>(parentWidget());
                bool isDarkStyle = dockWidgetParent && dockWidgetParent->isFloating();

                DockBarButton* button = new DockBarButton(buttonType, this, isDarkStyle);
                QObject::connect(button, &DockBarButton::buttonPressed, this, &TitleBar::handleButtonClicked);
                w = button;
            }

            if (!m_firstButton)
            {
                m_firstButton = w;
            }

            l->addWidget(w);
        }

        // Don't add the extra right margin spacing for simple mode
        if (!m_drawSimple)
        {
            l->addSpacing(DockBar::CloseButtonRightMargin);
        }
    }

    bool TitleBar::isLeftButtonDown() const
    {
        return QApplication::mouseButtons() & Qt::LeftButton;
    }

    bool TitleBar::canDragWindow() const
    {
        // Dock widgets use the internal Qt drag implementation
        return m_dragEnabled;
    }

    /**
     * Helper function to determine if we are currently resizing our title bar
     * from the top of our widget
     */
    bool TitleBar::isResizingTop() const
    {
        return isLeftButtonDown() && m_resizingTop && canResizeTop();
    }

    /**
     * Helper function to determine if we are currently in the state of click+dragging
     * our title bar to reposition it
     */
    bool TitleBar::isDraggingWindow() const
    {
        return isLeftButtonDown() && !m_dragPos.isNull() && canDragWindow();
    }

    void TitleBar::setButtons(WindowDecorationButtons buttons)
    {
        if (buttons != m_buttons)
        {
            m_buttons = buttons;
            setupButtons();
        }
    }

    int TitleBar::numButtons() const
    {
        return m_buttons.size();
    }

    void TitleBar::setForceInactive(bool force)
    {
        if (m_forceInactive != force)
        {
            m_forceInactive = force;
            update();
        }
    }

#include <Components/Titlebar.moc>
} // namespace AzQtComponents

