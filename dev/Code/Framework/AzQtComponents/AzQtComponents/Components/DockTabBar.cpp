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

#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/DockBar.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QAction>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>

// Constant for the width of the close button and its total offset (width + margin spacing)
static const int g_closeButtonWidth = 19;
static const int g_closeButtonOffset = g_closeButtonWidth + AzQtComponents::DockBar::ButtonsSpacing;
// Constant for the color of our tab indicator underlay
static const QColor g_tabIndicatorUnderlayColor(Qt::black);
// Constant for the opacity of our tab indicator underlay
static const qreal g_tabIndicatorUnderlayOpacity = 0.75;
// Constant for the duration of our tab animations (in milliseconds)
static const int g_tabAnimationDurationMS = 250;


namespace AzQtComponents
{
    /**
     * Create a dock tab widget that extends a QTabWidget with a custom DockTabBar to replace the default tab bar
     */
    DockTabBar::DockTabBar(QWidget* parent)
        : QTabBar(parent)
        , m_dockBar(new DockBar(this))
        , m_closeTabButton(new DockBarButton(DockBarButton::CloseButton, this))
        , m_tabIndicatorUnderlay(new QWidget(this))
        , m_draggingTabImage(new QLabel(this))
        , m_displacedTabImage(new QLabel(this))
        , m_dragTabFinishedAnimation(new QPropertyAnimation(m_draggingTabImage, "geometry", this))
        , m_displacedTabAnimation(new QPropertyAnimation(m_displacedTabImage, "geometry", this))
        , m_leftButton(nullptr)
        , m_rightButton(nullptr)
        , m_contextMenu(nullptr)
        , m_closeTabMenuAction(nullptr)
        , m_closeTabGroupMenuAction(nullptr)
        , m_menuActionTabIndex(-1)
        , m_tabIndexPressed(-1)
        , m_dragInProgress(false)
        , m_displacedTabIndex(-1)
    {
        setFixedHeight(DockBar::Height);

        // Hide our tab animation placeholder images by default
        m_displacedTabImage->hide();
        m_draggingTabImage->hide();

        // Configure our animation for sliding the tab we are dragging into its
        // final position when done re-ordering
        m_dragTabFinishedAnimation->setDuration(g_tabAnimationDurationMS);
        m_dragTabFinishedAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        QObject::connect(m_dragTabFinishedAnimation, &QPropertyAnimation::finished, this, &DockTabBar::dragTabAnimationFinished);

        // Configure our animation for sliding tabs as they are displaced while
        // dragging the tab around to re-order
        m_displacedTabAnimation->setDuration(g_tabAnimationDurationMS);
        m_displacedTabAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        QObject::connect(m_displacedTabAnimation, &QPropertyAnimation::finished, this, &DockTabBar::displacedTabAnimationFinished);

        // Handle our close tab button clicks
        QObject::connect(m_closeTabButton, &DockBarButton::buttonPressed, this, &DockTabBar::handleButtonClicked);

        // Handle when our current tab index changes
        QObject::connect(this, &QTabBar::currentChanged, this, &DockTabBar::currentIndexChanged);

        // Track when a user presses a tab so we can determine if we are going
        // to be dragging the tab to re-order
        QObject::connect(this, &QTabBar::tabBarClicked, this, &DockTabBar::tabPressed);

        // Our QTabBar base class has left/right indicator buttons for scrolling
        // through the tab header if all the tabs don't fit in the given space for
        // the widget, but they just float over the tabs, so we have added a
        // semi-transparent underlay that will be positioned below them so that
        // it looks better
        QPalette underlayPalette;
        underlayPalette.setColor(QPalette::Background, g_tabIndicatorUnderlayColor);
        m_tabIndicatorUnderlay->setAutoFillBackground(true);
        m_tabIndicatorUnderlay->setPalette(underlayPalette);
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_tabIndicatorUnderlay);
        effect->setOpacity(g_tabIndicatorUnderlayOpacity);
        m_tabIndicatorUnderlay->setGraphicsEffect(effect);

        // The QTabBar has two QToolButton children that are used as left/right
        // indicators to scroll across the tab header when the width is too short
        // to fit all of the tabs
        for (QToolButton* button : findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly))
        {
            // Grab references to each button for use later
            if (button->accessibleName() == QTabBar::tr("Scroll Left"))
            {
                m_leftButton = button;
            }
            else
            {
                m_rightButton = button;
            }

            // Update our close button position anytime the scroll indicator
            // button is pressed since it will shift the tab header
            QObject::connect(button, &QToolButton::clicked, this, &DockTabBar::updateCloseButtonPosition, Qt::QueuedConnection);
        }
    }

    /**
     * Handle resizing appropriately when our parent tab widget is resized,
     * otherwise when there is only one tab it won't know to stretch to the
     * full width
     */
    QSize DockTabBar::sizeHint() const
    {
        if (count() == 1)
        {
            return tabSizeHint(0);
        }

        return QTabBar::sizeHint();
    }

    /**
     * Return the tab size for the given index with a variable width based on the title length, and a preset height
     */
    QSize DockTabBar::tabSizeHint(int index) const
    {
        // If there's only one tab, then let the tab take up the entire width so that it appears the
        // same as a title bar
        if (count() == 1 && parentWidget())
        {
            return QSize(parentWidget()->width(), DockBar::Height);
        }

        // Otherwise, use the variable width based on the title length
        const QString title = tabText(index);
        return QSize(m_dockBar->GetTitleMinWidth(title) + closeButtonOffsetForIndex(index), DockBar::Height);
    }

    /**
     * Any time the tab layout changes (e.g. tabs are added/removed/resized),
     * we need to check if we need to add our tab indicator underlay, and
     * update our tab close button position
     */
    void DockTabBar::tabLayoutChange()
    {
        // If the tab indicators are showing, then we need to show our underlay
        if (m_leftButton->isVisible())
        {
            // The underlay will take up the combined space behind the left and
            // right indicator buttons
            QRect total = m_leftButton->geometry();
            total = total.united(m_rightButton->geometry());
            m_tabIndicatorUnderlay->setGeometry(total);

            // The indicator buttons get raised when shown, so we need to stack
            // our underlay under the left button, which will place it under
            // both indicator buttons, and then show it
            m_tabIndicatorUnderlay->stackUnder(m_leftButton);
            m_tabIndicatorUnderlay->show();
        }
        else
        {
            m_tabIndicatorUnderlay->hide();
        }

        // Update the close button position, but don't need to update the geometry
        // since that triggered this layout change. We use a single shot timer for
        // this since the tab rects haven't been updated until the next cycle.
        QTimer::singleShot(0, this, &DockTabBar::updateCloseButtonPosition);
    }

    /**
     * Handle the right-click context menu event by displaying our custom menu
     * with options to close/undock individual tabs or the entire tab group
     */
    void DockTabBar::contextMenuEvent(QContextMenuEvent* event)
    {
        // Figure out the index of the tab the event was triggered on, or use
        // the currently active tab if the event was triggered in the header
        // dead zone
        int index = tabAt(event->pos());
        if (index == -1)
        {
            index = currentIndex();
        }
        m_menuActionTabIndex = index;

        // Need to create our context menu/actions if this is the first time
        // it has been invoked
        if (!m_contextMenu)
        {
            m_contextMenu = new QMenu(this);

            // Action to close the specified tab, and leave the text blank since
            // it will be dynamically set using the title of the specified tab
            m_closeTabMenuAction = m_contextMenu->addAction(QString());
            QObject::connect(m_closeTabMenuAction, &QAction::triggered, this, [this]() { emit closeTab(m_menuActionTabIndex); });

            // Action to close all of the tabs in our tab widget
            m_closeTabGroupMenuAction = m_contextMenu->addAction(tr("Close Tab Group"));
            QObject::connect(m_closeTabGroupMenuAction, &QAction::triggered, this, &DockTabBar::closeTabGroup);

            // Separate the close actions from the undock actions
            m_contextMenu->addSeparator();

            // Action to undock the specified tab, and leave the text blank since
            // it will be dynamically set using the title of the specified tab
            m_undockTabMenuAction = m_contextMenu->addAction(QString());
            QObject::connect(m_undockTabMenuAction, &QAction::triggered, this, [this]() { emit undockTab(m_menuActionTabIndex); });

            // Action to undock the entire tab widget
            m_undockTabGroupMenuAction = m_contextMenu->addAction(tr("Undock Tab Group"));
            QObject::connect(m_undockTabGroupMenuAction, &QAction::triggered, this ,[this]() { emit undockTab(-1); });
        }

        // Update the menu labels for the close/undock individual tab actions
        QString tabName = tabText(index);
        m_closeTabMenuAction->setText(tr("Close %1").arg(tabName));
        m_undockTabMenuAction->setText(tr("Undock %1").arg(tabName));

        // Only enable the close/undock group actions if we have more than one
        // tab in our tab widget
        bool enableGroupActions = (count() > 1);
        m_closeTabGroupMenuAction->setEnabled(enableGroupActions);

        // Disable the undock group action if our tab widget
        // container is the only dock widget in a floating window
        QWidget* tabWidget = parentWidget();
        bool enableUndock = true;
        if (tabWidget)
        {
            StyledDockWidget* tabWidgetContainer = qobject_cast<StyledDockWidget*>(tabWidget->parentWidget());
            if (tabWidgetContainer)
            {
                enableUndock = !tabWidgetContainer->isSingleFloatingChild();
            }
        }
        m_undockTabGroupMenuAction->setEnabled(enableGroupActions && enableUndock);

        // Enable the undock action if there are multiple tabs or if this isn't
        // a single tab in a floating window
        m_undockTabMenuAction->setEnabled(enableGroupActions || enableUndock);

        // Show the context menu
        m_contextMenu->exec(event->globalPos());
    }

    /**
     * Close all of the tabs in our tab widget
     */
    void DockTabBar::closeTabGroup()
    {
        // Close each of the tabs using our signal trigger so they are cleaned
        // up properly
        int numTabs = count();
        for (int i = 0; i < numTabs; ++i)
        {
            emit closeTab(0);
        }
    }

    /**
     * When our tab index changes, we need to force a resize event to trigger a layout change, since the tabSizeHint needs
     * to be updated because we only show the close button on the active tab
     */
    void DockTabBar::currentIndexChanged()
    {
        resizeEvent(nullptr);
    }

    /**
     * Move our close tab button to be positioned on the currently active tab
     */
    void DockTabBar::updateCloseButtonPosition()
    {
        int activeIndex = currentIndex();
        const QRect area = tabRect(activeIndex);
        int rightEdge = area.right();

        // If the tab indicator underlay is showing and overlaps with the
        // right edge of the active tab area, then we need to use the underlay's
        // left edge as the cutoff so that the close button won't be hidden
        if (m_tabIndicatorUnderlay->isVisible())
        {
            QRect underlayRect = m_tabIndicatorUnderlay->geometry();
            int underlayLeftEdge = underlayRect.left();
            if (underlayLeftEdge < rightEdge)
            {
                rightEdge = underlayLeftEdge;
            }
        }

        // If we don't have enough room to display the tab title and the close
        // button, then just don't show it
        const QString title = tabText(activeIndex);
        if (m_dockBar->GetTitleMinWidth(title) + area.left() > rightEdge)
        {
            rightEdge = -1;
        }

        // If the right edge of our active tab area is offscreen, or if we are
        // dragging a tab to re-order them, then just hide our close tab button
        if (rightEdge < 0 || m_dragInProgress)
        {
            m_closeTabButton->hide();
        }
        // Otherwise, position the close button inside the right edge of our
        // available area and show/raise it
        else
        {
            m_closeTabButton->move(rightEdge - g_closeButtonOffset, (DockBar::Height - m_closeTabButton->height()) / 2);
            m_closeTabButton->show();
            m_closeTabButton->raise();
        }
    }

    /**
     * The close button is only present on the active tab, so return the close button offset for the
     * current index, otherwise none
     */
    int DockTabBar::closeButtonOffsetForIndex(int index) const
    {
        return (currentIndex() == index) ? g_closeButtonOffset : 0;
    }

    /**
     * Handle button press signals from our DockBarButtons based on their type
     */
    void DockTabBar::handleButtonClicked(const DockBarButton::WindowDecorationButton type)
    {
        // Emit a close tab signal when our close tab button is pressed
        if (type == DockBarButton::CloseButton)
        {
            emit closeTab(currentIndex());
        }
    }

    /**
     * Override the paint event so we can draw our custom tabs
     */
    void DockTabBar::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        int numTabs = count();
        for (int i = 0; i < numTabs; ++i)
        {
            // If we are currently dragging a tab for re-ordering, then don't
            // draw that tab or any tab that is currently being animated as
            // displaced as the tab is being dragged around since we will be
            // dragging around an animated copy of the tab
            if ((i == m_tabIndexPressed || i == m_displacedTabIndex) && m_dragInProgress)
            {
                continue;
            }

            // Retrieve the bounding rectangle area and title for this tab
            const QRect area = tabRect(i);
            const QString title = tabText(i);

            // Retrieve the appropriate colors depending on whether or not this
            // is the active tab (and whether or not we're on the active window)
            bool activeTab = (currentIndex() == i) && window()->isActiveWindow();
            DockBarColors colors = m_dockBar->GetColors(activeTab);

            // Calculate the beginning x position of our close button (if it exists)
            const int buttonsX = area.right() - closeButtonOffsetForIndex(i);

            // Draw the dock bar segment for this tab
            m_dockBar->DrawSegment(painter, area, buttonsX, true, true, colors, title);
        }
    }

    /**
     * Handle when a tab is pressed so that we can determine if we need to start
     * dragging a tab for re-ordering
     */
    void DockTabBar::tabPressed(int index)
    {
        m_tabIndexPressed = index;
        m_dragStartPosition = QCursor::pos();
    }

    /**
     * Override the mouse press event handler to fix a Qt issue where the QTabBar
     * doesn't ensure that it's the left mouse button that has been pressed
     * early enough, even though it properly checks for it first in its mouse
     * release event handler
     */
    void DockTabBar::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            event->ignore();
            return;
        }

        QTabBar::mousePressEvent(event);
    }

    /**
     * Handle the mouse move events if we are dragging a tab to re-order it
     */
    void DockTabBar::mouseMoveEvent(QMouseEvent* event)
    {
        // If a tab hasn't been pressed, then there is nothing to drag
        if (m_tabIndexPressed == -1)
        {
            return;
        }

        QPoint globalPos = event->globalPos();
        // If we don't have a drag in progress yet, check if the mouse has been
        // dragged past the specified threshold to initiate a tab drag for
        // re-ordering, including capturing a static image of the dragged
        // tab so we can move it around
        if (!m_dragInProgress)
        {
            if ((globalPos - m_dragStartPosition).manhattanLength() > QApplication::startDragDistance())
            {
                QRect area = tabRect(m_tabIndexPressed);
                setImageForAnimatedLabel(m_draggingTabImage, area);
                m_draggingTabImage->raise();
                m_closeTabButton->hide();
                m_dragInProgress = true;
            }
        }
        else
        {
            // Move our dragged image of the pressed tab based on the new
            // mouse position
            int dragDistance = globalPos.x() - m_dragStartPosition.x();
            QRect tabPressedArea = tabRect(m_tabIndexPressed);
            QRect tabDraggedArea = tabPressedArea.translated(dragDistance, 0);
            m_draggingTabImage->setGeometry(tabDraggedArea);

            // Index of the tab the dragged tab is currently over
            int overIndex = -1;
            if (dragDistance < 0)
            {
                overIndex = tabAt(tabDraggedArea.topLeft());
            }
            else
            {
                overIndex = tabAt(tabDraggedArea.topRight());
            }

            if (overIndex != m_tabIndexPressed && overIndex != -1)
            {
                // Check if we should re-order the tabs if we have dragged the
                // tab past the center of the tab it is over
                QRect overRect = tabRect(overIndex);
                int overCenterX = overRect.center().x();
                bool shouldReorderTabs = (dragDistance < 0) ? (tabDraggedArea.left() < overCenterX) : (tabDraggedArea.right() > overCenterX);
                if (shouldReorderTabs)
                {
                    // Disable widget updates while we re-order the tabs and
                    // kick off an animation
                    setUpdatesEnabled(false);

                    // Stop the current displaced tab animation if it is still
                    // running
                    if (m_displacedTabAnimation->state() == QAbstractAnimation::Running)
                    {
                        m_displacedTabAnimation->stop();
                    }

                    // Update our displaced tab image with a screen grab
                    // of the tab that we're going to swap with
                    setImageForAnimatedLabel(m_displacedTabImage, overRect);
                    m_displacedTabImage->stackUnder(m_draggingTabImage);

                    // The index of the displaced tab is about to be the currently
                    // pressed tab since we're going to swap them, which signifies
                    // that the displaced tab animation is still in progress
                    m_displacedTabIndex = m_tabIndexPressed;

                    // Swap the dragged tab with the one it has displaced
                    moveTab(m_tabIndexPressed, overIndex);

                    // Update the pressed tab index to be the tab we just
                    // swapped it with
                    m_tabIndexPressed = overIndex;

                    // Update the drag start position to be relative to the new
                    // location of the dragged tab now that it has been swapped
                    // to the new position
                    QRect newRect = tabRect(overIndex);
                    int newX = globalPos.x() + (newRect.left() - tabDraggedArea.left());
                    m_dragStartPosition.setX(newX);

                    // Start the displaced tab animation to move the displaced
                    // tab from its current spot to its new location
                    m_displacedTabAnimation->setStartValue(overRect);
                    m_displacedTabAnimation->setEndValue(tabRect(m_displacedTabIndex));
                    m_displacedTabAnimation->start();

                    // Re-enable widget updates now that we're done re-ordering
                    setUpdatesEnabled(true);
                }
            }
        }
    }

    /**
     * Handle the mouse being released by resetting our re-ordering drag state
     */
    void DockTabBar::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            event->ignore();
            return;
        }

        finishDrag();
    }

    /**
     * Once our animation for re-positioning the tab we were dragging has
     * finished we need to reset our dragging state
     */
    void DockTabBar::dragTabAnimationFinished()
    {
        m_dragInProgress = false;
        m_draggingTabImage->hide();
        m_dragStartPosition = QPoint();
        m_tabIndexPressed = -1;
        updateCloseButtonPosition();
        update();
    }

    /**
     * Once the animation for displacing a tab from dragging the other tab
     * around has finished, we need to reset the displaced tab state so that
     * the regular tab can be shown
     */
    void DockTabBar::displacedTabAnimationFinished()
    {
        m_displacedTabImage->hide();
        m_displacedTabIndex = -1;
        update();
    }

    /**
     * Reset our internal dragging to re-order the tabs state by sliding the
     * tab that was being dragged back into place
     */
    void DockTabBar::finishDrag()
    {
        // If we never started a drag, we still need to reset some of the other
        // state variables (e.g. tab index pressed)
        if (!m_dragInProgress)
        {
            dragTabAnimationFinished();
            return;
        }

        if (m_dragTabFinishedAnimation->state() == QAbstractAnimation::Running)
        {
            m_dragTabFinishedAnimation->stop();
        }      
        m_dragTabFinishedAnimation->setStartValue(m_draggingTabImage->geometry());
        m_dragTabFinishedAnimation->setEndValue(tabRect(m_tabIndexPressed));
        m_dragTabFinishedAnimation->start();
    }

    /**
     * Grab a screenshot of the specified area and set it as the pixmap of the
     * given animated label
     */
    void DockTabBar::setImageForAnimatedLabel(QLabel* label, const QRect& area)
    {
        if (!label)
        {
            return;
        }

        QPixmap tabImage = grab(area);
        label->setGeometry(area);
        label->setPixmap(tabImage);
        label->show();
    }

#include <Components/DockTabBar.moc>
} // namespace AzQtComponents